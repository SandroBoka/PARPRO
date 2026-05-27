#include "helpers.h"

#include <cstdlib>
#include <iostream>
#include <mpi.h>
#include <vector>

using namespace std;

const int DEPTH = 7;

const int TAG_TASK = 1;
const int TAG_RESULT = 2;
const int TAG_STOP = 3;

void WorkerLoop(const char *boardFile);

void RunTaskAsMaster(Board &board, const vector<Task> &tasks, vector<double> &taskResults,
                     int size);

void StopWorkers(int size);

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (argc < 2) {
        if (rank == 0) {
            cout << "Nisu predani svi argumenti" << endl;
        }

        MPI_Finalize();
        return 0;
    }

    Board B;

    if (rank == 0) {
        B.Load(argv[1]);
        Board emptyBoard(B.Rows(), B.Columns());
        emptyBoard.Save(argv[1]);
    }

    MPI_Barrier(MPI_COMM_WORLD); // cekanje na sve procese
    B.Load(argv[1]);

    int depth = DEPTH; // provjera prvog argumenta
    if (argc > 2) {
        depth = atoi(argv[2]);
    }

    int splitDepth = MAX_SPLIT_DEPTH; // provjera drugog argumenta
    if (argc > 3) {
        splitDepth = atoi(argv[3]);

        if (splitDepth > MAX_SPLIT_DEPTH) {
            splitDepth = MAX_SPLIT_DEPTH;
        }
    }

    if (splitDepth > depth) {
        splitDepth = depth;
    }

    if (rank == 0) {
        cout << "Master proces, ukupno procesa: " << size << endl;
        cout << "Dubina: " << depth << endl;
        cout << "Dubina dijeljenja " << splitDepth << endl;

        bool gameOver = false;

        PrintBoard(B);

        while (!gameOver) {
            int humanInput;
            cin >> humanInput;

            int humanMove = humanInput - 1;

            while (humanMove < 0 || humanMove >= B.Columns() || !B.MoveLegal(humanMove)) {
                cout << "Neispravan potez. Unesite ponovno" << endl;
                cin >> humanInput;
                humanMove = humanInput - 1;
            }

            B.Move(humanMove, HUMAN);
            B.Save(argv[1]);

            PrintBoard(B);

            if (B.GameEnd(humanMove)) {
                cout << "Igra zavrsena! Pobjeda igraca." << endl;
                gameOver = true;
                break;
            }

            vector<Task> tasks;
            vector<double> taskResults;

            Task startTask;
            startTask.moveCount = 0;
            startTask.depthLeft = depth;
            startTask.lastMover = EMPTY;
            startTask.taskIndex = -1;

            double startTime = MPI_Wtime();

            GenerateTasks(B, depth, splitDepth, 0, startTask, tasks);

            cout << "Broj zadataka: " << tasks.size() << endl;

            RunTaskAsMaster(B, tasks, taskResults, size); // podjela i skupljanje zadataka

            int bestMove = ChooseBestMove(tasks, taskResults, B.Columns());

            B.Move(bestMove, CPU);
            B.Save(argv[1]);

            double endTime = MPI_Wtime();

            cout << "Vrijeme racunanja: " << (endTime - startTime) << " sekundi" << endl;
            cout << "Racunalo je odigralo stupac " << bestMove + 1 << endl;

            PrintBoard(B);

            if (B.GameEnd(bestMove)) {
                cout << "Igra zavrsena! Pobjeda racunala." << endl;
                gameOver = true;
            }
        }

        StopWorkers(size);
    } else {
        WorkerLoop(argv[1]);
    }

    MPI_Finalize();
    return 0;
}

void WorkerLoop(const char *boardFile) {
    while (true) {
        Task task;
        MPI_Status status;

        MPI_Recv(&task, sizeof(Task), MPI_BYTE, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        if (status.MPI_TAG == TAG_STOP) {
            break;
        }

        Result result;
        Board board;
        board.Load(boardFile);

        result.value = ExecuteTask(board, task);
        result.taskIndex = task.taskIndex;

        MPI_Send(&result, sizeof(Result), MPI_BYTE, 0, TAG_RESULT, MPI_COMM_WORLD);
    }
}

void RunTaskAsMaster(Board &board, const vector<Task> &tasks, vector<double> &taskResults,
                     int size) {
    taskResults.assign(tasks.size(), 0.0);

    // ako je pokrenuto samo s 1 procesorom
    if (size == 1) {
        for (int i = 0; i < tasks.size(); i++) {
            taskResults[i] = ExecuteTask(board, tasks[i]);
        }

        return;
    }

    int nextTaskIndex = 0;
    int finishedTasks = 0;

    for (int worker = 1; worker < size; worker++) {
        // prva podjela zadataka
        if (nextTaskIndex < tasks.size()) {
            MPI_Send(&tasks[nextTaskIndex], sizeof(Task), MPI_BYTE, worker, TAG_TASK,
                     MPI_COMM_WORLD);

            nextTaskIndex++;
        }
    }

    while (finishedTasks < tasks.size()) {
        Result result;
        MPI_Status status;

        MPI_Recv(&result, sizeof(Result), MPI_BYTE, MPI_ANY_SOURCE, TAG_RESULT, MPI_COMM_WORLD,
                 &status);

        int worker = status.MPI_SOURCE;

        taskResults[result.taskIndex] = result.value;
        finishedTasks++;

        // slanje zadataka workerima
        if (nextTaskIndex < tasks.size()) {
            MPI_Send(&tasks[nextTaskIndex], sizeof(Task), MPI_BYTE, worker, TAG_TASK,
                     MPI_COMM_WORLD);

            nextTaskIndex++;
        }
    }
}

void StopWorkers(int size) {
    Task dummyTask;

    for (int worker = 1; worker < size; worker++) {
        MPI_Send(&dummyTask, 0, MPI_BYTE, worker, TAG_STOP, MPI_COMM_WORLD);
    }
}
