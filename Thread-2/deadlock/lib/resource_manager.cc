#include <mutex>
#include <thread>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include "resource_manager.h"

namespace proj2 {

int ResourceManager::request(RESOURCE r, int amount) {
    if (amount <= 0) {
        return 1;
    }

    std::unique_lock<std::mutex> lk(this->resource_mutex[r]);
    while (true) {
        if (this->resource_amount[r] >= amount) {
            this->resource_amount[r] -= amount;
            this->resource_mutex[r].unlock();

            auto this_id = std::this_thread::get_id();
            std::unique_lock<std::mutex> locka(this->alloc_mutex);
            this->alloc_res[this_id][r] += amount; //! don't support request release then request again
            this->alloc_mutex.unlock();

            std::unique_lock<std::mutex> lockb(this->bank_mutex);

            if (bank()) {
                this->bank_mutex.unlock();
                this->bank_cv.notify_all();
                break;
            }
            else {
                // return the resources.
                this->resource_amount[r] += amount;
                this->alloc_res[this_id][r] -= amount;
                this->bank_cv.wait(lockb);
            }
        } 
        else {
            this->resource_cv[r].wait(lk);
            auto this_id = std::this_thread::get_id();
            /* HINT: If you choose to detect the deadlock and recover,
                     implement your code here to kill and restart threads.
                     Note that you should release this thread's resources
                     properly.
             */
            if (tmgr->is_killed(this_id)) {
                return -1;
            }
        }
    }
    return 0;
}

void ResourceManager::release(RESOURCE r, int amount) {
    if (amount <= 0)  return;
    std::unique_lock<std::mutex> lk(this->resource_mutex[r]);
    this->resource_amount[r] += amount;
    this->resource_cv[r].notify_all();
}

void ResourceManager::budget_claim(std::map<RESOURCE, int> budget) {
    // This function is called when some workload starts.
    // The workload will eventually consume all resources it claims
    std::unique_lock<std::mutex> lockm(this->max_mutex);
    std::unique_lock<std::mutex> locka(this->alloc_mutex);

    auto this_id = std::this_thread::get_id();
    this->max_res[this_id] = budget;
    for (int i = 0; i < 4; ++i) {
        this->alloc_res[this_id][(RESOURCE)i] = 0;
    }
}

bool ResourceManager::bank() {
    for (int i = 0; i < 4; ++i) {
        std::unique_lock<std::mutex> lk(this->resource_mutex[(RESOURCE)i]);
    }
    auto avail = this->resource_amount;

    std::unique_lock<std::mutex> lockm(this->max_mutex);
    auto max = this->max_res;

    std::unique_lock<std::mutex> lockal(this->alloc_mutex);
    auto alloc = this->alloc_res;

    // release the mutex
    for (int i = 0; i < 4; ++i) {
        this->resource_mutex[(RESOURCE)i].unlock();
    }
    this->max_mutex.unlock();
    this->alloc_mutex.unlock();

    // the banker algo.
    bool done = true;

    std::map<std::thread::id, bool> unfinished;

    std::map<std::thread::id, std::map<RESOURCE, int>>::iterator i; // to cover all entries.
    for (i = max.begin(); i != max.end(); ++i) {
        unfinished[i->first] = true;
    }

    std::map<std::thread::id, bool>::iterator j;
    do {
        done = true;
        for (j = unfinished.begin(); j != unfinished.end(); j++) {
            if (unfinished[j->first] == true) {
                bool flag = true;
                for (int i = 0; i < 4; ++i) {
                    if (max[j->first][(RESOURCE)i] - alloc[j->first][(RESOURCE)i] > avail[(RESOURCE)i]) {
                        flag = false;
                        continue;
                    }
                }
                if (flag) {
                    unfinished[j->first] = false;
                    for (int i = 0; i < 4; ++i) {
                        avail[(RESOURCE)i] += alloc[j->first][(RESOURCE)i];
                        done = false;
                    }
                }
            }
        }
    } while (!done);

    for (j = unfinished.begin(); j != unfinished.end(); ++j) {
        if (unfinished[j->first]) {
            return false;
        }
    }
    return true;
}
} // namespace proj2