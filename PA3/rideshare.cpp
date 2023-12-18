#include <iostream>
#include <pthread.h>
#include <vector>
#include <semaphore.h>
using namespace std;

pthread_mutex_t cout_mutex;
sem_t teamA_sem, teamB_sem; // Semaphores for team A and B
int teamA_count = 0, teamB_count = 0; // Counters for team members
int carID = 0; // Car ID assignment

typedef struct car_ 
{
    unsigned long int carID = 0;
    sem_t seats;
    int teamA_count;
    pthread_mutex_t team_a_increment;
    int teamB_count;
    pthread_mutex_t team_b_increment;
} car ;

void* teamA_thread(void* arg) 
{
    vector<car>* cars = (vector<car>*)arg;

    pthread_mutex_lock(&cout_mutex);
    cout << "Thread ID: " << pthread_self() << ", Team: A, I am looking for a car." << endl;
    pthread_mutex_unlock(&cout_mutex);

    for (auto& c : *cars) {
        if (sem_trywait(&c.seats) == 0) 
        {
            pthread_mutex_lock(&c.team_a_increment);
            if (c.teamB_count == 0 || c.teamA_count >= c.teamB_count) 
            {
                c.teamA_count++;
                if (c.teamA_count + c.teamB_count = 4)
                {
                    cout << "Thread ID: " << pthread_self() << ", Team: A, I have found a spot in a car with ID " << c.carID << endl;
                    cout << "Thread ID: " << pthread_self() << ", Team: A, I am the captain and driving the car" << endl;
                }
                cout << "Thread ID: " << pthread_self() << ", Team: A, I have found a spot in a car" << endl;
                pthread_mutex_unlock(&c.team_a_increment);
                break;
            }
            pthread_mutex_unlock(&c.team_a_increment);
            sem_post(&c.seats);
        }
    }

    return nullptr;
}

void* teamB_thread(void* arg) 
{
    vector<car>* cars = (vector<car>*)arg;

    pthread_mutex_lock(&cout_mutex);
    cout << "Thread ID: " << pthread_self() << ", Team: B, I am looking for a car." << endl;
    pthread_mutex_unlock(&cout_mutex);

    for (auto& c : *cars) {
        if (sem_trywait(&c.seats) == 0) 
        {
            pthread_mutex_lock(&c.team_b_increment);
            if (c.teamA_count == 0 || c.teamB_count > c.teamA_count) 
            {
                c.teamB_count++;
                if (c.teamA_count + c.teamB_count == 4)
                {
                    pthread_mutex_lock(&cout_mutex);
                    cout << "Thread ID: " << pthread_self() << ", Team: B, I have found a spot in a car with ID " << c.carID << endl;
                    cout << "Thread ID: " << pthread_self() << ", Team: B, I am the captain and driving the car" << endl;
                    pthread_mutex_unlock(&cout_mutex);
                }
                else
                {
                    pthread_mutex_lock(&cout_mutex);
                    cout << "Thread ID: " << pthread_self() << ", Team: B, I have found a spot in a car" << endl;
                    pthread_mutex_unlock(&cout_mutex);
                }
                pthread_mutex_unlock(&c.team_b_increment);
                break;
            }
            pthread_mutex_unlock(&c.team_b_increment);
            sem_post(&c.seats);
        }
    }

    return nullptr;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <num_teamA> <num_teamB>" << endl;
        return 1;
    }

    int num_teamA = stoi(argv[1]);
    int num_teamB = stoi(argv[2]);

    if ((num_teamA % 2 != 0) || (num_teamB % 2 != 0) || ((num_teamA + num_teamB) % 4 != 0)) {
        cerr << "Error: Invalid number of team members." << endl;
        return 1;
    }
    
    // Car Array
    int car_count = (num_teamA + num_teamB) / 4;
    vector<car> cars(car_count);

    // Initialize mutex and semaphores
    pthread_mutex_init(&cout_mutex, nullptr);

    for (int i = 0; i < car_count; i++) {
        cars[i].carID = i;
        sem_init(&(cars[i].seats), 0, 4); // Initialize semaphore with 4 seats per car
        pthread_mutex_init(&(cars[i].team_a_increment), nullptr);
        pthread_mutex_init(&(cars[i].team_b_increment), nullptr);
    }

    // Vectors for threads
    vector<pthread_t> teamA_threads(num_teamA);
    vector<pthread_t> teamB_threads(num_teamB);

    // Create threads for Team A and Team B
    for (int i = 0; i < num_teamA; ++i) {
        pthread_create(&teamA_threads[i], nullptr, teamA_thread, (void*)&cars);
    }
    for (int i = 0; i < num_teamB; ++i) {
        pthread_create(&teamB_threads[i], nullptr, teamB_thread, (void*)&cars);
    }

    // Join all threads
    for (auto& thread : teamA_threads) {
        pthread_join(thread, nullptr);
    }
    for (auto& thread : teamB_threads) {
        pthread_join(thread, nullptr);
    }

    // Cleanup: Destroy semaphores and mutexes
    for (auto& c : cars) {
        sem_destroy(&c.seats);
        pthread_mutex_destroy(&c.team_a_increment);
        pthread_mutex_destroy(&c.team_b_increment);
    }
    pthread_mutex_destroy(&cout_mutex);

    cout << "The main terminates" << endl;
    return 0;
}