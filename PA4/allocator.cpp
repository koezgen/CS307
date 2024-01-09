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
    Node* prev;
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

        else break;
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
        if ((iter->ID == -1) && (iter->SIZE >= size))
        {
            Node* temp = iter->next;
            Node* alloc = new Node(iter->ID, iter->SIZE - size, iter->INDEX + size);
            iter->ID = tid;
            iter->SIZE = size;

            iter->next = alloc;
            alloc->next = temp;
            alloc->prev = iter;

            if (alloc->SIZE == 0)
            {
                iter->next = nullptr;
                delete alloc;
            }

            cout << "Allocated for thread " << tid << endl;
            fsync(STDOUT_FILENO);
            print();
            pthread_mutex_unlock(&MUTEX);
            return iter->INDEX;
        }

        else if (iter->next != nullptr)
        {
            iter = iter->next;
        }

        else
        {
            cout << "Can not allocate, requested size " << size << " for thread " << tid << " is bigger than remaining size" << endl;
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

int dec_var(Node* p, Node* n)
{
    if ((p == nullptr) && (n == nullptr))
    {
        return 0;
    }

    else if ((p == nullptr) && (n != nullptr))
    {
        return 1;
    }

    else if ((p != nullptr) && (n == nullptr))
    {
        return 2;
    }

    else if ((p->ID != -1) && (n->ID != -1))
    {
        return 3;
    }

    else if ((p->ID != -1) && (n->ID == -1))
    {
        return 4;
    }

    else if ((p->ID == -1) && (n->ID != -1))
    {
        return 5;
    }

    else if ((p->ID == -1) && (n->ID == -1))
    {
        return 6;
    }
}

// TODO: NEED TO IMPLEMENT
int HeapManager::myFree(int tid, int index)
{
    pthread_mutex_lock(&MUTEX);

    Node* iter = MEMORY;
    while (iter != nullptr)
    {
        if ((iter->ID == tid) && (iter->INDEX == index))
        {
            Node* previous = iter->prev;
            Node* next = iter->next;

            // There are 4 cases for coalescence.
            switch (dec_var(previous, next))
            {
                // Both sides are NULL
                case 0:
                    iter->ID = -1;
                    cout << "Freed for thread " << tid << endl;
                    fsync(STDOUT_FILENO);
                    print();
                    pthread_mutex_unlock(&MUTEX);
                    return 1;

                // Left side is NULL
                case 1:
                    if (next->ID != -1)
                    {
                        iter->ID = -1;
                        cout << "Freed for thread " << tid << endl;
                        fsync(STDOUT_FILENO);
                        print();
                        pthread_mutex_unlock(&MUTEX);
                        return 1;
                    }

                    else
                    {
                        next->SIZE += iter->SIZE;
                        next->INDEX = iter->INDEX;
                        next->prev = nullptr;
                        delete iter;
                        MEMORY = next;
                        cout << "Freed for thread " << tid << endl;
                        fsync(STDOUT_FILENO);
                        print();
                        pthread_mutex_unlock(&MUTEX);
                        return 1;
                    }

                // Right side is NULL
                case 2:
                    if (previous->ID != -1)
                    {
                        iter->ID = -1;
                        cout << "Freed for thread " << tid << endl;
                        fsync(STDOUT_FILENO);
                        print();
                        pthread_mutex_unlock(&MUTEX);
                        return 1;
                    }

                    else
                    {
                        previous->SIZE += iter->SIZE;
                        previous->next = nullptr;
                        delete iter;
                        cout << "Freed for thread " << tid << endl;
                        fsync(STDOUT_FILENO);
                        print();
                        pthread_mutex_unlock(&MUTEX);
                        return 1;
                    }

                // Both sides are full.
                case 3:
                    iter->ID = -1;
                    cout << "Freed for thread " << tid << endl;
                    fsync(STDOUT_FILENO);
                    print();
                    pthread_mutex_unlock(&MUTEX);
                    return 1;

                // Left side is full.
                case 4:
                    next->SIZE += iter->SIZE;
                    next->INDEX = iter->INDEX;
                    previous->next = next;
                    next->prev = previous;
                    delete iter;
                    cout << "Freed for thread " << tid << endl;
                    fsync(STDOUT_FILENO);
                    print();
                    pthread_mutex_unlock(&MUTEX);
                    return 1;

                // Right side is full.
                case 5:
                    previous->SIZE += iter->SIZE;
                    previous->next = next;
                    next->prev = previous;
                    delete iter;
                    cout << "Freed for thread " << tid << endl;
                    fsync(STDOUT_FILENO);
                    print();
                    pthread_mutex_unlock(&MUTEX);
                    return 1;

                // Both sides are empty.
                case 6:
                    // Coalesce into right.
                    Node* next_buf = iter->next;
                    next->SIZE += iter->SIZE;
                    next->INDEX = iter->INDEX;
                    previous->next = next;
                    next->prev = previous;
                    delete iter;

                    // Coalesce into left.
                    previous->SIZE += next_buf->SIZE;
                    previous->next = next_buf->next;
                    delete next_buf;

                    cout << "Freed for thread " << tid << endl;
                    fsync(STDOUT_FILENO);
                    print();
                    pthread_mutex_unlock(&MUTEX);
                    return 1;
            }
        }

        else
        {
            iter = iter->next;
        }
    }

    fsync(STDOUT_FILENO);
    pthread_mutex_unlock(&MUTEX);
    return -1;
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
