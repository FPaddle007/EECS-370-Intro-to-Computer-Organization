        lw      0       1       uno
        lw      0       2       dos
        add     1       2       6
        add     1       6       7
        noop
        noop
        add     7       7       7
        lw      0       3       uno
        nor     1       7       6
        halt
uno     .fill   1
dos     .fill   2