# Rucas v2

This is the fixed and documented version of Rucas. The [original 2018 version](..) is kept as it was in the root of the repo.

Rucas is a small esoteric programming language in the spirit of Brainfuck, but with readable assembly-style mnemonics. A program works on a tape of 30,000 byte-sized cells and a stack, and it has loops, `IF` blocks and console I/O. The interpreter is a single C++ file with no dependencies.

```
ADD 72
PRNTA
```

## Building and running

From this `v2` folder, any C++11 compiler works:

```bash
g++ -std=c++11 -O2 -o rucas main.cpp
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

Print `hi`:

```
ADD 104
JMPR
ADD 105
JMPL
PRNTA 2
```

The [samples](samples) folder has more:

| Sample | What it does |
| ------ | ------------ |
| [hello_world.ruc](samples/hello_world.ruc) | Prints `hello world` |
| [truth_machine.ruc](samples/truth_machine.ruc) | Reads `0` or `1`: prints `0` once, or `1` forever |
| [Fibonacci_sequence.ruc](samples/Fibonacci_sequence.ruc) | Prints the Fibonacci numbers up to 233 using the stack |
| [if_statement.ruc](samples/if_statement.ruc) | A basic `IF $CELL` block |
| [if_equal.ruc](samples/if_equal.ruc) | Reads a character and prints `Y` if it was `y` |
