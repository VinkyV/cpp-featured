#include <iostream>
#include <windows.h>

#define TEMP_ALLOC_IMPLEMENTATION
#include <cpp-featured.hpp>

int main(int argc, char **argv) {
    for (int i = 0; i < 100; i++) {
        TEMP_POOL {
            char* msg  = tprintf("hello #%d", i);          // temp string
            int  *nums = (int *)tmalloc(10000 * sizeof *nums); // temp array

            for (int j = 0; j < 5; j++) nums[j] = j * i;
            println("%s -> last = %d", msg, nums[4]);
        } // msg and nums are freed here, no free() needed
    }
    str msg = "123124312";
    println(msg.data());

    if (argc > 1) {
        panic("Too much arguments!");
    }

    return 0;
}