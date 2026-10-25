# Rucas

Rucas is a small esoteric programming language in the spirit of Brainfuck, but with readable assembly-style mnemonics. A program works on a tape of 30,000 byte-sized cells and a stack, and it has loops, `IF` blocks and console I/O.

```
ADD 72
PRNTA
```

## The repo

| File | What it is |
| ---- | ---------- |
| [rucas.cpp](rucas.cpp) | The interpreter. A single C++ file with no dependencies. |
| [samples](samples) | Example programs from 2018. They run unchanged on `rucas.cpp`. |
| [main.cpp](main.cpp) | The original 2018 interpreter, kept as it was. It is Windows-only and has the bugs that `rucas.cpp` fixes. |
| [main.ruc](main.ruc) | The original 2018 scratch program. It contains two typos (`JMPRT` and `PRNT`) that the old interpreter silently accepted; `rucas.cpp` points them out. |

## Building and running

Any C++11 compiler works:

```bash
g++ -std=c++11 -O2 -o rucas rucas.cpp
```

```bash
./rucas samples/hello_world.ruc
```

Input for `CINP` is read from stdin, so you can pipe it in:

```bash
echo 1 | ./rucas samples/truth_machine.ruc
```

## The machine

- **Tape:** 30,000 cells, each holding a value from 0 to 255. Values wrap: 255 + 1 is 0, and 0 - 1 is 255.
- **Pointer:** points at the *current cell* and starts at cell 0. The tape is circular, so moving left from cell 0 lands on cell 29999.
- **Stack:** an unbounded stack of values. Reading from an empty stack gives 0.

## Syntax

- One instruction per line, written as `OPERATION [ARGUMENT]`. Indentation is free.
- Blank lines and lines starting with `#` are ignored, so you can use them for comments.
- Unknown instructions, bad arguments and unmatched `STLP`/`NDLP` or `IF`/`ENDIF` pairs are reported with their line number.

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
| CINP | (none) | Read one character from stdin into the current cell, or 0 at end of input |

### Control flow
| Operation | Arguments | Explanation |
| --------- | --------- | ----------- |
| STLP | (none) | Start a loop. If the current cell is 0, skip to after the matching `NDLP`. |
| NDLP | (none) | End of a loop. If the current cell is not 0, jump back to the start of the loop. |
| IF | (none) : N : $CELL : $STACK | Run the block up to the matching `ENDIF` only if the argument (default `$CELL`) is greater than 0 |
| IF EQL | N : $CELL : $STACK | Run the block up to the matching `ENDIF` only if the argument equals the current cell |
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
| ADDS | (none) | Push (top + second-from-top) onto the stack, wrapped to 0-255 |
| SUBS | (none) | Push (top - second-from-top) onto the stack, wrapped to 0-255 |

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

Reads one character and prints `Y` if it was a `y` (ASCII 121):

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
# Cells: 0 = counter, 1 = a, 2 = b, 3 = space
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

### Samples

| Sample | What it does |
| ------ | ------------ |
| [hello_world.ruc](samples/hello_world.ruc) | Prints `hello world` |
| [truth_machine.ruc](samples/truth_machine.ruc) | Reads `0` or `1`: prints `0` once, or `1` forever |
| [if_statement.ruc](samples/if_statement.ruc) | A basic `IF $CELL` block |
| [Fibonacci_sequence.ruc](samples/Fibonacci_sequence.ruc) | The 2018 take on Fibonacci: prints the numbers in pairs and keeps going after they wrap past 255 |
