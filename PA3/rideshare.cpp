#include <iostream>
#include <pthread.h>
#include <vector>
#include <unistd.h>
using namespace std;

pthread_mutex_t lock;
pthread_barrier_t barrier;
pthread_mutex_t cout_mutex;

static unsigned long int carID = 0;

typedef struct __thread_args_ {
    char team;
} thread_args;

// I am using Dijkstra Semaphores.
typedef struct semaphore {
    pthread_mutex_t s_lock;
    pthread_cond_t cond;
    int s_value;
} sem_t;

void sem_init(sem_t *sem, int val) {
    pthread_mutex_init(&sem->s_lock, NULL);
    pthread_cond_init(&sem->cond, NULL);
    sem->s_value = val;
}

void sem_post(sem_t *sem) {
    pthread_mutex_lock(&sem->s_lock);
    sem->s_value++;
    pthread_cond_signal(&sem->cond);
    pthread_mutex_unlock(&sem->s_lock);
}

void sem_wait(sem_t *sem) {
    pthread_mutex_lock(&sem->s_lock);
    sem->s_value--;
    pthread_mutex_unlock(&lock);
    if (sem->s_value < 0) {
        pthread_cond_wait(&sem->cond, &sem->s_lock);
    }
    pthread_mutex_unlock(&sem->s_lock);
}

sem_t sem_A, sem_B;
sem_t *sem_T, *sem_O;

void* fan_threads(void *argc) {
    bool driver = false;
    auto *args = static_cast<thread_args*>(argc);

    // CRITICAL SECTION
    pthread_mutex_lock(&lock);

    pthread_mutex_lock(&cout_mutex);
    cout << "Thread ID " << pthread_self() << " Team: " << args->team << ", I am looking for a car" << endl;
    pthread_mutex_unlock(&cout_mutex);
    fsync(STDOUT_FILENO);

    if (args->team == 'A')
    {
        sem_T = &sem_A;
        sem_O = &sem_B;
    }

    else
    {
        sem_T = &sem_B;
        sem_O = &sem_A;
    }

    int val_T = sem_T->s_value;
    int val_O = sem_O->s_value;

    if(val_T < 0 && val_O < -1)
    {
        driver = true;
        sem_post(sem_T);

        for (int i = 0; i < 2; i++)
        {
            sem_post(sem_O);
        }
    }

    else if(val_T < -2)
    {
        driver = true;

        for (int i = 0; i < 3; i++)
        {
            sem_post(sem_T);
        }
    }

    else
    {
        sem_wait(sem_T);
    }

    pthread_mutex_lock(&cout_mutex);
    cout << "Thread ID " << pthread_self() << " Team: " << args->team << ", I have found a spot in a car" << endl;
    fsync(STDOUT_FILENO);
    pthread_mutex_unlock(&cout_mutex);

    pthread_barrier_wait(&barrier);

    if (driver)
    {
        pthread_mutex_lock(&cout_mutex);
        cout << "Thread ID " << pthread_self() << " Team: " << args->team << ", I am the captain and driving the car with ID " << carID << endl;
        fsync(STDOUT_FILENO);
        pthread_mutex_unlock(&cout_mutex);

        // New Car Creation
        pthread_barrier_destroy(&barrier);
        pthread_barrier_init(&barrier, nullptr, 4);
        pthread_mutex_unlock(&lock);
        carID++;
    }

    delete args;
    return nullptr;
}

int main(int argc, char* argv[]){

    if (argc != 3)
    {
        cerr << "Usage: " << argv[0] << " <num_teamA> <num_teamB>" << endl;
        return 1;
    }

    int num_teamA = stoi(argv[1]);
    int num_teamB = stoi(argv[2]);

    if ((num_teamA % 2 != 0) || (num_teamB % 2 != 0) || ((num_teamA + num_teamB) % 4 != 0))
    {
        cerr << "Error: Invalid number of team members." << endl;
        return 1;
    }

    pthread_mutex_init(&lock, nullptr);
    pthread_mutex_init(&cout_mutex, nullptr);
    pthread_barrier_init(&barrier, nullptr, 4);

    sem_init(&sem_A, 0);
    sem_init(&sem_B, 0);

    vector<pthread_t> threads(num_teamA + num_teamB);

    // Even amount of threads are present.
    for (int i = 0; i < num_teamA + num_teamB; ++i)
    {
        auto *arg = new thread_args;
        arg->team = (i % 2 == 0) ? 'A' : 'B';
        pthread_create(&threads[i], nullptr, fan_threads, arg);
    }

    // Join all threads
    for (auto& thread : threads) {
        pthread_join(thread, nullptr);
    }

    pthread_barrier_destroy(&barrier);
    pthread_mutex_destroy(&lock);

    cout << "The main terminates" << endl;
    return 0;
}
