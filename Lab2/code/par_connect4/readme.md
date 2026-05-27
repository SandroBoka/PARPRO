# Paralelni Connect4 s MPI

## Opis

Igra Connect4 s AI protivnikom.
Master proces generira zadatke pretrage i dinamicki ih raspodjeljuje worker procesima.
Svaki worker evaluira svoju granu stabla pretrage te vraca rezultat masteru.

## Kompajliranje

```bash
mpic++ connect4.cpp board.cpp helpers.cpp -o connect4
```

## Pokretanje

```bash
mpirun -np <broj_procesa> ./connect4 <datoteka_ploce> [dubina] [dubina_dijeljenja]
```

| Argument | Opis | Default |
|---|---|---|
| `<datoteka_ploce>` | putanja do `.txt` datoteke s pocetnim stanjem ploce (npr. `ploca.txt`) | — |
| `[dubina]` | dubina minimax pretrage | `7` |
| `[dubina_dijeljenja]` | dubina na kojoj se generiraju zadaci za paralelizaciju | `MAX_SPLIT_DEPTH=3` |

**Primjer:**

```bash
mpirun -np 4 ./connect4 ploca.txt 7 3
```

> Napomena: s `-np 1` program radi sekvencijalno (bez workera).

## Datoteke

| Datoteka | Opis |
|---|---|
| `connect4.cpp` | main program; MPI inicijalizacija, game loop, master/worker logika, `RunTaskAsMaster` (dinamicko rasporedivanje zadataka), `StopWorkers` |
| `helpers.h` | definicija struktura `Task` i `Result`, konstanta `MAX_SPLIT_DEPTH`, deklaracije pomocnih funkcija |
| `helpers.cpp` | `Evaluate` (minimax rekurzija), `GenerateTasks`, `ExecuteTask`, `ChooseBestMove`, `PrintBoard` |
| `board.h` | definicija klase `Board` (konstante: `EMPTY`, `CPU`, `HUMAN`) |
| `board.cpp` | implementacija klase `Board`: `Move`, `UndoMove`, `GameEnd`, `Load`, `Save` |

## Format datoteke ploce (`ploca.txt`)

```
<broj_redaka> <broj_stupaca>
<vrijednosti celija odvojene razmakom po retku>
```

Vrijednosti: `0` = prazno, `1` = CPU, `2` = igrac
