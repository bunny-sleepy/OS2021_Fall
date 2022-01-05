#include "boat.h"

using std::size_t;

namespace proj2 {
	
Boat::Boat() {
    oahu_children = 0;
    oahu_adults = 0;
    molokai_children = 0;
    molokai_adults = 0;
    boat_on_oahu = true;
    space_to_molokai = 1;
}

void Boat::begin(int adults, int children, BoatGrader *bg) {
    this->oahu_adults = adults;
    this->oahu_children = children;
    this->total_adults = adults;
    this->total_children = children;
    // create threads
    std::vector<std::thread> threads_child, threads_adult;
    for (size_t i = 0; i < adults; ++i) {
        threads_adult.push_back(std::thread(adult, this, bg));
    }
    for (size_t i = 0; i < children; ++i) {
        threads_child.push_back(std::thread(child, this, bg));
    }
    // join threads
    for (auto& ch_thread : threads_adult) {
        if (ch_thread.joinable()) {
            ch_thread.join();
        }
    }
    for (auto& ch_thread : threads_child) {
        if (ch_thread.joinable()) {
            ch_thread.join();
        }
    }
}

void child(Boat* boat, BoatGrader* bg) {
    const unsigned int total_children = boat->total_children;
    const unsigned int total_adults = boat->total_adults;

    std::mutex private_mtx;
    std::unique_lock<std::mutex> private_lck(private_mtx);
    // on oahu
    while (true) {
        bool flag = false;
        while (true) {
            if (boat->space_to_molokai) {
                std::lock_guard<std::mutex> boat_lck(boat->mtx);
                if (!boat->space_to_molokai) {
                    continue;
                }
                bg->ChildRideToMolokai();
                boat->oahu_children--;
                boat->molokai_children++;
                boat->space_to_molokai--;
                boat->cv.notify_all();
                break;
            }
            else if (!boat->boat_on_oahu) {
                boat->cv.wait(private_lck);
            }
            else {
                // acquire boat
                std::lock_guard<std::mutex> boat_lck(boat->mtx);
                if (!boat->boat_on_oahu) {
                    continue;
                }
                bg->ChildRowToMolokai();
                boat->oahu_children--;
                boat->molokai_children++;
                boat->space_to_molokai++;
                boat->boat_on_oahu = false;
                boat->cv.notify_all();
                break;
            }
        }
        // on molokai
        while (true) {
            if (boat->molokai_adults == total_adults && boat->molokai_children == total_children) {
                // check if all have arrived
                flag = true;
                boat->cv.notify_all();
                break;
            }
            else if (boat->boat_on_oahu) {
                boat->cv.wait(private_lck);
            }
            else {
                // row back
                std::lock_guard<std::mutex> boat_lck(boat->mtx);
                if (boat->boat_on_oahu) {
                    continue;
                }
                bg->ChildRowToOahu();
                boat->oahu_children++;
                boat->molokai_children--;
                boat->boat_on_oahu = true;
                boat->cv.notify_all();
                break;
            }
        }
        // terminated
        if (flag) {
            break;
        }
    }
}

void adult(Boat* boat, BoatGrader* bg) {
    std::mutex private_mtx;
    std::unique_lock<std::mutex> private_lck(private_mtx);
    // on oahu
    while (true) {
        if (!boat->boat_on_oahu || boat->oahu_children >= 2) {
            boat->cv.wait(private_lck);
        }
        else {
            // acquire boat
            std::lock_guard<std::mutex> boat_lck(boat->mtx);
            if (!boat->boat_on_oahu || boat->oahu_children >= 2) {
                continue;
            }
            // row to molokai
            bg->AdultRowToMolokai();
            boat->oahu_adults--;
            boat->molokai_adults++;
            boat->boat_on_oahu = false;
            boat->cv.notify_all();
            break;
        }
    }
    // on molokai
    // do nothing
}

} // namespace proj2