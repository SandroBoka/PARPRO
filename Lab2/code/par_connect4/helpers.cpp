#include "helpers.h"
#include <iostream>

using namespace std;

// rekurzivna funkcija: ispituje sve moguce poteze i vraca ocjenu dobivenog
// stanja ploce Current: trenutno stanje ploce LastMover: HUMAN ili CPU
// iLastCol: stupac prethodnog poteza
// iDepth: dubina se smanjuje do 0
double Evaluate(Board Current, data LastMover, int iLastCol, int iDepth) {
    double dResult, dTotal;
    data NewMover;
    bool allMovesLose = true, allMovesWin = true;
    int iMoves;

    if (Current.GameEnd(iLastCol)) {
        if (LastMover == CPU) {
            return 1; // pobjeda
        } else {
            // if(LastMover == HUMAN)
            return -1; // poraz
        }
    }

    if (iDepth == 0) {
        return 0; // dosli smo do najdublje razine
    }

    iDepth--;

    // tko je na potezu sljedeci
    if (LastMover == CPU) {
        NewMover = HUMAN;
    } else {
        NewMover = CPU;
    }

    dTotal = 0;
    iMoves = 0; // broj mogucih poteza u ovoj razini

    for (int iCol = 0; iCol < Current.Columns(); iCol++) {
        if (Current.MoveLegal(iCol)) {
            iMoves++;
            Current.Move(iCol, NewMover);
            dResult = Evaluate(Current, NewMover, iCol, iDepth);
            Current.UndoMove(iCol);
            if (dResult > -1) {
                allMovesLose = false;
            }
            if (dResult != 1) {
                allMovesWin = false;
            }
            if (dResult == 1 && NewMover == CPU) {
                return 1; // ako svojim potezom mogu doci do pobjede (pravilo 1)
            }
            if (dResult == -1 && NewMover == HUMAN) {
                return -1; // ako protivnik moze potezom doci do pobjede (pravilo 2)
            }

            dTotal += dResult;
        }
    }
    // ispitivanje za pravilo 3.
    if (allMovesWin == true) {
        return 1;
    }
    if (allMovesLose == true) {
        return -1;
    }

    dTotal /= iMoves; // dijelimo ocjenu s brojem mogucih poteza iz zadanog stanja
    return dTotal;
}

void PrintTask(const Task &task) {
    cout << "Task: firstMove=" << task.movesToPlay[0] << ", moveCount=" << task.moveCount
         << ", moves=";

    for (int i = 0; i < task.moveCount; i++) {
        cout << task.movesToPlay[i] << " ";
    }

    cout << ", lastMover=" << task.lastMover
         << ", lastMove=" << task.movesToPlay[task.moveCount - 1]
         << ", depthLeft=" << task.depthLeft << endl;
}

void GenerateTasks(Board &board, int depth, int splitDepth, int currentDepth, Task currentTask,
                   vector<Task> &tasks) {
    // ako smo dosegli max dubinu onda spremamo taj zadatak
    if (currentDepth == splitDepth) {
        currentTask.depthLeft = depth - currentDepth;
        currentTask.taskIndex = tasks.size();
        tasks.push_back(currentTask);
        return;
    }

    data player = (currentDepth % 2 == 0) ? CPU : HUMAN;

    for (int col = 0; col < board.Columns(); col++) {
        if (board.MoveLegal(col)) {
            board.Move(col, player);

            currentTask.movesToPlay[currentDepth] = col;
            currentTask.moveCount = currentDepth + 1;
            currentTask.lastMover = player;

            if (board.GameEnd(col)) {
                currentTask.depthLeft = depth - (currentDepth + 1);
                currentTask.taskIndex = tasks.size();
                tasks.push_back(currentTask);
            } else {
                GenerateTasks(board, depth, splitDepth, currentDepth + 1, currentTask, tasks);
            }

            board.UndoMove(col);
        }
    }
}

double ExecuteTask(Board board, const Task &task) {
    for (int i = 0; i < task.moveCount; i++) {
        data player = (i % 2 == 0) ? CPU : HUMAN;
        board.Move(task.movesToPlay[i], player);
    }

    int lastMove = task.movesToPlay[task.moveCount - 1];

    return Evaluate(board, task.lastMover, lastMove, task.depthLeft);
}

bool TaskMatchesPrefix(const Task &task, const vector<int> &prefix) {
    if (task.moveCount < prefix.size()) {
        return false;
    }

    for (int i = 0; i < prefix.size(); i++) {
        if (task.movesToPlay[i] != prefix[i]) {
            return false;
        }
    }

    return true;
}

bool PrefixExists(const vector<Task> &tasks, const vector<int> &prefix) {
    for (int i = 0; i < tasks.size(); i++) {
        if (TaskMatchesPrefix(tasks[i], prefix)) {
            return true;
        }
    }

    return false;
}

int FindExactTask(const vector<Task> &tasks, const vector<int> &prefix) {
    for (int i = 0; i < tasks.size(); i++) {
        if (tasks[i].moveCount == prefix.size() && TaskMatchesPrefix(tasks[i], prefix)) {
            return i;
        }
    }

    return -1;
}

double ReduceTaskResults(const vector<Task> &tasks, const vector<double> &taskResults,
                         const vector<int> &prefix, int columns) {
    int exactTask = FindExactTask(tasks, prefix);
    if (exactTask != -1) { // ako postoji stajemo tu
        return taskResults[exactTask];
    }

    data nextMover = (prefix.size() % 2 == 0) ? CPU : HUMAN;

    bool allMovesLose = true;
    bool allMovesWin = true;
    double total = 0;
    int moveCount = 0;

    for (int col = 0; col < columns; col++) {
        vector<int> childPrefix = prefix;
        childPrefix.push_back(col);

        if (!PrefixExists(tasks, childPrefix)) {
            continue; // ako ne postoji preskocimo
        }

        double result = ReduceTaskResults(tasks, taskResults, childPrefix, columns);

        moveCount++;

        if (result > -1) {
            allMovesLose = false;
        }

        if (result != 1) {
            allMovesWin = false;
        }

        if (result == 1 && nextMover == CPU) {
            return 1;
        }

        if (result == -1 && nextMover == HUMAN) {
            return -1;
        }

        total += result;
    }

    if (allMovesWin) {
        return 1;
    }

    if (allMovesLose) {
        return -1;
    }

    if (moveCount == 0) {
        return 0;
    }

    return total / moveCount;
}

int ChooseBestMove(const vector<Task> &tasks, const vector<double> &taskResults, int columns) {
    double best = -2;
    int bestMove = -1;

    for (int col = 0; col < columns; col++) {
        vector<int> prefix;
        prefix.push_back(col);

        if (!PrefixExists(tasks, prefix)) {
            continue;
        }

        double value = ReduceTaskResults(tasks, taskResults, prefix, columns);

        cout << "Stupac " << col << ", vrijednost: " << value << endl;

        if (bestMove == -1 || value > best) {
            best = value;
            bestMove = col;
        }
    }

    cout << "Najbolji: " << bestMove << ", vrijednost: " << best << endl;

    return bestMove;
}