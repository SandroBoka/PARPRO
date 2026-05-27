// Deklaracija razreda Board

#include <assert.h>
typedef int data;
const data EMPTY = 0;
const data CPU = 1;
const data HUMAN = 2;

class Board {
private:
  data **field;
  int *height;
  int rows, cols;
  data LastMover;
  int lastcol;
  void Take(); // zauzmi i popuni prazninama
  void Free();

public:
  // defaultni konstruktor
  Board() : rows(6), cols(7), LastMover(EMPTY), lastcol(-1) { Take(); }
  // parametrizirani konstruktor
  Board(const int row, const int col)
      : rows(row), cols(col), LastMover(EMPTY), lastcol(-1) {
    Take();
  }
  // copy konstruktor
  Board(const Board &src);

  ~Board() // destruktor
  {
    Free();
  }

  int Columns() // broj stupaca
  {
    return cols;
  }

  int Rows() // broj redaka
  {
    return rows;
  }

  data *operator[](const int row);
  bool MoveLegal(const int col);               // moze li potez u stupcu col
  bool Move(const int col, const data player); // napravi potez
  bool UndoMove(const int col);                // vrati potez iz stupca
  bool GameEnd(const int lastcol);             // je li zavrsno stanje
  bool Load(const char *fname);
  void Save(const char *fname);
};

// operator overload za []
inline data *Board::operator[](const int row) {
  assert(row >= 0 && row < rows);
  return field[row];
}
