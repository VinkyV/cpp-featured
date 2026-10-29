#include <iostream>
#include <windows.h>

#include <cassert>

#define TEMP_ALLOC_IMPLEMENTATION
#include <cpp-featured.hpp>

int main(int argc, char **argv) {
    for (int i = 0; i < 5; i++) {
        TEMP_POOL {
            char* msg  = tprintf("hello #%d", i);          // temp string
            int  *nums = (int *)tmalloc(10000 * sizeof *nums); // temp array

            for (int j = 0; j < 5; j++) nums[j] = j * i;
            println("%s -> last = %d", msg, nums[4]);
        } // msg and nums are freed here, no free() needed
    }
    str msg = "Hello World";
    println(msg.data());

    if (argc > 1) {
        panic("Too much arguments!");
    }

    println("OK!");

    return 0;
}