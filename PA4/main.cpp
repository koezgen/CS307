#include <iostream>
#include <pthread.h>
#include <random>
#include "allocator.cpp"

using namespace std;


void* threadfunc1(void* arg)
{
    auto* memory = static_cast<HeapManager*>(arg);
    auto addr = memory->myMalloc(pthread_self(), 10);
    memory->myFree(pthread_self(), addr);
    return nullptr;
}

void* threadfunc2(void* arg)
{
    auto* memory = static_cast<HeapManager*>(arg);
    auto addr = memory->myMalloc(pthread_self(), 20);
    memory->myFree(pthread_self(), addr);
    return nullptr;
}

void* threadfunc3(void* arg)
{
    auto* memory = static_cast<HeapManager*>(arg);
    auto addr = memory->myMalloc(pthread_self(), 35);
    memory->myFree(pthread_self(), addr);
    return nullptr;
}

void* threadfunc4(void* arg)
{
    auto* memory = static_cast<HeapManager*>(arg);
    auto addr = memory->myMalloc(pthread_self(), 35);
    memory->myFree(pthread_self(), addr);
    return nullptr;
}

int main()
{
    HeapManager heap;
    heap.initHeap(100);

    pthread_t threads[10];
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(1, 4); // Uniform distribution between 1 and 4

    for (auto &thread : threads) {
        int funcNum = distrib(gen); // Generate a random number between 1 and 4
        void* (*selectedFunc)(void*) = nullptr;

        switch (funcNum) {
            case 1: selectedFunc = threadfunc1; break;
            case 2: selectedFunc = threadfunc2; break;
            case 3: selectedFunc = threadfunc3; break;
            case 4: selectedFunc = threadfunc4; break;
            default: return 1;
        }

        if (pthread_create(&thread, nullptr, selectedFunc, (void* ) &heap) != 0) {
            cerr << "Error creating thread" << endl;
            return 1;
        }
    }

    for (auto &thread : threads) {
        pthread_join(thread, nullptr);
    }

    cout << "Execution is done" << endl;
    heap.print();

    return 0;
}
