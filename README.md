# Rucas
Rucas is a small little esolang which can be used for many operations

Usage is as follows:

### Cell manipulation
| Operation | Arguments | Explanation |
| ----------- | ----------- | ----------- |
| ADD | N | add n to current cell's value |
| SUB |  N | subtract n  off current cell's value |

### Pointer manipulation
| Operation | Arguments | Explanation |
| ----------- | ----------- | ----------- |
| JMPR | N | Move the pointer n cells to the right |
| JMPL |  N | Move the pointer n cells to the left |

### I/O 
| Operation | Arguments | Explanation |
| ----------- | ----------- | ----------- |
| PRNTA | N/A : N | PRINT ASCII value, If N is given, PRINT ASCII vallue of (N) cells |
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

