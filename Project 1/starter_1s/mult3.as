        lw      0   2   mplier  put mplier in reg 1
        lw      0   3   mcand   put mcand in reg 2
        lw      0   4   uno     mask
        lw      0   5   peak    max num of bits
        noop
        beq     0   0   for     if reg 0 = 0, branch to begin for loop
for     nor     2   4   7       bit masking 0 to 1 (store in reg 7)
        beq     0   7   op      if reg 7 = 0, do op
        beq     0   0   oop     if reg 0 = 0, do oop
op      add     3   1   1       adds mcand (which is shifted left) to multiply
oop     lw      0   7   neg1    put -1 into reg 7 (temp register)
        add     5   7   5       sub 1 when loop happens
        beq     0   5   fin     stops if max bit is 0
        add     3   3   3       add register to itself by shifting left
        add     4   4   4       add register to itself by shifting left (bit masking)
        lw      0   7   pos1    put 1 into register 7 (temp register)
        add     4   7   4       add 1 to left shifted bit that's been masked
        beq     0   0   for     if reg 0 = 0, go back to beginning of for loop
fin     noop
        halt
mcand   .fill   1103            fill reg 3 with 1103
mplier  .fill   7048            fill reg 2 with 7048
uno     .fill   1              masked bit - 1111 to 1110 (nor) then shift left by 1
pos1    .fill   1               fill pos1 with 1
neg1    .fill   -1              fill neg1 with -1
peak    .fill   15              15 bit calculation




        nor     3   3   5       nor 1103 to get inverse (-1103 and load into reg 5)
        nor     5   5   2       nor -1103 to get inverse (1103) and store into reg 2