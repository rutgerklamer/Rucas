# Code patterns

The moves that Rucas programs are built from, each in a small program that runs on its own and prints what it did. The comments in the programs say why each move works, and this page puts the moves side by side.

```bash
./rucas code_patterns/cells.ruc
```

| Program | Shows | Prints |
| ------- | ----- | ------ |
| [cells.ruc](cells.ruc) | Copying, adding, doubling, negating, emptying and swapping cells | 7, 14, 28, 228, 0, 3 and 7 |
| [pointers.ruc](pointers.ruc) | Following pointers, and moving by a number known only while running | two and w |
| [numbers.ruc](numbers.ruc) | The order on the stack, numbers below 0, numbers with a point, random numbers, comparing numbers below 0 | 3, -5, 4608, 54, 61, 49, 0 and 1 |
| [points.ruc](points.ruc) | Multiplying numbers with a point below 0, printing them with four decimals | -1.1250 and 0.0625 |
| [loops.ruc](loops.ruc) | Counting loops, walking until a 0, leaving a loop from inside, the largest in a list | `***`, hi, 2 and 77 |
| [lists.ruc](lists.ruc) | One loop that sums a matrix by rows and by columns | 6 15 and 5 7 9 |
| [chances.ruc](chances.ruc) | Picking a letter by its count, rounding a division by chance | 24 letters, 19 of them c, and 173 |
| [functions.ruc](functions.ruc) | A function called from two places, and from a loop | 9, 25, 4, 9 and 16 |
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

**Comparing numbers below 0.** `IF LESS` and `IF MORE` see the size of a cell, so −5, stored as 4294967291, comes out above 3. Adding 2147483648 to both first flips their top bit, which moves every number below 0 under every number above it, and the comparison comes out right. Every number keeps its order, so this works for any two.

## Points

**Multiply below 0.** `MULS` wraps around, and wrapped to 32 bits a product of two numbers below 0, or of one below and one above, is still right. Only the division by 4096 goes wrong, since `DIVS` sees the size of the cell, so −1.5 × 0.75, stored as 4294967296 − 18874368, would come out as 254.875 and not −1.125. Make the product positive, divide it, and put the minus back.

**Print with four decimals.** Print a minus and go on with the size, then the whole part, the product ÷ 4096, and a point. What is left after the point is in 4096ths, so times 10000 ÷ 4096 turns it into ten-thousandths. `PRNTD` drops leading zeros, so 256, which is 0.0625, would print as 0.625; a 0 goes in front for every place the number is short of 1000.

## Loops

**A set number of times.** `STLP` and `NDLP` both stand on a counter that counts down to 0.

**Walk until a 0.** `NDLP` tests the cell the pointer stands on at that moment, not the cell the loop started on. Moving with `JMPR` inside the loop makes it stop at the first 0, like the end of a word.

**Leave a loop from inside.** End one branch on a cell that holds 0 and the other on a cell that does not, and `NDLP` stops or goes round to match. Such a loop starts on any cell that is not 0, so `STLP` lets it in.

**The largest in a list.** Keep the largest so far in a cell, push it, step onto the next number and test it with `IF MORE $STACK`; when the number is larger, it takes the place of the largest. A pointer in a cell walks the list, since the loop also jumps back to the cell holding the largest.

## Lists

**A list is numbers a step apart.** Keep where it starts, how many there are and the step in cells, and walk it with a pointer in a cell, `JMP $CELL`, and the step added each round.

**A matrix is lines of a list.** A cell more says how many lines there are, and another how far apart they start. Stored line after line, a matrix of 2 lines of 3 is read as rows with a step of 1 and lines 3 apart. Swap the two counts and the two steps, a step of 3 and lines 1 apart, and the same loop reads it as columns. lists.ruc sums both ways; a network runs forward on rows and learns on columns with one loop.

## Chances

**Pick by count.** Draw a number r below the total of the counts, then walk along them. While r is not inside the count it stands on, the count comes off r and the walk steps on. A letter with a count of 5 out of 8 holds 5 of the 8 values r can have, so it comes out 5 times in 8.

**Round by chance.** `DIVS` rounds down, so 7 ÷ 4 a hundred times adds up to 100 and not 175. With q the result and r what is left, take q + 1 with a chance of r ÷ 4. A random state below r × (4294967295 ÷ 4) comes r ÷ 4 of the time, so compare the next state with that once, and the sum comes out 175 on average. Small steps that `DIVS` would lose every time then count on average.

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

**A loop with a call in it** cannot use `STLP`, since the code after a call has to be a new piece. It runs through pieces instead. One piece checks whether anything is left and calls, the next one uses the result, steps on and goes back to the first. functions.ruc squares 2, 3 and 4 that way.

**A function that calls itself** puts its own values on the tape stack before the call, under the return note, and takes them back in the piece the call comes back to. recursion.ruc works out 5! = 120 that way.

[lm_base](../showcase/lm_base) is built from these patterns.
