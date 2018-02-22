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
| PRNTA | / | PRINT ASCII value |
| PRNTR | / | PRINT RAW value |
| CINP |  / | Copy input to current cell we are on |

### LOOP
| Operation | Arguments | Explanation |
| ----------- | ----------- | ----------- |
| STLP | / | Start a loop if the current cells value isn't 0  |
| NDLP | / | End a loop if the current cell value is 0 |


### Stack manipulation
| Operation | Arguments | Explanation |
| ----------- | ----------- | ----------- |
| PSHS | / | Push current cells value to the top of the stack  |
| CLRS | / | Clear the stack |
| RMVS | / | Remove the bottom most value of the stack |
| EQL TOPS | / | Set the value of the element on top of the stack to the current cell |

### Mathematics
| Operation | Arguments | Explanation |
| ----------- | ----------- | ----------- |
| ADDS | / | Add the top two values on the stack together and push this value on top of the stack, If the value > 255 it will result in a int overflow  |
| SUBS | / | Subtract the second-top value from the top value on the stack and push this value on top of the stack, If the value < 0 it will result in a int overflow   |

