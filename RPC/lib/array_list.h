#ifndef ARRAY_LIST_H
#define ARRAY_LIST_H

#include <cstdlib>

namespace proj4 {

    class MmaClient;

    class ArrayList{
        private:
        friend class MmaClient;
        size_t size;
        MmaClient* mma;
        int array_id;
        ArrayList(size_t, MmaClient*, int);
        ~ArrayList();
        public:
        // you should not modify the public interfaces used in tests
        int Read (unsigned long);
        void Write (unsigned long, int);
    };

} // namespce: proj3
#endif // ARRAY_LIST_H