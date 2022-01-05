#include "q3.hh"

std::vector<std::thread> thread_list;
std::vector<proj1::my_cv*> user_resources;
std::vector<proj1::my_cv*> item_resources;

namespace proj1 {

// implement my_cv class
my_cv::my_cv(): busy(false), epoch_now(0) {}

void my_cv::check_and_lock() {
    std::unique_lock<std::mutex> lck(this->mtx);
    while (this->busy) {
        this->cv.wait(lck);
    }
    this->busy = true;
}

void my_cv::check_and_lock_with_epoch(int epoch) {
    // want the resouces, lock this; if the resouce is busy, wait until the lock is open and the process is notified.
    std::unique_lock<std::mutex> lck(this->mtx);
    while (this->busy || epoch != this->epoch_now) {
        this->cv.wait(lck);
    }
    this->busy = true;
}

bool my_cv::test_and_lock_with_epoch(int epoch) {
    std::unique_lock<std::mutex> lck(this->mtx);
    if (this->busy || epoch != this->epoch_now) {
        return false;
    }
    this->busy = true;
    return true;
}

bool my_cv::wait_with_epoch(int epoch) {
    // check whether the resource is ready, if not, wait.
    std::unique_lock<std::mutex> lck(this->mtx);
    while (this->busy || this->epoch_now != epoch) {
        this->cv.wait(lck);
    }
    return true;
}

void my_cv::unlock() {
    this->busy = false;
}

void my_cv::unlock_and_notify() {
    std::lock_guard<std::mutex> lck(this->mtx);
    this->busy = false;
    this->cv.notify_all();
}

void my_cv::unlock_and_notify_with_epoch(int epoch) {
    std::lock_guard<std::mutex> lck(this->mtx);
    this->busy = false;
    this->inst_num_in_one_epoch[epoch]--;
    while (this->epoch_now < this->inst_num_in_one_epoch.size()
           && this->inst_num_in_one_epoch[this->epoch_now] == 0) {
        this->epoch_now++;
    }
    this->cv.notify_all();
}

void require_resources(int user_idx, int item_idx, int epoch) {
    // make sure only one thread is asking for resources.
    while (true) {
        item_resources[item_idx]->wait_with_epoch(epoch);
        user_resources[user_idx]->check_and_lock_with_epoch(epoch);
        if (item_resources[item_idx]->test_and_lock_with_epoch(epoch)) {
            return;
        }
        else {
            user_resources[user_idx]->unlock_and_notify();
        }
    }
}

void init_epoch(int item_idx, int user_idx, int epoch) {
    if (user_resources[user_idx]->inst_num_in_one_epoch.size() == 0) {
        user_resources[user_idx]->update_epoch(epoch);
    }
    if (item_resources[item_idx]->inst_num_in_one_epoch.size() == 0) {
        item_resources[item_idx]->update_epoch(epoch);
    }
    while (user_resources[user_idx]->inst_num_in_one_epoch.size() < epoch + 1) {
        user_resources[user_idx]->inst_num_in_one_epoch.push_back(0);
    }
    user_resources[user_idx]->inst_num_in_one_epoch[epoch]++;
    while (item_resources[item_idx]->inst_num_in_one_epoch.size() < epoch + 1) {
        item_resources[item_idx]->inst_num_in_one_epoch.push_back(0);
    }
    item_resources[item_idx]->inst_num_in_one_epoch[epoch]++;
}

void run_one_init(EmbeddingHolder* users, EmbeddingHolder* items, Embedding* new_user, int item_index, int user_idx) {
    item_resources[item_index]->check_and_lock();
    Embedding* item_emb = items->get_embedding(item_index);
    // Call cold start for downstream applications, slow
    EmbeddingGradient* gradient = cold_start(new_user, item_emb);
    users->update_embedding(user_idx, gradient, 0.01);
    delete gradient;
    item_resources[item_index]->unlock_and_notify();
}

void run_one_instruction(Instruction inst, EmbeddingHolder* users, EmbeddingHolder* items) {
    switch(inst.order) {
        case INIT_EMB: {
            // We need to init the embedding
            int length = users->get_emb_length();
            Embedding* new_user = new Embedding(length);
            int user_idx = users->append(new_user);
            user_resources[user_idx]->check_and_lock();
            std::vector<std::thread> thread_list_for_init;
            for (auto item_index : inst.payloads) {
                thread_list_for_init.push_back(std::thread(run_one_init, users, items, new_user, item_index, user_idx));
            }
            for (unsigned int i = 0; i < thread_list_for_init.size(); i++) {
                if (thread_list_for_init[i].joinable()) {
                    thread_list_for_init[i].join();
                }
            }
            user_resources[user_idx]->unlock_and_notify();
            break;
        }
        case UPDATE_EMB: {
            int user_idx = inst.payloads[0];
            int item_idx = inst.payloads[1];
            int label = inst.payloads[2];
            int epoch = 0;
            if (inst.payloads.size() > 3) {
               epoch = inst.payloads[3];
            }
            require_resources(user_idx, item_idx, epoch);
            Embedding* user = users->get_embedding(user_idx);
            Embedding* item = items->get_embedding(item_idx);
            EmbeddingGradient* gradient = calc_gradient(user, item, label);
            users->update_embedding(user_idx, gradient, 0.01);
            delete gradient;
            gradient = calc_gradient(item, user, label);
            items->update_embedding(item_idx, gradient, 0.001);
            delete gradient;
            user_resources[user_idx]->unlock_and_notify_with_epoch(epoch);
            item_resources[item_idx]->unlock_and_notify_with_epoch(epoch);
            break;
        }
        case RECOMMEND: {
            int user_idx = inst.payloads[0];
            user_resources[user_idx]->check_and_lock();
            Embedding* user = users->get_embedding(user_idx);
            user_resources[user_idx]->unlock_and_notify();
            std::vector<Embedding*> item_pool;
            int iter_idx = inst.payloads[1];
            for (unsigned int i = 2; i < inst.payloads.size(); ++i) {
                int item_idx = inst.payloads[i];
                item_resources[item_idx]->check_and_lock();
                item_pool.push_back(items->get_embedding(item_idx));
                item_resources[item_idx]->unlock_and_notify();
            }
            Embedding* recommendation = recommend(user, item_pool);
            recommendation->write_to_stdout();
            break;
        }
    }
}
} // namespace proj1

int main(int argc, char *argv[]) {
    proj1::EmbeddingHolder* users = new proj1::EmbeddingHolder("data/q3.in");
    proj1::EmbeddingHolder* items = new proj1::EmbeddingHolder("data/q3.in");
    proj1::Instructions instructions = proj1::read_instructrions("data/q3_instruction.tsv");
    {
    proj1::AutoTimer timer("q3");  // using this to print out timing of the block
    for (unsigned int i = 0; i < users->get_n_embeddings(); i++) {
        user_resources.push_back(new proj1::my_cv()); // give the users that already exist a cv.  
    }

    for (unsigned int i = 0; i < items->get_n_embeddings(); i++) {
        item_resources.push_back(new proj1::my_cv()); // give the items a cv. 
    }

    // For all the instructions, initial.
    for (proj1::Instruction inst: instructions) {
        switch (inst.order) {
            case proj1::INIT_EMB: {
                user_resources.push_back(new proj1::my_cv());
                break;
            }
            case proj1::UPDATE_EMB: {
                auto user_idx = inst.payloads[0];
                auto item_idx = inst.payloads[1];
                int epoch = 0;
                if (inst.payloads.size() > 3) {
                    epoch = inst.payloads[3];
                }
                proj1::init_epoch(item_idx, user_idx, epoch);
                break;
            }
        }
    }
    // run instructions
    for (proj1::Instruction inst: instructions) {
        thread_list.push_back(std::thread(proj1::run_one_instruction, inst, users, items));
    }
    for (unsigned int i = 0; i < thread_list.size(); i++) {
        if (thread_list[i].joinable()) {
            thread_list[i].join();
        }
    }
    }
    // Write the result
    users->write_to_stdout();
    items->write_to_stdout();
    for (unsigned int i = 0; i < user_resources.size(); i++) {
        delete user_resources[i];  
    }
    
    for (unsigned int i = 0; i < item_resources.size(); i++) {
        delete item_resources[i];
    }
    delete users;
    delete items;
    return 0;
}
