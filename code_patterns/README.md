# Code patterns

The moves that Rucas programs are built from, each in a small program that runs on its own and prints what it did. The comments in the programs say why each move works, and this page puts the moves side by side.

```bash
./rucas code_patterns/cells.ruc
```

| Program | Shows | Prints |
| ------- | ----- | ------ |
| [cells.ruc](cells.ruc) | Copying, adding, doubling, negating, emptying and swapping cells | 7, 14, 28, 228, 0, 3 and 7 |
| [pointers.ruc](pointers.ruc) | Following pointers, and moving by a number known only while running | two and w |
| [numbers.ruc](numbers.ruc) | The order on the stack, numbers below 0, numbers with a point, random numbers | 3, -5, 4608, 54, 61 and 49 |
| [loops.ruc](loops.ruc) | Counting loops, walking until a 0, leaving a loop from inside | `***`, hi and 2 |
| [functions.ruc](functions.ruc) | A function called from two places | 9 and 25 |
| [recursion.ruc](recursion.ruc) | A function that calls itself | 120 |

## Cells

**Copy** a cell into another by way of the stack, since no instruction moves a value from one cell to another.

```
CLRS
JMP 1
PSHS
JMP 2
EQL TOPS
```

**Add** one cell to another the same way, with `ADD $STACK` in place of `EQL TOPS`.

**Double** a cell with `ADD $CELL`; the cell adds its own value.

**Empty** a cell with `SUB $CELL`; whatever it holds, taking that off itself leaves 0.

**Negate** a cell. `SUB $CELL` makes it 0, and `SUB $STACK` then takes the pushed value off that 0. In a cell of 8 bits −28 shows as 228, since the cell wraps around at 256.

```
CLRS
PSHS
SUB $CELL
SUB $STACK
```

**Swap** two cells. `EQL TOPS` only copies the top of the stack, and nothing takes the top off, so the value under it has to come back up. `ADDS` then `SUBS` does that, since `SUBS` gives the top minus the one under it, and (a + b) − a = b.

```
CLRS
JMP 2
PSHS
JMP 1
PSHS
JMP 2
EQL TOPS
ADDS
SUBS
JMP 1
EQL TOPS
```

## Pointers

A cell can hold the number of another cell, and `JMP $CELL` goes there.

**Follow a pointer** with `JMP 1` and then `JMP $CELL`. A second `JMP $CELL` follows the next one. In pointers.ruc cell 8 holds 2, cell 2 holds 60, and the word two starts at cell 60. That is how a word, or a row of a table, gets picked by its number.

**Move by a number known only while running** with `JMPR $STACK`. Push the number, go to the start, and move. `JMP $STACK` goes to the number itself.

## Numbers

**The order on the stack.** `SUBS`, `DIVS` and `MODS` work on the top and the value under it, top first. For 7 ÷ 2, push the 2 first and then the 7.

**Numbers below 0.** A cell has no minus. With `BITS 32` it wraps around at 4294967296, so 2 − 7 becomes 4294967291. Count everything above 2147483647 as below 0, and adding, subtracting and multiplying still come out right. `DIVS`, `MODS`, `PRNTD` and the `IF` tests know nothing about it, so they get the size, and the minus is handled by hand.

**Numbers with a point.** Store every number times 4096, so 1.5 is 6144 and 0.75 is 3072. Adding works as it is. A product is 4096 times too big, so divide it once by 4096; 6144 × 3072 ÷ 4096 is 4608, which is 1.125.

**Random numbers.** `state × 1103515245 + 12345` visits all 2^32 values before it repeats. Its low bits repeat much sooner, so divide by 65536 to use the top 16 bits, and `MODS` then gives a number below any limit.

## Loops

**A set number of times.** `STLP` and `NDLP` both stand on a counter that counts down to 0.

**Walk until a 0.** `NDLP` tests the cell the pointer stands on at that moment, not the cell the loop started on. Moving with `JMPR` inside the loop makes it stop at the first 0, like the end of a word.

**Leave a loop from inside.** End one branch on a cell that holds 0 and the other on a cell that does not, and `NDLP` stops or goes round to match. Such a loop starts on any cell that is not 0, so `STLP` lets it in.

## Functions

Rucas cannot jump to a line. The whole program goes in one loop instead, with cell 0 holding the number of the piece to run next. Every piece is an `IF EQL` on cell 0, and writing a number into cell 0 runs that piece; 0 ends the loop.

**Call** a function by writing on a stack on the tape which piece to come back to, then going to the function. A function cannot know who called it, so the note tells it.

```
JMP 1
ADD
JMP $CELL
ADD 2                   # come back to piece 2
JMP 0
SUB $CELL
ADD 50                  # go to piece 50
```

**Return** by taking the top note off the tape and going to the piece it names.

```
JMP 1
JMP $CELL
CLRS
PSHS
SUB $CELL
JMP 1
SUB
JMP 0
EQL TOPS
```

The notes live on the tape and not on the stack of `PSHS`, since nearly every sum starts with `CLRS`. The code after a call goes in a new piece, since a call ends the piece it is in.

**A function that calls itself** puts its own values on the tape stack before the call, under the return note, and takes them back in the piece the call comes back to. recursion.ruc works out 5! = 120 that way.

[lm_base](../showcase/lm_base) is built from these patterns.
