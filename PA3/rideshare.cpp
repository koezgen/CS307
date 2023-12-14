#include <iostream>
#include <pthread.h>
#include <string>
#include <semaphore.h>
#include <vector>
using namespace std;

pthread_barrier_t initialization_barrier;
pthread_mutex_t cout_mutex;

typedef struct car_ {
    sem_t seats;
} car;

// Fan thread function for team A
void* team_a_thread(void* arg) {
    car* automobile = static_cast<car*>(arg);
    pthread_t threadID = pthread_self();

    pthread_barrier_wait(&initialization_barrier);

    sem_wait(&automobile->seats); // Acquire a seat
    pthread_mutex_lock(&cout_mutex); // Lock the mutex
    cout << "Team A thread " << threadID << " got a seat." << endl;
    pthread_mutex_unlock(&cout_mutex); // Unlock the mutex
    // Perform some actions here
    sem_post(&automobile->seats); // Release the seat

    return nullptr;
}

// Fan thread function for team B
void* team_b_thread(void* arg) {
    car* automobile = static_cast<car*>(arg);
    pthread_t threadID = pthread_self();

    pthread_barrier_wait(&initialization_barrier);

    sem_wait(&automobile->seats); // Acquire a seat
    pthread_mutex_lock(&cout_mutex); // Lock the mutex
    cout << "Team B thread " << threadID << " got a seat." << endl;
    pthread_mutex_unlock(&cout_mutex); // Unlock the mutex
    sem_post(&automobile->seats); // Release the seat

    return nullptr;
}

int main(int argc, char *argv[]) {
    int teamAcnt = 0;
    int teamBcnt = 0;

    if (argc < 3) {
        cerr << "Error: Not enough arguments provided.\n";
        return 1;
    }

    try {
        teamAcnt = stoi(argv[1]);
        teamBcnt = stoi(argv[2]);

        if ((teamAcnt % 2 != 0) || (teamBcnt % 2 != 0) || ((teamAcnt + teamBcnt) % 4 != 0)) {
            throw runtime_error("Error: Invalid amount of team members.");
        }

    } catch (const invalid_argument& e) {
        cerr << "Invalid argument: non-integer value provided.\n";
        return 1;
    } catch (const out_of_range& e) {
        cerr << "Invalid argument: value out of range for an integer.\n";
        return 1;
    } catch (const runtime_error& e) {
        cerr << e.what() << endl;
        return 1;
    }

    pthread_barrier_init(&initialization_barrier, nullptr, teamBcnt+teamAcnt);

    vector<car> cars((teamAcnt + teamBcnt) / 4);
    vector<pthread_t> team_a_supporters(teamAcnt);
    vector<pthread_t> team_b_supporters(teamBcnt);

    for (auto &automobile : cars) {
        sem_init(&automobile.seats, 0, 4);
    }

    // Create threads for team A and team B
    for (int i = 0; i < teamAcnt; ++i) {
        pthread_create(&team_a_supporters[i], nullptr, team_a_thread, (void*)&cars[i % cars.size()]);
    }

    for (int i = 0; i < teamBcnt; ++i) {
        pthread_create(&team_b_supporters[i], nullptr, team_b_thread, (void*)&cars[i % cars.size()]);
    }

    // Destroy the barrier
    pthread_barrier_destroy(&initialization_barrier);

    // Destroy the semaphores
    for (auto &automobile : cars) {
        sem_destroy(&automobile.seats);
    }

    // Join threads
    for (auto& thread : team_a_supporters) {
        pthread_join(thread, nullptr);
    }

    for (auto& thread : team_b_supporters) {
        pthread_join(thread, nullptr);
    }

    return 0;
}
