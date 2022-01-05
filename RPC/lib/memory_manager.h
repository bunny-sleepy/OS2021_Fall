#ifndef MEMORY_MANAGER_H_
#define MEMORY_MANAGER_H_

#include <assert.h>
#include <map>
#include <string>
#include <cstdlib>
#include<cstdio>
#include <queue>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <mutex>
#include <set>
#include <condition_variable>

#define PageSize 1024

namespace proj4 {

class PageFrame {
public:
    PageFrame();
    int& operator[] (unsigned long);
    void WriteDisk(std::string);
    void ReadDisk(std::string);
private:
    int mem[PageSize] = {};
};

class PageInfo {
public:
    PageInfo();
    void SetInfo(int,int,int);
    void clock_count(int);
    void ClearInfo();
    void lock();
    void Unlock();
    int GetHolder();
    int GetVid();
    int GetClock();
private:
    int holder; //page holder id (array_id)
    int virtual_page_id; // page virtual #
    /*add your extra states here freely for implementation*/
    int clock;
    std::mutex page_mutex;
};

class ArrayList;

class MemoryManager {
public:
    // you should not modify the public interfaces used in tests
    MemoryManager(size_t);
    int ReadPage(int array_id, int virtual_page_id, int offset);
    void WritePage(int array_id, int virtual_page_id, int offset, int value);
    int Allocate(size_t);
    void Release(ArrayList*);
    int Release(int);
    ~MemoryManager();
private:
    std::map<int, std::map<int, int>> page_map; // mapping from ArrayList's virtual page # to physical page #
    PageFrame* mem; // mapping from virtual address to disk address
    PageInfo* page_info; // physical page info
    unsigned int* free_list;  // use bitmap implementation to identify and search for free pages
    int next_array_id;
    size_t mma_sz; 
    /*add your extra states here freely for implementation*/

    std::queue<int> free_page;
    //Acknowledge: thanks Zijian Zhu for discussing the clock/fifo policy when concerning release mem
    std::queue<std::pair<int, int>> used_page; // pair: (phyPageidx, array_id), for FIFO
    std::vector<std::pair<int, int>> clock_counter; // pair: ((phyPageidx, array_id), clock), for CLOCK
    int clock_ptr;
    
    std::mutex pm_mutex;
    // std::mutex pi_mutex;
    // std::mutex mem_mutex;
    // std::mutex q_mutex;
    std::map<std::pair<int, int>, std::mutex> page_in_disk; 

    void PageIn(int array_id, int virtual_page_id, int phyPageidx);
    void PageOut(int array_id, int virtual_page_id, int phyPageidx);
    void PageReplace(int array_id, int virtual_page_id, bool allocate);
    void replaceFIFO(int array_id, int virtual_page_id, bool allocate);
    void replaceClock(int array_id, int virtual_page_id, bool allocate);
};

} 

#endif
