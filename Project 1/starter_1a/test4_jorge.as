        lw      0       2       count    
        lw      0       3       negOne       
        lw      0       1       -32768       
        lw      0       4       one
while   beq     2       1       end
        add     4       4       4       i<<1
        add     2       3       2       count--
        beq     0       0       while   loop
end     halt
negOne  .fill   -1
count   .fill   10
one     .fill   1
