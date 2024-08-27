        lw      0       3       cero
        lw      0       1       uno
        lw      0       2       dos
        noop
        sw      0       3       4
        noop
        lw      1       2       3
        noop
        halt
cero    .fill   0
uno     .fill   1
dos     .fill   4