# Rucas
Rucas is a small little esolang which can be used for many operations

Usage is as follows:

### Arguments
| Arguments | Explanation |
| --------- | ----------- |
| N | N is any number you want |
| $CELL | $CELL is the value of the current cell |
| $STACK | $STACK is the value of the top cell of the stack, if stack is empty, value is 0 |
| N/A | N/A You don't have to enter anything at all |

### Cell manipulation
| Operation | Arguments | Explanation |
| ----------- | ----------- | ----------- |
| ADD | N/A : N : $ CELL | add (1 / N / $CELL) to current cell's value |
| SUB | N/A : N : $ CELL | subtract (1 / N / $CELL)  off current cell's value |

### Pointer manipulation
| Operation | Arguments | Explanation |
| ----------- | ----------- | ----------- |
| JMPR | N/A : N : $ CELL | Move the pointer (1 / N / $CELL) cells to the right |
| JMPL | N/A : N : $ CELL | Move the pointer (1 / N / $CELL) cells to the left |
| JMP | N : $ CELL | Jump to cell (N / $CELL) |

### I/O 
| Operation | Arguments | Explanation |
| ----------- | ----------- | ----------- |
| PRNTA | N/A : N : $CELL | PRINT ASCII value, If (N / $CELL) is given, PRINT ASCII vallue of (N / $CELL) cells |
| PRNTR | N/A | PRINT RAW value, If N is given, PRINT RAW vallue of (N) cells  |
| CINP |  N/A | Copy input to current cell we are on |

### LOOP
| Operation | Arguments | Explanation |
| ----------- | ----------- | ----------- |
| STLP | N/A | Start a loop if the current cells value isn't 0  |
| NDLP | N/A | End a loop if the current cell value is 0 |


### Stack manipulation
| Operation | Arguments | Explanation |
| ----------- | ----------- | ----------- |
| PSHS | N/A | Push current cells value to the top of the stack  |
| CLRS | N/A | Clear the stack |
| RMVS | N/A | Remove the bottom most value of the stack |
| EQL TOPS | N/A | Set the value of the element on top of the stack to the current cell |

### Mathematics
| Operation | Arguments | Explanation |
| ----------- | ----------- | ----------- |
| ADDS | N/A | Add the top two values on the stack together and push this value on top of the stack, If the value > 255 it will result in a int overflow  |
| SUBS | N/A | Subtract the second-top value from the top value on the stack and push this value on top of the stack, If the value < 0 it will result in a int overflow   |

