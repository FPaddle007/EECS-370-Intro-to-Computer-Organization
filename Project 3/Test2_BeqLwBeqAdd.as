        lw      0       1       uno
        lw      0       2       find
        lw      0       5       length
        noop
for     beq     5       3       fin
        lw      3       4       idk
        beq     2       4       fin
        add     3       1       3
        beq     0       0       for
        noop
        noop
fin     halt
uno     .fill   1
idk     .fill   10
        .fill   -1
        .fill   3
        .fill   18
        .fill   9
length  .fill   7
find    .fill   20