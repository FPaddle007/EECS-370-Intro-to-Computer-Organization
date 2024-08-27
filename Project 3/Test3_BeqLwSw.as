        lw      0       1       uno
        lw      0       2       dos
        beq     2       3       fin
        lw      0       3       idk
        sw      0       3       10
        noop
        noop
        noop
        lw      0       3       2
        noop
        noop
        noop
        beq     2       3       fin
        noop
fin     halt
uno     .fill   1
dos     .fill   2
idk     .fill   12