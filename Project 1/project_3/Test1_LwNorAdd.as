        lw      0       1       uno
        lw      0       2       dos
        lw      0       3       tres
        nor     3       3       3
        add     1       3       3
        add     3       3       3
        noop
        add     3       3       4
        halt
uno     .fill   1
dos     .fill   4
tres    .fill   5