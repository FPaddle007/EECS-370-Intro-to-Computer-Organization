        lw          0       1       n
        lw          0       2       r
        lw          0       4       Caddr        load combination function address
        jalr        4       7                    call function
comb    lw          0       6       posOne       load register 4 with positive 1 & beginning of (n,r) combination                                    Callee save here
        sw          5       7       stack        Save return address
        add         5       6       5            increment memory pointer
        sw          5       1       stack        save n in memory
        add         5       6       5            increment memory pointer
        add         1       1       1            compute 2*input
        add         1       1       3            compute 4* input into return value
        lw          0       6       negOne       r6 = -1
        add         5       6       5            decrement memory pointer
        lw          5       1       stack        recover original input
        add         5       6       5            decrement memory pointer
        lw          5       7       stack        recover original return address
        jalr        7       4                    return. register 4 is not restored
        beq         2       0       re1      base cases (r = 0) & (n = r)
        beq         1       2       re1
        lw          0       6       negOne       combination(n - 1, r)
        add         1       6       1
        lw          0       4       Caddr
        jalr        4       7 
        lw          0       6       negOne       r gets decremented
        add         2       6       2
        sw          5       3       stack        save on the stack
        lw          0       6       posOne
        add         5       6       5
        jalr        4       7                    second call combination(n - 1, r - 1)
        lw          0       6       negOne       save stack result in register 4
        add         5       6       5
        lw          5       4       stack
        add         4       3       3            add and put into register 3      
re1     beq         0       0       res
        lw          0       6       posOne       return 1
        add         0       6       3
        beq         0       0       res
res     lw          0       6       negOne      restore callee saved registers  
        add         5       6       5           decrement memory pointer
        lw          5       4       stack       restore register 4
        add         5       6       5           decrement memory pointer
        lw          5       2       stack       restore r
        add         5       6       5           decrement memory pointer
        lw          5       1       stack       restore n
        add         5       6       5           decrement memory pointer
        lw          5       7       stack       restore register 7
        noop                                    return to return address
        jalr        7       6
n       .fill       7
r       .fill       3
Caddr   .fill       comb
negOne  .fill       -1
posOne  .fill       1
Stack   .fill       0