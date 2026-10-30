# Rucas

Rucas is a small esoteric programming language in the spirit of Brainfuck, but with readable assembly-style mnemonics. A program works on a tape of 30,000 byte-sized cells and a stack, and it has loops, `IF` blocks and console I/O.

Written in 2018. In 2026 it learned to multiply, divide and choose, and runs language models written in it by hand, a small neural network among them.

```
ADD 72
PRNTA
```

## The repo

| File | What it is |
| ---- | ---------- |
| [rucas.cpp](rucas.cpp) | The interpreter. A single C++ file with no dependencies. |
| [samples](samples) | Example programs from 2018. They run unchanged on `rucas.cpp`. |
| [showcase](showcase) | [names.ruc](showcase/names/names.ruc), a language model in Rucas, the names it learns from, and [names_optimised.ruc](showcase/names/names_optimised.ruc), the same model in fewer instructions. [lm_base](showcase/lm_base) is a base of functions for bigger models, and [names_network.ruc](showcase/names/names_network.ruc) and [names_attention.ruc](showcase/names/names_attention.ruc) are neural networks built on it. |
| [code_patterns](code_patterns) | Small programs that show the moves Rucas programs are built from, and why they work. |
| [main.cpp](main.cpp) | The original 2018 interpreter, kept as it was. It is Windows-only and has the bugs that `rucas.cpp` fixes. |
| [main.ruc](main.ruc) | The original 2018 scratch program. It contains two typos (`JMPRT` and `PRNT`) that the old interpreter silently accepted; `rucas.cpp` points them out. |

## Building and running

Any C++11 compiler works;

```bash
g++ -std=c++11 -O2 -o rucas rucas.cpp
```

On Windows without a compiler, the one that comes with Zig works;

```bash
python -m pip install ziglang
```

```bash
python -m ziglang c++ -std=c++11 -O2 -o rucas.exe rucas.cpp
```

```bash
./rucas samples/hello_world.ruc
```

Input for `CINP` is read from stdin, so you can pipe it in;

```bash
echo 1 | ./rucas samples/truth_machine.ruc
```

Or give it as a file after the program;

```bash
./rucas showcase/names/names.ruc showcase/names/names.txt
```

A program can be spread over several files. Give them in order and they run as one program; an input file still comes last.

```bash
./rucas showcase/lm_base/base.ruc showcase/lm_base/test.ruc
```

## The machine

- **Tape;** 30,000 cells, each holding a value from 0 to 255. Values wrap; 255 + 1 is 0, and 0 - 1 is 255.
- **Pointer;** points at the *current cell* and starts at cell 0. The tape is circular, so moving left from cell 0 lands on the last cell.
- **Stack;** an unbounded stack of values. Reading from an empty stack gives 0.

A program can give its cells 16 or 32 bits with a `BITS 16` or `BITS 32` line before its first instruction. With `BITS 32` a cell holds 0 to 4294967295, and stack values wrap the same way.

## Syntax

- One instruction per line, written as `OPERATION [ARGUMENT]`. Indentation is free.
- A `#` starts a comment, on a line of its own or after an instruction.
- Unknown instructions, bad arguments and unmatched `STLP`/`NDLP`, `IF`/`ELSE`/`ENDIF` are reported with their line number, before anything runs.

### Arguments

| Argument | Meaning |
| -------- | ------- |
| N | Any non-negative number |
| $CELL | The value of the current cell |
| $STACK | The value on top of the stack, or 0 if the stack is empty |
| (none) | Leave it out to use the default, which is 1 unless stated otherwise |

## Instructions

### Cell manipulation
| Operation | Arguments | Explanation |
| --------- | --------- | ----------- |
| ADD | (none) : N : $CELL : $STACK | Add the argument (default 1) to the current cell |
| SUB | (none) : N : $CELL : $STACK | Subtract the argument (default 1) from the current cell |

### Pointer manipulation
| Operation | Arguments | Explanation |
| --------- | --------- | ----------- |
| JMPR | (none) : N : $CELL : $STACK | Move the pointer the given number of cells (default 1) to the right |
| JMPL | (none) : N : $CELL : $STACK | Move the pointer the given number of cells (default 1) to the left |
| JMP | N : $CELL : $STACK | Move the pointer to that cell number (0 to 29999) |

### I/O
| Operation | Arguments | Explanation |
| --------- | --------- | ----------- |
| PRNTA | (none) : N : $CELL : $STACK | Print the current cell as an ASCII character. With an argument, print that many cells starting at the current one. The pointer does not move. |
| PRNTD | (none) : N : $CELL : $STACK | Print the current cell as a decimal number. With an argument, print that many cells separated by spaces. The pointer does not move. |
| CINP | (none) | Read one character of input into the current cell, or 0 at end of input |

### Control flow
| Operation | Arguments | Explanation |
| --------- | --------- | ----------- |
| STLP | (none) | Start a loop. If the current cell is 0, skip to after the matching `NDLP`. |
| NDLP | (none) | End of a loop. If the current cell is not 0, jump back to the start of the loop. |
| IF | (none) : N : $CELL : $STACK | Run the block up to the matching `ENDIF` only if the argument (default `$CELL`) is greater than 0 |
| IF EQL | N : $CELL : $STACK | Run the block up to the matching `ENDIF` only if the argument equals the current cell |
| IF LESS | N : $CELL : $STACK | Run the block only if the current cell is less than the argument |
| IF MORE | N : $CELL : $STACK | Run the block only if the current cell is more than the argument |
| ELSE | (none) | Inside an `IF` block; what runs when the `IF` does not |
| ENDIF | (none) | End of an `IF` block |
| HALT | (none) | Stop the program |

Loops and `IF` blocks can be nested.

### Stack manipulation
| Operation | Arguments | Explanation |
| --------- | --------- | ----------- |
| PSHS | (none) | Push the current cell's value onto the stack |
| CLRS | (none) | Clear the stack |
| RMVS | (none) | Remove the bottom-most value of the stack |
| EQL TOPS | (none) | Set the current cell to the value on top of the stack |

### Stack arithmetic
| Operation | Arguments | Explanation |
| --------- | --------- | ----------- |
| ADDS | (none) | Push (top + second-from-top) onto the stack, wrapped to the cell size |
| SUBS | (none) | Push (top - second-from-top) onto the stack, wrapped to the cell size |
| MULS | (none) | Push (top × second-from-top) onto the stack, wrapped to the cell size |
| DIVS | (none) | Push (top ÷ second-from-top) onto the stack, rounded down. Dividing by 0 stops the program with an error. |
| MODS | (none) | Push the remainder of (top ÷ second-from-top) onto the stack |

For `SUBS`, `DIVS` and `MODS`, push the second value first; with 2 pushed and then 7, `DIVS` pushes 7 ÷ 2 = 3.

## Examples

### Print `hi`

```
ADD 104
JMPR
ADD 105
JMPL
PRNTA 2
```

### Answer a yes

Reads one character and prints `Y` if it was a `y` (ASCII 121);

```
CINP
IF EQL 121
	JMPR
	ADD 89
	PRNTA
	JMPL
ENDIF
```

### Fibonacci

Prints `0 1 1 2 3 5 8 13 21 34 55 89 144 233`, every Fibonacci number that fits in a cell. It keeps `a` and `b` in two cells and uses the stack to compute the next pair.

```
# Cells; 0 = counter, 1 = a, 2 = b, 3 = space
ADD 14
JMPR 2
ADD 1
JMPR
ADD 32
JMPL 3
STLP
	JMPR
	PRNTD
	JMPR 2
	PRNTA
	JMPL 2
	CLRS
	PSHS
	JMPR
	ADD $STACK
	PSHS
	SUBS
	JMPL
	EQL TOPS
	JMPL
	SUB
NDLP
```

### A language model

[names.ruc](showcase/names/names.ruc) reads a list of names and counts which letter follows which; that counting is its training. Then it writes 20 new names, picking every next letter at random in proportion to those counts. It keeps the counts in a table of 27 × 27 cells and finds a cell with `JMP $STACK`, picks with random numbers made by `MULS`, `DIVS` and `MODS`, and walks a row with `IF LESS` and `ELSE`.

```bash
./rucas showcase/names/names.ruc showcase/names/names.txt
```

```
reno
bienaconca
a
sedr
nnamelacidreriolian
```

To write other names, change the seed on the line `ADD 2018`, or give it a list of your own; one name per line, letters a to z.

[names_optimised.ruc](showcase/names/names_optimised.ruc) writes the same names in 109 instructions instead of 157. Cells 0 to 26 point at the rows of counts, so two `JMP $CELL` find a row without any arithmetic, and it walks around a row until the random number fits, so it needs no totals and no `MODS`.

```bash
./rucas showcase/names/names_optimised.ruc showcase/names/names.txt
```

### A base for bigger models

[lm_base](showcase/lm_base) gives Rucas functions without new instructions. The whole program runs in one loop. Cell 0 holds the number of the piece that runs next, every piece is an `IF EQL` block, and a call keeps the piece to come back to on a stack in cells 32 to 47. [base.ruc](showcase/lm_base/base.ruc) opens that loop and holds the functions, and a program's own file adds pieces 1 to 99 and closes the loop.

| Piece | Function | What it does |
| ----- | -------- | ------------ |
| 100 | FMUL | c = a × b |
| 110 | MATVEC | Line j of the matrix at a times the list at b goes to place j of the list at c |
| 120 | PRNT | Prints n numbers from a, one per line with four decimals |
| 130 | ALLOC | c = the first of n fresh cells |
| 150 | EXP | Turns each of n numbers from a into e to its power |
| 160 | TANH | Turns each of n numbers from a into its tanh |
| 165 | DTANH | Multiplies each of n numbers from a by 1 − b[i]², the slope of tanh |
| 170 | RAND | Fills n numbers from a with random numbers from b up to b + c |
| 180 | COPY | Copies n numbers from a to b |
| 190 | STEP | A learning step; weight i of line j in the matrix at c loses a[j] × b[i] ÷ cell 9 |
| 200 | SOFTMAX | Turns n numbers from a into chances that add up to 1.0 |
| 205 | DSOFTMAX | Turns n errors from a, on the chances SOFTMAX gave in b, into the errors on its scores |
| 210 | LOG | Turns each of n numbers from a into its natural log |

Numbers are signed, with 12 bits after the point, so 1.0 is 4096. A list is n numbers starting at a cell, a step apart; the step for a is in cell 6 and the step for b in cell 7, both 1 to start. A matrix is lines of n numbers, with cell 18 saying how many lines and cell 19 how far apart they start, so the same MATVEC reads rows or columns. Arguments go in cells 2 (a), 3 (b), 4 (c) and 5 (n), and a single result comes back in c. STEP divides by cell 9, which starts at 4096; 4096 ÷ rate makes it a learning step of that rate. RAND keeps its state in cell 31, starting at 2018, so every run comes out the same. Cells 48 to 95 are the program's own, and ALLOC hands out cells from 96. A call to FMUL that comes back to piece 2;

```
JMP 1
ADD
JMP $CELL
ADD 2
JMP 0
SUB $CELL
ADD 100
```

[test.ruc](showcase/lm_base/test.ruc) calls every function and prints what each one gives.

```bash
./rucas showcase/lm_base/base.ruc showcase/lm_base/test.ruc
```

### A network that learns names

[names_network.ruc](showcase/names/names_network.ruc) is a small neural network on top of lm_base. It learns from the same names which symbol comes after the 3 before it. Each of the 3 gets 8 learned numbers, its embedding; the 24 of them feed 32 tanh neurons, and those give a score for every symbol, which SOFTMAX turns into chances. That makes 1,907 weights. It learns one example at a time, from random places in the names, for 10 passes of 9,630 examples, and then writes 20 names. It takes under a minute.

```bash
./rucas showcase/lm_base/base.ruc showcase/names/names_network.ruc showcase/names/names.txt
```

It prints the average loss of every pass, then the names. The loss says how surprised the network is by the right symbol; the counts in names.ruc come to 2.35.

```
2.4938
2.4106
...
2.1296
bra
lebynn
rulah
ertey
ila
andel
haelani
cadeline
```

### A network with attention

[names_attention.ruc](showcase/names/names_attention.ruc) is that network with one attention head added, so besides the 3 symbols before the next one it also looks back over 8. Each of the 8 gets its embedding plus one for its place. The newest one asks a question, A times its embedding, and each of the 8 gets a score for how well it answers; SOFTMAX turns the scores into chances, and the 8 mixed by those chances go into the tanh neurons together with the 3 newest. That makes 2,291 weights. It learns at 0.02, then at 0.005 for the last 3 passes, and takes about a minute and a half.

```bash
./rucas showcase/lm_base/base.ruc showcase/names/names_attention.ruc showcase/names/names.txt
```

It ends at a loss of 2.06, where names_network.ruc ends at 2.13.

```
2.4580
2.3000
...
2.0610
maialey
mirk
belia
aliliah
lachley
rari
abylinn
julie
```

### Samples

| Sample | What it does |
| ------ | ------------ |
| [hello_world.ruc](samples/hello_world.ruc) | Prints `hello world` |
| [truth_machine.ruc](samples/truth_machine.ruc) | Reads `0` or `1`; prints `0` once, or `1` forever |
| [if_statement.ruc](samples/if_statement.ruc) | A basic `IF $CELL` block |
| [Fibonacci_sequence.ruc](samples/Fibonacci_sequence.ruc) | The 2018 take on Fibonacci; prints the numbers in pairs and keeps going after they wrap past 255 |
