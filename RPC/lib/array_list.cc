#include "array_list.h"

#include "memory_manager.h"
#include "mma_client.h"

namespace proj4 {
    ArrayList::ArrayList(size_t sz, MmaClient* cur_mma, int id) {
        mma = cur_mma;
        array_id = id;
        size = sz;
    }
    int ArrayList::Read (unsigned long idx) {
        //read the value in the virtual index of 'idx' from mma's memory space
        size_t virtual_pid = idx/PageSize;
        size_t offset = idx % PageSize; 
        return mma->ReadPage(array_id, virtual_pid, offset);
    }
    void ArrayList::Write (unsigned long idx, int value) {
        //write 'value' in the virtual index of 'idx' into mma's memory space
        size_t virtual_pid = idx/PageSize;
        size_t offset = idx % PageSize; 
        mma->WritePage(array_id, virtual_pid, offset, value);
    }
    ArrayList::~ArrayList() {}
} // namespce: proj3