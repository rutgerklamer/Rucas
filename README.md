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
| ADD | N/A : N : $CELL : $STACK | add (1 : N : $CELL : $STACK) to current cell's value |
| SUB | N/A : N : $CELL : $STACK | subtract (1 : N : $CELL : $STACK)  off current cell's value |

### Pointer manipulation
| Operation | Arguments | Explanation |
| ----------- | ----------- | ----------- |
| JMPR | N/A : N : $CELL : $STACK | Move the pointer (1 : N : $CELL : $STACK) cells to the right |
| JMPL | N/A : N : $CELL : $STACK | Move the pointer (1 : N : $CELL : $STACK) cells to the left |
| JMP | N : $ CELL | Jump to cell (N / $CELL) |

### I/O 
| Operation | Arguments | Explanation |
| ----------- | ----------- | ----------- |
| PRNTA | N/A : N : $CELL : $STACK | PRINT ASCII value, If (N : $CELL : $STACK) is given, PRINT ASCII vallue of (N / $CELL) cells |
| PRNTD | N/A | PRINT Decimal value, If N is given, PRINT Decimal value of (N) cells  |
| CINP |  N/A | Copy input to current cell we are on |

### Statements
| Operation | Arguments | Explanation |
| ----------- | ----------- | ----------- |
| STLP | N/A | Start a loop if the current cells value isn't 0  |
| NDLP | N/A | End a loop if the current cell value is 0 |
| IF | $CELL : $STACK | Runs the code inside the statement if ($CELL: $STACK) > 0 |
| ENDIF | N/A | End if statement |


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

