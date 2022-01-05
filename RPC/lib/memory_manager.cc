#include "memory_manager.h"
#include <chrono>
#include <thread>
#include <cstdlib>
#include <iostream>
#define useClock true

namespace proj4 {
  PageFrame::PageFrame() {
    if (access("./disk", 00)) { 
      mkdir("./disk", 0777);
    }
  }

  int &PageFrame::operator[](unsigned long idx) {
    //each page should provide random access like an array
    return mem[idx];
  }

  void PageFrame::WriteDisk(std::string filename) {
    FILE *fp = fopen(("disk/" + filename).c_str(), "w");
    fwrite(mem, sizeof(mem), 1, fp);
    fclose(fp);
  }

  void PageFrame::ReadDisk(std::string filename) {
    // read page content from disk files
    FILE *fp = fopen(("disk/" + filename).c_str(), "r");
    fread(mem, sizeof(mem), 1, fp);
    fclose(fp);
  }

  PageInfo::PageInfo() {
    holder = -1;
    virtual_page_id = -1;
    clock = 0;
  }

  void PageInfo::SetInfo(int cur_holder, int cur_vid, int cur_used = 0) {
    //modify the page states
    //you can add extra parameters if needed
    holder = cur_holder;
    virtual_page_id = cur_vid;
    clock = cur_used;
  }
  
  void PageInfo::clock_count(int cur_used = 1) {
    clock = cur_used;
  }

  void PageInfo::ClearInfo() {
    //clear the page states
    //you can add extra parameters if needed
    holder = -1;
    virtual_page_id = -1;
    clock = 0;
  }

  void PageInfo::lock() {
    page_mutex.lock();
  }

  void PageInfo::Unlock() {
    page_mutex.unlock();
  }

  int PageInfo::GetHolder() { return holder; }
  int PageInfo::GetVid() { return virtual_page_id; }
  int PageInfo::GetClock() { return clock; }

  MemoryManager::MemoryManager(size_t sz) {
    //mma should build its memory space with given space size
    //you should not allocate larger space than 'sz' (the number of physical pages)
    page_map.clear();
    mem = new PageFrame[sz]();
    page_info = new PageInfo[sz]();
    free_list = new unsigned int[sz]();
    for (int i = 0; i < sz; i++)
      free_page.push(i);
    next_array_id = 0;
    mma_sz = sz;
    clock_ptr = 0;
    clock_counter.clear();
    page_in_disk.clear();
  }
  MemoryManager::~MemoryManager() {}

  void MemoryManager::PageOut(int array_id, int virtual_page_id, int phyPageidx) {
    // swap out the physical page with the indx of 'phyPageidx out' into a disk file
    auto filename = std::to_string(array_id) + "," + std::to_string(virtual_page_id);
    mem[phyPageidx].WriteDisk(filename);
    page_in_disk[std::make_pair(array_id, virtual_page_id)].unlock(); //acquire lock until the data is written in disk
  }

  void MemoryManager::PageIn(int array_id, int virtual_page_id, int phyPageidx) {
    // swap the target page from the disk file into a physical page with the index of 'phyPageidx out'
    auto filename = std::to_string(array_id) + "," + std::to_string(virtual_page_id);
    page_in_disk[std::make_pair(array_id, virtual_page_id)].lock(); //acquire lock until the data is written in disk
    mem[phyPageidx].ReadDisk(filename);
    page_in_disk[std::make_pair(array_id, virtual_page_id)].unlock();
  }

  void MemoryManager::PageReplace(int array_id, int virtual_page_id, bool allocate = false) {
    if (allocate) {
      pm_mutex.lock();
    }
    if (!free_page.empty()) {
      // allocate an empty page
      int phyPageidx = free_page.front();
      page_info[phyPageidx].lock();
      free_list[phyPageidx] = 1;
      free_page.pop();
      // check alloc/page in.
      int exist = page_map[array_id][virtual_page_id];
      page_map[array_id][virtual_page_id] = phyPageidx;
      page_info[phyPageidx].SetInfo(array_id, virtual_page_id);
      // FIFO
      if (!useClock) {
        used_page.push(std::make_pair(phyPageidx, array_id));
      }
      // CLOCK
      if (useClock) {
        if (clock_ptr == 0) {
          clock_counter.push_back(std::make_pair(phyPageidx, array_id));
        }
        else {
          clock_counter.insert(clock_counter.begin() + clock_ptr, std::make_pair(phyPageidx, array_id));
          clock_ptr++;
        }
      }
      pm_mutex.unlock();
      if (exist == -1) {
        PageIn(array_id, virtual_page_id, phyPageidx);
        // std::cout << "get here 2"<< std::endl;
      } else {
        for (size_t i = 0; i < PageSize; i++) {
          mem[phyPageidx][i] = 0;
        }
      }
      if (allocate) {
        page_info[phyPageidx].Unlock();
      }
    }
    else {
      // FIFO
      if (!useClock) {
        replaceFIFO(array_id, virtual_page_id, allocate);
      }
      // CLOCK
      if (useClock) {
        replaceClock(array_id, virtual_page_id, allocate);
      }
    }
  }

  void MemoryManager::replaceClock(int array_id, int virtual_page_id, bool allocate) {
    while (true) {
      std::pair<int, int> pid_arrid = clock_counter[clock_ptr];
      int phyPageidx = pid_arrid.first;
      if (pid_arrid.second != page_info[phyPageidx].GetHolder()) {
        clock_counter.erase(clock_counter.begin() + clock_ptr);
        if (clock_ptr == clock_counter.size()) {
          clock_ptr = 0;
        }
        continue;
      }
      if (page_info[phyPageidx].GetClock() == 1) {
        page_info[phyPageidx].clock_count(0);
        clock_ptr++;
        if (clock_ptr == clock_counter.size()) {
          clock_ptr = 0;
        }
        continue;
      }
      clock_counter[clock_ptr].second = array_id;
      clock_ptr++;
      if (clock_ptr == clock_counter.size()) {
        clock_ptr = 0;
      }
      page_info[phyPageidx].lock();
      /*page out info*/
      int old_holder = page_info[phyPageidx].GetHolder();
      int old_vid = page_info[phyPageidx].GetVid();
      page_map[old_holder][old_vid] = -1;
      /*page out info*/
      /*page in info*/
      int exist = page_map[array_id][virtual_page_id];
      page_map[array_id][virtual_page_id] = phyPageidx;
      page_info[phyPageidx].SetInfo(array_id, virtual_page_id);
      /*page in info*/
      page_in_disk[std::make_pair(old_holder, old_vid)].lock();
      pm_mutex.unlock();
      PageOut(old_holder, old_vid, phyPageidx);
      if (exist == -1) {
        PageIn(array_id, virtual_page_id, phyPageidx);
      } else {
        for (size_t i = 0; i < PageSize; i++) {
          mem[phyPageidx][i] = 0;
        }
      } if (allocate) {
        page_info[phyPageidx].Unlock();
      }
      break;
    }
  }

  void MemoryManager::replaceFIFO(int array_id, int virtual_page_id, bool allocate) {
    while (true) {
      std::pair<int, int> pid_arrid = used_page.front();
      used_page.pop();
      int phyPageidx = pid_arrid.first;
      if (pid_arrid.second != page_info[phyPageidx].GetHolder()) continue;
      page_info[phyPageidx].lock();
      /*page out info*/
      used_page.push(std::make_pair(phyPageidx, array_id));
      int old_holder = page_info[phyPageidx].GetHolder();
      int old_vid = page_info[phyPageidx].GetVid();
      page_map[old_holder][old_vid] = -1;
      /*page out info*/
      /*page in info*/
      int exist = page_map[array_id][virtual_page_id];
      page_map[array_id][virtual_page_id] = phyPageidx;
      page_info[phyPageidx].SetInfo(array_id, virtual_page_id);
      /*page in info*/
      page_in_disk[std::make_pair(old_holder, old_vid)].lock(); // lock the file to be pageout
      pm_mutex.unlock(); //unlock information
      PageOut(old_holder, old_vid, phyPageidx);
      if (exist == -1) {
        PageIn(array_id, virtual_page_id, phyPageidx);
      } else {
        for (size_t i = 0; i < PageSize; i++) {
          mem[phyPageidx][i] = 0;
        }
      }
      if (allocate) {
        page_info[phyPageidx].Unlock();
      }
      break;
    }
  }

  int MemoryManager::ReadPage(int array_id, int virtual_page_id, int offset) {
    // for arrayList of 'array_id', return the target value on its virtual space
    pm_mutex.lock();
    int phyPageidx = page_map[array_id][virtual_page_id];
    if (phyPageidx <= -1) {
      PageReplace(array_id, virtual_page_id);
      phyPageidx = page_map[array_id][virtual_page_id];
    } else {
      page_info[phyPageidx].lock();
      page_info[phyPageidx].clock_count();
      pm_mutex.unlock();
    }
    int ret = mem[phyPageidx][offset];
    page_info[phyPageidx].Unlock();
    return ret;
  }
  void MemoryManager::WritePage(int array_id, int virtual_page_id, int offset, int value) {
    // for arrayList of 'array_id', write 'value' into the target position on its virtual space
    pm_mutex.lock();
    int phyPageidx = page_map[array_id][virtual_page_id];
    if (phyPageidx <= -1) {
      PageReplace(array_id, virtual_page_id);
      phyPageidx = page_map[array_id][virtual_page_id];
    } else {
      page_info[phyPageidx].lock();
      page_info[phyPageidx].clock_count();
      pm_mutex.unlock();
    }
    mem[phyPageidx][offset] = value;
    // std::cout << "page id"<< phyPageidx << std::endl;
    page_info[phyPageidx].Unlock();
  }
  int MemoryManager::Allocate(size_t sz) {
    // when an application requires for memory, create an ArrayList and record mappings 
    // from its virtual memory space to the physical memory space
    int pageNum = 0;
    if (sz % PageSize == 0) {
      pageNum = sz / PageSize;
    } else {
      pageNum = sz / PageSize + 1;
    }
    pm_mutex.lock();
    int array_id = next_array_id++;
    page_map[array_id];
    for (int i = 0; i < pageNum; i++) {
      page_map[array_id][i] = -2; // -2 for unallocated, -1 for in disk files
    }
    pm_mutex.unlock();
    return array_id;
  }
  int MemoryManager::Release(int idx) {
    this->pm_mutex.lock();
    int vir_page_num = 0;
    for (auto it: this->page_map[idx]) {
      if (it.second >= 0) {
        vir_page_num++;
        this->page_info[it.second].ClearInfo();
        this->free_list[it.second] = 0;
        this->free_page.push(it.second);
      }
    }
    this->page_map.erase(idx);
    this->pm_mutex.unlock();
    return vir_page_num;
  }
} 
