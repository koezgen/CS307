#include <iostream>
#include <string>
#include <pthread.h>
#include <unistd.h>

using namespace std;

struct Node {
    explicit Node(int ID = -1, unsigned int size = 0, unsigned int index = 0)
            : ID(ID), SIZE(size), INDEX(index), next(nullptr) {}

    int ID;
    unsigned int SIZE;
    unsigned int INDEX;
    Node* next;
};


class HeapManager
{
    private:
        Node* MEMORY;
        pthread_mutex_t MUTEX = PTHREAD_MUTEX_INITIALIZER;
        pthread_mutex_t PRINT_MUTEX = PTHREAD_MUTEX_INITIALIZER;
    public:
        explicit HeapManager();
        ~HeapManager();
        int initHeap(size_t mem_size);
        int myMalloc(int ID, int size);
        int myFree(int ID, int index);
        void print();
};

HeapManager::HeapManager()
{
    MEMORY = nullptr;
}

HeapManager::~HeapManager()
{
    while (MEMORY != nullptr)
    {
        Node* buf;
        buf = MEMORY->next;
        delete MEMORY;
        MEMORY = buf;
    }
}

void HeapManager::print()
{
    pthread_mutex_lock(&PRINT_MUTEX);

    Node* iter = MEMORY;

    while (iter != nullptr)
    {
        cout << "[" <<iter->ID << "]" << "[" <<iter->SIZE << "]"<< "[" <<iter->INDEX << "]";

        if (iter->next != nullptr)
        {
            cout << "---";
            iter = iter->next;
        }
    }

    cout << endl;

    fsync(STDOUT_FILENO);
    pthread_mutex_unlock(&PRINT_MUTEX);
}

int HeapManager::myMalloc(int tid, int size)
{
    pthread_mutex_lock(&MUTEX);
    Node* iter = MEMORY;
    while (iter != nullptr)
    {
        if (iter->SIZE >= size)
        {
            Node* temp = iter->next;
            Node* alloc = new Node(iter->ID, iter->SIZE - size, iter->INDEX + size);
            iter->ID = tid;
            iter->SIZE = size;

            iter->next = alloc;
            alloc->next = temp;

            return 0;
        }

        else if (iter->next != nullptr)
        {
            iter = iter->next;
        }

        else
        {
            cout << "Can not allocate, requested size " << size << "for thread " << tid << " is bigger than remaining size" << endl;
            fsync(STDOUT_FILENO);
            print();
            pthread_mutex_unlock(&MUTEX);
            return -1;
        }
    }
    fsync(STDOUT_FILENO);
    pthread_mutex_unlock(&MUTEX);
    return 0;
}

// TODO: NEED TO IMPLEMENT
int HeapManager::myFree(int ID, int index)
{
    pthread_mutex_lock(&MUTEX);
    cout << "Memory Free " << ID << " and " << index << endl;
    fsync(STDOUT_FILENO);
    pthread_mutex_unlock(&MUTEX);
    return 0;
}

int HeapManager::initHeap(size_t mem_size)
{
    pthread_mutex_lock(&MUTEX);
    MEMORY = new Node(-1, mem_size, 0);
    cout << "Memory initialized" << endl;
    fsync(STDOUT_FILENO);
    print();
    pthread_mutex_unlock(&MUTEX);
    return 0;
}
