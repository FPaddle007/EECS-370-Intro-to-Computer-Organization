        lw      0       2       nun
        lw      0       4       negOne
        lw      0       7       maxNeg
        noop
        noop
        lw      0       1       uno
        lw      0       5       idk
        noop
        noop
        lw      0       6       hey
        lw      0       3       negTwo
for     beq     4       7       fin
        nor     5       3       1
        beq     0       7       huh
ehh     add     6       6       6
        add     3       4       3
        add     4       4       4
        noop
        beq     0       0       for
huh     add     2       6       2
        beq     0       0       ehh
fin     halt
        noop
uno     .fill   1
nun     .fill   0
cinco   .fill   5
negOne  .fill   -1
maxNeg  .fill   -32768
idk     .fill   20
negTwo  .fill   -2
hey     .fill   15