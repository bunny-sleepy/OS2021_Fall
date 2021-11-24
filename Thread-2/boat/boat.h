#ifndef BOAT_H_
#define BOAT_H_

#include <cstdio>
#include <thread>
#include <mutex>
#include <unistd.h>
#include <vector>
#include <iostream>
#include <condition_variable>

#include "boatGrader.h"

namespace proj2 {
class Boat {
public:
    Boat();
    ~Boat() = default;
    // interface for main
    void begin(int, int, BoatGrader*);
    // determine if the boat is available to go to
    bool boat_available();
    // total number of children
    unsigned int total_children, total_adults;
    // number of people on each island
    unsigned int oahu_children, oahu_adults, molokai_children, molokai_adults;
    // whether boat is at oahu
    bool boat_on_oahu;
    // if there is free space
    unsigned int space_to_molokai;
    // for boat mutual exclusion
    std::condition_variable cv;
    mutable std::mutex mtx;
};
// the child thread
void child(Boat*, BoatGrader*);
// the adult thread
void adult(Boat*, BoatGrader*);

} // namespace proj2

#endif // BOAT_H_
