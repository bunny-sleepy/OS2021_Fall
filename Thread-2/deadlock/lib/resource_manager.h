#ifndef DEADLOCK_LIB_RESOURCE_MANAGER_H_
#define DEADLOCK_LIB_RESOURCE_MANAGER_H_

#include <map>
#include <mutex>
#include <thread>
#include <condition_variable>
#include "thread_manager.h"

namespace proj2 {

enum RESOURCE {
    GPU = 0,
    MEMORY,
    DISK,
    NETWORK
};

class ResourceManager {
public:
    ResourceManager(ThreadManager *t, std::map<RESOURCE, int> init_count): \
        resource_amount(init_count), tmgr(t) {}
    void budget_claim(std::map<RESOURCE, int> budget);
    int request(RESOURCE, int amount);
    void release(RESOURCE, int amount);
    bool bank();
    std::map<std::thread::id, int> alloc;
private:
    std::map<RESOURCE, int> resource_amount;
    std::map<RESOURCE, std::mutex> resource_mutex;
    std::map<std::thread::id, std::map<RESOURCE, int>> max_res;
    std::map<std::thread::id, std::map<RESOURCE, int>> alloc_res;
    std::mutex max_mutex;
    std::mutex alloc_mutex;
    std::mutex bank_mutex;
    std::condition_variable bank_cv;
    std::map<RESOURCE, std::condition_variable> resource_cv;
    ThreadManager *tmgr;
};

}  // namespce proj2

#endif // DEADLOCK_LIB_RESOURCE_MANAGER_H_