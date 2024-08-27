        lw      0   3   mcand   put mcand in reg 4
        lw      0   2   mplier  put mplier in reg 3
        lw      0   4   uno     mask
        lw      0   6   neg1    will use to decrement 15
        lw      0   7   peak    max num of bits
        nor     2   2   2       nor 7048 to get inverse (-7048) and load into reg 2
for     beq     0   7   fin     check to see if reg 7 = 0, and if so, branch to fin
        nor     4   4   5       nor 1 to get inverse (-1) and load into reg 5
        nor     2   5   5       nor (-7048 and -1 together) to get AND gate between the two
        beq     0   5   jump    if value in reg 2 is equal 0, JUMP to add
        add     1   3   1       add mcand (left shifted) (load into reg 1)
jump    add     3   3   3       left shift bit by adding reg to itself (mcand)
        add     4   4   4       left shift bit by adding reg to itself (masked bit)
        add     7   6   7       decrement -1 from reg 7 (15) till you get 0
        beq     0   0   for     once it hits 0, exit loop
        noop
fin     halt
mcand   .fill   1103            fill 1103 into mcand (reg 3)
mplier  .fill   7043            fill 7048 into mplier (reg 2)
uno     .fill   1               masked bit - 1111 to 1110 (nor) then shift left by 1
neg1    .fill   -1              fill -1 into reg 6
peak    .fill   15              15 bit calculation