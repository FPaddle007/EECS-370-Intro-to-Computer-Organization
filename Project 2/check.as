        lw      0       1       nIn
        lw      0       2       rIn
        lw      0       6       fnAddr
        jalr    6       7       // Jumps to the fn address stored in r6, r7 saves the return address
        halt                    // End of program, result should be in r3
        noop
        noop
fn      noop    Start of the fn comb(n,r)
        lw      0       6       pos1    // load r6 with +1
        noop   ////////////////////////////////////////////////////////////
        noop       Callee saving done here
        noop   ////////////////////////////////////////////////////////////
        sw      5       7       stack   // save the return address on the stack
        add     5       6       5       // increment stack pointer
        sw      5       1       stack   // save n input on the stack
        add     5       6       5       // increment stack pointer
        sw      5       2       stack   // save r input on the stack
        add     5       6       5       // increment stack pointer
        sw      5       4       stack   // save r4 local var on the stack
        add     5       6       5       // increment stack pointer
        noop   ////////////////////////////////////////////////////////////
        noop
        noop   //  Base Cases (r == 0), (n == r) return 1
        beq     2       0       return1
        beq     1       2       return1
        noop   //  Make a call to comb(n - 1, r)
        lw      0       6       neg1
        add     1       6       1
        lw      0       4       fnAddr
        jalr    4       7
        noop   // Decrement r
        lw      0       6       neg1
        add     2       6       2
        noop   // Save intermediate result on the stack
        sw      5       3       stack
        lw      0       6       pos1
        add     5       6       5
        noop   // Make second fn call comb(n - 1, r - 1)
        jalr    4       7
        noop   // Restore intermediate result on stack into r4
        lw      0       6       neg1
        add     5       6       5
        lw      5       4       stack   
        noop   // Add results together and put it in r3, then restore registers
        add     4       3       3
        beq     0       0       restore
return1 noop   // Returning the result of 1
        lw      0       6       pos1
        add     0       6       3
        beq     0       0       restore
restore noop   // Restoring all callee saved registers
        lw      0       6       neg1
        add     5       6       5       // decrement stack pointer
        lw      5       4       stack   // restore r4 local var
        add     5       6       5       // decrement stack pointer
        lw      5       2       stack   // restore r input
        add     5       6       5       // decrement stack pointer
        lw      5       1       stack   // restore n input
        add     5       6       5       // decrement stack pointer
        lw      5       7       stack   // restore r7 return address
        noop   // Jump back to the return address, ignore PC + 1 stored in r6
        jalr    7       6               
        noop
        noop
nIn     .fill   1
rIn     .fill   1
fnAddr  .fill   fn
pos1    .fill   1
neg1    .fill   -1
stack   .fill   0
stack1  .fill   0
stack2  .fill   0
stack3  .fill   0
stack4  .fill   0
stack5  .fill   0
stack6  .fill   0
stack7  .fill   0
stack8  .fill   0
stack9  .fill   0
stack10 .fill   0
stack11 .fill   0
stack12 .fill   0