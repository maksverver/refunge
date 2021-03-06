A program is represented in a two-dimensional data field that is unbounded at
the bottom but has a fixed width. Every cell in the field is an 8 bit register
that stores values from 0 to 255 (inclusive). Values wrap around on overflow,
so for example 250 + 20 = 14 and 12 - 34 = 234.

The data field stores both the program to be executed and the data that may be
used during execution. All registers not explicitly initialized before starting
the program are set to zero. Locations in the data field may be referenced by
a pair of (row,column) where row 0 is the topmost row, and column 0 is the
leftmost column. The width of the data field is fixed when loading a program.
The left and right edge of the field are connected: when a pointer moves over
the left edge it reappears on the right side of the field, and vice versa.

A program may be presented in source code form, which is an 8-bit encoding
where the lowest 128 values corresponds to the ASCII character set. Lines in
the file are seperated by the ASCII line feed character (10). The width of the
data field is equal to the length of the longest line in the input. Every line
in the source file is copied to the corresponding row in the data field, with
uninitialized bytes at the end of each line set to zero.

The state of a running program is defined by the data field and a collection
of one or more cursors, each cursor having the following attributes:

    (attribute)                     (possible values)           (initial value)
    ------------------------------- --------------------------- ---------------
    Instruction pointer location    (0..width,0..infinity)      (0,0)
    Instruction pointer direction   up, down, left or right     right
    Data pointer location           (0..width,0..infinity)      (0,0)
    Data mode                       none, add, subtract,        none
                                    input or output

When the program starts, it has a single cursor in the upperleft position
(as described by the initial values in the table above). The program ends when
there are no more cursors left.

The program executes in a series of discrete steps. At each step, each cursor
evaluates the character in the cell referenced by the instruction pointer. If
it is a valid instruction (as described below) the corresponding action is
taken. Afterwards, the instruction pointer of each cursor is moved one step
in the current direction. Characters that are not part of the instruction set
are ignored; i.e. no action is taken when they are encountered and the
instruction pointer moves over them normally.

A cursor is removed at the end of an execution step, if:
 - its data pointer moved off the top of the field, or
 - its instruction pointer moved off the top of the field, or
 - its instruction pointer moved off the bottom of the field.
The field is actually unbounded at the bottom. For the purpose of removing
instruction pointers, the bottom of the field is defined to be directly under
the lowest row in the data field that has been initialized or that has ever
been visited by a data pointer, whichever is lower.

The following instructions are available:

    ~ + - ? !
            Set data mode to "none", "add", "subtract", "input" or "output"
            respectively.

    > v < ^ X
            Move the data pointer to the right, bottom, left or top or keep
            the data pointer at the current location. In all cases, a data
            operation will be perfomed depending on the current data mode
            of the cursor.

                (data mode) (effect)
                ----------- ------------------------------------------------
                none        none
                add         add source cell to destination cell
                subtract    subtract source cell from destination cell
                input       read a byte into the destination cell
                output      write the byte in the source cell

            With the "X" instruction, a single cell acts as both source and
            destination cell.

            During a single execution step, at most one byte of input is read.
            All read operations result in the same value being assigned,
            possibly to different cells. If a read error occurs or end of file
            has been reached, no values are assigned and the destination cell
            retains its old value.

            In a single step at most one byte of output can be written. If two
            or more cursors write the same value, this value is written only
            once. If two or more cursors try to write different values, no
            write occurs.

    / \ |   Mirror.
            Change the instruction pointer direction, depending on the current
            direction, as described below:

                (old)    /       \       |
                -------- ------- ------- -------
                up       right   left    down
                down     left    right   up
                left     down    up      right
                right    up      down    left

    # @     Jump and conditional jump.
            The first form jumps over the next character in the current
            direction. The second form jumps only if the cell referenced by
            the data pointer contains a zero, and moves as normal otherwise.

    Y       Fork.
            Duplicate the cursor (including data pointer and data mode).
            The two cursors each take a different direction, depending
            on the direction of the instruction pointer before the fork:

                (old)   (new 1) (new 2)
                ------- ------- -------
                up      right   left
                down    left    right
                left    up      down
                right   down    up

When more than one cursor tries to write to the same data cell in a single
execution step, additions and subtractions are combined as expected. Input
operations occur before additions and subtractions, so if a field is (for
example) added to by one cursor and read into by another, then the final value
will be the sum of the byte read and the value added.
