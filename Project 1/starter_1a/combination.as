        lw          0       1       n
        lw          0       2       r
        lw          0       4       Caddr        load combination function address
        jalr        4       7                    call function
        halt
comb    lw          0       6       posOne       load register 6 with positive 1 & beginning of (n,r) combination & Callee save here
        sw          5       7       Stack        store register 7
        add         5       6       5            increment memory pointer
        sw          5       1       Stack        store n
        add         5       6       5            increment memory pointer
        sw          5       2       Stack        store r
        add         5       6       5            increment memory pointer           
        beq         2       0       re1          base case (r = 0)
        beq         1       2       re1          base case (n = r)
        lw          0       6       negOne       combination(n - 1, r)
        add         1       6       1
        lw          0       4       Caddr
        jalr        4       7 
        lw          0       6       negOne       load -1 into reg 6 to decrement r
        add         2       6       2            r gets decremented
        sw          5       3       Stack        save result on the stack
        lw          0       6       posOne
        add         5       6       5
        lw          0       4       Caddr
        jalr        4       7                    second call combination(n - 1, r - 1)
        lw          0       6       negOne       restore stack result in register 6
        add         5       6       5
        lw          5       4       Stack
        add         4       3       3            add, put into register 3, and restore register
        beq         0       0       res
re1     lw          0       3       posOne       return 1
res     lw          0       6       negOne       restore callee saved registers  
        add         5       6       5            decrement memory pointer
        lw          5       2       Stack        restore r
        add         5       6       5            decrement memory pointer
        lw          5       1       Stack        restore n
        add         5       6       5            decrement memory pointer
        lw          5       7       Stack        restore register 7
        jalr        7       4
n       .fill       7
r       .fill       3
Caddr   .fill       comb
negOne  .fill       -1
posOne  .fill       1
Stack   .fill       0