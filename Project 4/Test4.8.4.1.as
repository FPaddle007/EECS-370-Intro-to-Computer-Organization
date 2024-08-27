        lw      0       1       uno
        lw      0       2       nun
        lw      0       3       nun
        add     1       4       7
        nor     2       2       7
        add     7       7       7
        noop
for     beq     7       3       fin
        lw      0       4       tres
        add     4       2       4
        noop
        sw      0       4       tres
        add     3       1       3
        beq     0       0       for
fin     halt
        noop
uno     .fill   1
nun     .fill   0
tres    .fill   5