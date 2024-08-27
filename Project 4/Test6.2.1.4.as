        lw      0       1       cinco
        nor     0       0       3
        lw      0       2       nun
        add     0       0       4
        noop
        noop
for     beq     3       1       fin
        lw      2       6       nun
        nor     2       2       7
        nor     5       7       5
        nor     6       6       5
        noop
        beq     0       5       never
        add     2       4       4
        add     3       2       3
        beq     0       0       for
fin     halt
        noop
uno     .fill   1
nun     .fill   0
cinco   .fill   5
        .fill   0
        .fill   2
        .fill   4
        .fill   6