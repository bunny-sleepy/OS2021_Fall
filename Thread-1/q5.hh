#ifndef _Q5_HH_
#define _Q5_HH_
#include <vector>
#include <tuple>
#include <mutex>
#include <thread>
#include <condition_variable>

#include <string>   // string
#include <chrono>   // timer
#include <iostream> // cout, endl

#include "lib/utils.h"
#include "lib/model.h" 
#include "lib/embedding.h" 
#include "lib/instruction.h"

namespace proj1 {

class GlobalRecord {
public:
    GlobalRecord(): global_epoch(0) {}
    ~GlobalRecord() = default;
    // current minimum epoch
    int global_epoch;
    unsigned int get_epoch(int index) const { return epochs[index + 1]; }
    void set_epoch(int index, unsigned int value) { epochs[index + 1] = value; }
    void create_epoch(int index) {
        while (epochs.size() < index + 2)
            epochs.push_back(0);
    }
    void update(int index) {
        std::lock_guard<std::mutex> lck(this->mtx);
        epochs[index + 1]--;
        for (unsigned int i = 1; i <= epochs.size(); ++i) {
            if (i == epochs.size() || epochs[i] != 0) {
                global_epoch = i - 1;
                break;
            }
        }
    }
    void write_recommend(Embedding* recommendation) {
        std::lock_guard<std::mutex> lck(this->mtx);
        recommendation->write_to_stdout();
    }
private:
    // number of instructions left unfinished in each epoch
    std::vector<unsigned int> epochs;
    mutable std::mutex mtx;
}; // global_record

class my_cv {
public:
    friend class GlobalRecord;
    // constructor
    my_cv();
    // destructor
    ~my_cv() = default;
    // want the resouces, lock this; if the resouce is busy,
    // then wait until the lock is open and the process is notified.
    void check_and_lock();
    // check lock with epoch
    void check_and_lock_with_epoch(int epoch);
    // check and lock with iter_idx
    void check_and_lock_with_iter(int iter_idx);
    // lock, reture true if success
    bool test_and_lock();
    // lock with epoch, return true if success
    bool test_and_lock_with_epoch(int epoch);
    // unlock
    void unlock();
    // wait
    bool wait();
    // unlock and notify
    void unlock_and_notify();
    // unlock and notify with epoch
    void unlock_and_notify_with_epoch(int epoch);
    // unlock and notify all
    void notify_all();
    // check lock
    bool check_lock() const { return !this->busy; }
    // wait and set with epoch
    bool wait_with_epoch(int epoch);
    // get epoch
    int get_epoch() const { return this->epoch_now; }
    // update epoch
    void update_epoch(int epoch) { this->epoch_now = epoch; }
    std::vector<int> inst_num_in_one_epoch;
    std::condition_variable cv;
private:
    int epoch_now;
    std::condition_variable cv_iter;
    mutable std::mutex mtx;
    mutable std::mutex mtx_iter;
    bool busy;
}; // my_cv

// require the resources of both user and item, if either of them is busy, wait.
// prevent deadlocks
void require_resources(int user_idx, int item_idx, int epoch);
// prevent deadlocks
void require_resources_recommendation(int user_idx, std::vector<int> item_idx_list, int iter_idx);
// multi-threading for initialization
void run_one_init(EmbeddingHolder* users, EmbeddingHolder* items,
                  Embedding* new_user, int item_index, int user_idx);

void run_one_instruction(Instruction inst, EmbeddingHolder* users, EmbeddingHolder* items);

void init_epoch(int item_idx, int user_idx, int epoch);

} // namespace proj1

#endif // _Q5_HH_