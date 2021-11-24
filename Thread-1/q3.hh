#ifndef _Q3_HH_
#define _Q3_HH_
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

class my_cv {
public:
    // constructor
    my_cv();
    // destructor
    ~my_cv() = default;
    // want the resouces, lock this; if the resouce is busy,
    // then wait until the lock is open and the process is notified.
    void check_and_lock();
    // check lock with epoch
    void check_and_lock_with_epoch(int epoch);
    // lock with epoch, return true if success
    bool test_and_lock_with_epoch(int epoch);
    // unlock
    void unlock();
    // unlock and notify
    void unlock_and_notify();
    // unlock and notify with epoch
    void unlock_and_notify_with_epoch(int epoch);
    // check lock
    bool check_lock() const { return !this->busy; }
    // wait and set with epoch
    bool wait_with_epoch(int epoch);
    // get epoch
    int get_epoch() const { return this->epoch_now; }
    // update epoch
    void update_epoch(int epoch) { this->epoch_now = epoch; }
    std::vector<int> inst_num_in_one_epoch;
private:
    int epoch_now;
    std::condition_variable cv;
    mutable std::mutex mtx;
    bool busy;
}; // my_cv

// require the resources of both user and item, if either of them is busy, wait.
// prevent deadlocks
void require_resources(int user_idx, int item_idx, int epoch);
// multi-threading for initialization
void run_one_init(EmbeddingHolder* users, EmbeddingHolder* items,
                  Embedding* new_user, int item_index, int user_idx);

void run_one_instruction(Instruction inst, EmbeddingHolder* users, EmbeddingHolder* items);

void init_epoch(int item_idx, int user_idx, int epoch);

} // namespace proj1

#endif // _Q3_HH_