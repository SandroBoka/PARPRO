#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define LEFT 0
#define RIGHT 1
#define SLEEP_TIME 100000

#define TAG_REQUEST 2
#define TAG_FORK 3

struct Fork
{
    int isClean;
};

struct Philosopher
{
    struct Fork forks[2];
    int hasFork[2];
    int neighbours[2];
    int world_rank;
    int sentRequests[2];
    int receivedRequest[2];
};

const char* getForkSide(int side) {
    return side ? "desnu" : "lijevu";
}

int getRandomTime() {
    return (rand() % 10) + 1;
}

// HELPER FOR CALCULTING LEFT AND RIGHT NEIGHBOUR
void calculateNeighbours(struct Philosopher *p, int world_Size) {
    int left = (p->world_rank + world_Size - 1) % world_Size;
    int right = (p->world_rank + world_Size + 1) % world_Size;

    p->neighbours[LEFT] = left;
    p->neighbours[RIGHT] = right;
}

// THINKING
void think(struct Philosopher *p) {
    int thinkingTime = getRandomTime();

    printf("%*sFilozof %d misli %ds\n",  p->world_rank * 4, "", p->world_rank, thinkingTime); // identation

    for(int i = 0; i < thinkingTime * 10; i++) {
        usleep(SLEEP_TIME);

        int flag = 0;
        MPI_Status status;
        MPI_Iprobe(MPI_ANY_SOURCE, TAG_REQUEST, MPI_COMM_WORLD, &flag, &status);

        if (flag) {
            int requestedFork; // either LEFT or RIGHT
            MPI_Recv(&requestedFork, 1, MPI_INT, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            if (p->hasFork[requestedFork] && !p->forks[requestedFork].isClean) {
                p->hasFork[requestedFork] = 0;
                int neighboursLeftOrRight = !requestedFork;
                MPI_Send(&neighboursLeftOrRight, 1, MPI_INT, status.MPI_SOURCE, TAG_FORK, MPI_COMM_WORLD);
            } else {
                p->receivedRequest[requestedFork] = 1;
            }
        }
     }
}

// SEARCHING FOR FORKS
void getForks(struct Philosopher *p) {
    while (!p->hasFork[LEFT] || !p->hasFork[RIGHT]) {
        for (int i = 0; i < 2; i++) {
            if(!p->hasFork[i] && !p->sentRequests[i]) {
                printf("%*sFilozof %d trazi %s vilicu \n",  p->world_rank * 4, "", p->world_rank, getForkSide(i)); // identation
                
                int message = !i; // If we need left fork that is our neighbours right fork and vice versa

                p->sentRequests[i] = 1;
                MPI_Send(&message, 1, MPI_INT, p->neighbours[i], TAG_REQUEST, MPI_COMM_WORLD);
            }
        }

        for (int i = 0; i < 2; i++) {
            if (p->receivedRequest[i] && p->hasFork[i] && !p->forks[i].isClean) {
                p->hasFork[i] = 0;
                int neighboursLeftOrRight = !i;
                p->receivedRequest[i] = 0;
                printf("%*sFilozof %d salje vilicu filozofu %d\n",  p->world_rank * 4, "", p->world_rank, p->neighbours[i]);
                MPI_Send(&neighboursLeftOrRight, 1, MPI_INT, p->neighbours[i], TAG_FORK, MPI_COMM_WORLD);
            }
        }

        int message;
        MPI_Status status;

        MPI_Recv(&message, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        if (status.MPI_TAG == TAG_FORK) {
            printf("%*sFilozof %d prima %s vilicu od filozofa %d\n",  p->world_rank * 4, "", p->world_rank, getForkSide(message), status.MPI_SOURCE);
            p->hasFork[message] = 1;
            p->forks[message].isClean = 1;
            p->sentRequests[message] = 0;
        } else if (status.MPI_TAG == TAG_REQUEST) {
            if (p->hasFork[message] && !p->forks[message].isClean) {
                p->hasFork[message] = 0;
                int neighboursLeftOrRight = !message;

                MPI_Send(&neighboursLeftOrRight, 1, MPI_INT, status.MPI_SOURCE, TAG_FORK, MPI_COMM_WORLD);
            } else {
                p->receivedRequest[message] = 1;
            }
        }

    }
}

void eat(struct Philosopher *p) {
    int time = getRandomTime();
    printf("%*sFilozof %d jede %d sekundi\n",  p->world_rank * 4, "", p->world_rank, time);

    for (int i = 0; i < time * 10; i++) {
        usleep(SLEEP_TIME);

        int flag = 0;
        MPI_Status status;
        MPI_Iprobe(MPI_ANY_SOURCE, TAG_REQUEST, MPI_COMM_WORLD, &flag, &status);

         if (flag) {
            int requestedFork; // either LEFT or RIGHT
            MPI_Recv(&requestedFork, 1, MPI_INT, status.MPI_SOURCE, TAG_REQUEST, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
             p->receivedRequest[requestedFork] = 1;
         }
    }

    p->forks[0].isClean = 0;
    p->forks[1].isClean = 0;

    for (int i = 0; i < 2; i++) {
        if (p->receivedRequest[i]) {
            p->hasFork[i] = 0;
            p->receivedRequest[i] = 0;
            int neighboursLeftOrRight = !i;
            printf("%*sFilozof %d salje vilicu filozofu %d\n",  p->world_rank * 4, "", p->world_rank, p->neighbours[i]);
            MPI_Send(&neighboursLeftOrRight, 1, MPI_INT, p->neighbours[i], TAG_FORK, MPI_COMM_WORLD);
        }
    }
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    struct Philosopher philosopher = {0};

    int world_size;

    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &philosopher.world_rank);

    srand(time(NULL) + philosopher.world_rank);

    if (world_size < 2)
    {
        if (philosopher.world_rank == 0)
        {
            printf("Za ovaj problem trebamo barem 2 filozofa\n.");
        }

        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // inital conditons -> all forks dirty, first philosopher has both
    if (philosopher.world_rank != world_size - 1)
    {
        philosopher.hasFork[RIGHT] = 1;
    }
    if (philosopher.world_rank == 0)
    {
        philosopher.hasFork[LEFT] = 1;
    }

    calculateNeighbours(&philosopher, world_size);

    do {
        think(&philosopher);

        getForks(&philosopher);

        eat(&philosopher);
    } while (1);

    MPI_Finalize();
}