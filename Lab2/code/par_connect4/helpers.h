#ifndef HELPERS_H
#define HELPERS_H

#include "board.h"
#include <vector>

using namespace std;

const int MAX_SPLIT_DEPTH = 3;

struct Task {
    int moveCount;
    int movesToPlay[MAX_SPLIT_DEPTH];
    int depthLeft;
    int lastMover;
    int taskIndex;
};

struct Result {
    int taskIndex;
    double value;
};

double Evaluate(Board Current, data LastMover, int iLastCol, int iDepth);

void PrintTask(const Task &task);

void GenerateTasks(Board &board, int depth, int splitDepth, int currentDepth, Task currentTask,
                   vector<Task> &tasks);

double ExecuteTask(Board board, const Task &task);

void PrintBoard(Board &board);

bool TaskMatchesPrefix(const Task &task, const vector<int> &prefix);

bool PrefixExists(const vector<Task> &tasks, const vector<int> &prefix);

int FindExactTask(const vector<Task> &tasks, const vector<int> &prefix);

double ReduceTaskResults(const vector<Task> &tasks, const vector<double> &taskResults,
                         const vector<int> &prefix, int columns);

int ChooseBestMove(const vector<Task> &tasks, const vector<double> &taskResults, int columns);

#endif
