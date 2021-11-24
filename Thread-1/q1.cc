#include "q1.hh"

std::mutex require_mutex;
std::vector<std::thread> thread_list;
std::vector<proj1::my_cv*> user_resources;
std::vector<proj1::my_cv*> item_resources;

namespace proj1 {

// implement my_cv class
my_cv::my_cv(): busy(false) {}

void my_cv::lock_it() {
    std::unique_lock<std::mutex> lck(this->mtx);
    while (this->busy) {
        this->cv.wait(lck);
    }
    this->busy = true;
}

void my_cv::unlock_it() {
    this->busy = false;
}

void my_cv::unlock_and_notify_them() {
    this->busy = false;
    this->cv.notify_all();
}

bool my_cv::check_lock() const {
    return !this->busy;
}

void require_resources(int user_idx, int item_idx) {
    // make sure only one thread is asking for resources.
    std::unique_lock<std::mutex> lock(require_mutex); 
    while (true) {
        user_resources[user_idx]->lock_it();
        if (item_resources[item_idx]->check_lock()) {
            item_resources[item_idx]->lock_it();
            return;
        }
        else {
            user_resources[user_idx]->unlock_it();
        }

        item_resources[item_idx]->lock_it();
        if (user_resources[user_idx]->check_lock()) {
            user_resources[user_idx]->lock_it();
            return;
        }
        else {
            item_resources[item_idx]->unlock_it();
        }
    }
}

void run_one_instruction(Instruction inst, EmbeddingHolder* users, EmbeddingHolder* items) {
    switch (inst.order) {
        case INIT_EMB: {
            // We need to init the embedding
            int length = users->get_emb_length();
            Embedding* new_user = new Embedding(length);
            int user_idx = users->append(new_user);
            user_resources.push_back(new my_cv());
            user_resources[user_idx]->lock_it();
            for (int item_index: inst.payloads) {
                item_resources[item_index]->lock_it();
                Embedding* item_emb = items->get_embedding(item_index);
                // Call cold start for downstream applications, slow
                EmbeddingGradient* gradient = cold_start(new_user, item_emb);
                users->update_embedding(user_idx, gradient, 0.01);
                item_resources[item_index]->unlock_and_notify_them();
                delete gradient;
            }
            user_resources[user_idx]->unlock_and_notify_them();
            break;
        }
        case UPDATE_EMB: {
            int user_idx = inst.payloads[0];
            int item_idx = inst.payloads[1];
            int label = inst.payloads[2];
            require_resources(user_idx, item_idx);
            Embedding* user = users->get_embedding(user_idx);
            Embedding* item = items->get_embedding(item_idx);
            EmbeddingGradient* gradient = calc_gradient(user, item, label);
            users->update_embedding(user_idx, gradient, 0.01);
            delete gradient;
            gradient = calc_gradient(item, user, label);
            items->update_embedding(item_idx, gradient, 0.001);
            delete gradient;
            user_resources[user_idx]->unlock_and_notify_them();
            item_resources[item_idx]->unlock_and_notify_them();
            break;
        }
        case RECOMMEND: {
            int user_idx = inst.payloads[0];
            user_resources[user_idx]->lock_it();
            Embedding* user = users->get_embedding(user_idx);
            user_resources[user_idx]->unlock_and_notify_them();
            std::vector<Embedding*> item_pool;
            int iter_idx = inst.payloads[1];
            for (unsigned int i = 2; i < inst.payloads.size(); ++i) {
                int item_idx = inst.payloads[i];
                item_resources[item_idx]->lock_it();
                item_pool.push_back(items->get_embedding(item_idx));
                item_resources[item_idx]->unlock_and_notify_them();
            }
            Embedding* recommendation = recommend(user, item_pool);
            recommendation->write_to_stdout();
            break;
        }
    }
}
} // namespace proj1

int main(int argc, char *argv[]) {
    proj1::EmbeddingHolder* users = new proj1::EmbeddingHolder("data/q1.in");
    proj1::EmbeddingHolder* items = new proj1::EmbeddingHolder("data/q1.in");
    proj1::Instructions instructions = proj1::read_instructrions("data/q1_instruction.tsv");
    {
    // print the timing of block
    proj1::AutoTimer timer("q1");
    for (unsigned int i = 0; i < users->get_n_embeddings(); i++) {
        user_resources.push_back(new proj1::my_cv()); // give the users that already exist a cv.  
    }
    // give the items a conditional variable. 
    for (unsigned int i = 0; i < items->get_n_embeddings(); i++) {
        item_resources.push_back(new proj1::my_cv());
    }

    // Run all the instructions
    for (proj1::Instruction inst: instructions) {
        thread_list.push_back(std::thread(proj1::run_one_instruction, inst, users, items));
    }
    // thread join
    for (unsigned int i = 0; i < thread_list.size(); i++) {
        if (thread_list[i].joinable()) {
            thread_list[i].join();
        }
    }
    }
    // write the result
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
