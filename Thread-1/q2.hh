#ifndef _Q2_HH_
#define _Q2_HH_
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
    void lock_it();
    // unlock
    void unlock_it();
    // unlock and notify
    void unlock_and_notify_them();
    // check lock
    bool check_lock() const;
private:
    std::condition_variable cv;
    mutable std::mutex mtx;
    bool busy;
}; // my_cv

// require the resources of both user and item, if either of them is busy, wait.
// prevent deadlocks
void require_resources(int user_idx, int item_idx);
// multi-threading for initialization
void run_one_init(EmbeddingHolder* users, EmbeddingHolder* items,
                  Embedding* new_user, int item_index, int user_idx);

void run_one_instruction(Instruction inst, EmbeddingHolder* users, EmbeddingHolder* items);

} // namespace proj1

#endif // _Q2_HH_