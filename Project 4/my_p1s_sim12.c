/*
 * Project 1
 * EECS 370 LC-2K Instruction-level simulator
 *
 * Make sure *not* to modify printState or any of the associated functions
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define NUMMEMORY 65536 /* maximum number of words in memory */
#define NUMREGS 8       /* number of machine registers */

// File
#define MAXLINELENGTH 1000 /* MAXLINELENGTH is the max number of characters we read */


typedef struct stateStruct
{
    int pc;
    int mem[NUMMEMORY];
    int reg[NUMREGS];
    int numMemory;
} stateType;


void printState(stateType *);

int convertNum(int);

extern void cache_init(int blockSize, int numSets, int blocksPerSet);
extern int cache_access(int addr, int write_flag, int write_data);
extern void printStats();
static stateType state;
static int num_mem_accesses = 0;
int mem_access(int addr, int write_flag, int write_data) {
    ++num_mem_accesses;
    if (write_flag) {
        state.mem[addr] = write_data;
    }
    return state.mem[addr];
}
int get_num_mem_accesses(){
	return num_mem_accesses;
}

int main(int argc, char *argv[])
{
    // cache parameters
    int blockSizeInWords = atoi(argv[2]); // argv 2
    int numOfSets = atoi(argv[3]); // argv 3
    int blocksPerSet = atoi(argv[4]); // argv 4

    // initialie my registers and other other stuff
    int reg_One = 0; //arg0
    int reg_Two = 0; //arg1
    int destination_reg = 0; // arg2
    int operation = 0; //opcode
    int offsetField = 0;
    int instruction_num = 0;
    int halt_Q;

    char line[MAXLINELENGTH];
    FILE *filePtr;

    if (argc != 5)
    {
        printf("error: usage: %s <machine-code file>\n", argv[0]);
        exit(1);
    }

    filePtr = fopen(argv[1], "r");
    if (filePtr == NULL)
    {
        printf("error: can't open file %s", argv[1]);
        perror("fopen");
        exit(1);
    }

    cache_init(blockSizeInWords, numOfSets, blocksPerSet);

    /* read the entire machine-code file into memory */
    for (state.numMemory = 0; fgets(line, MAXLINELENGTH, filePtr) != NULL; state.numMemory++)
    {
        if (sscanf(line, "%d", state.mem + state.numMemory) != 1)
        {
            printf("error in reading address %d\n", state.numMemory);
            exit(1);
        }
        printf("memory[%d]=%d\n", state.numMemory, state.mem[state.numMemory]);
    }
    // Your code starts here

    // the registers have been initialized to 0
    for (int p = 0; p < NUMREGS; p++)
    {
        state.reg[p] = 0;
    }

    state.pc = 0;
    instruction_num = 0;
    halt_Q = 0;

    // iterate through each operation while it is halted
    while (!halt_Q)
    {
        //printState(&state);
        ++instruction_num;

        int instruction = cache_access(state.pc, 0, 0);

        operation = ((instruction >> 22) & 7);
        reg_One = ((instruction >> 19) & 7);
        reg_Two = ((instruction >> 16) & 7);
        destination_reg = ((instruction >> 0) & 7);           // for add and nor
        offsetField = convertNum((instruction >> 0) & 65535); // for beq, lw, & sw

        //add
        if (operation == 0)
        {
            state.reg[destination_reg] = (state.reg[reg_One] + state.reg[reg_Two]);
            ++state.pc;
        }

        //nor
        else if (operation == 1)
        {
            state.reg[destination_reg] = ~(state.reg[reg_One] | state.reg[reg_Two]);
            ++state.pc;
        }
        
        //lw
        else if (operation == 2)
        {
            state.reg[reg_Two] = (instruction + offsetField);
            ++state.pc;
        }

        //sw
        else if (operation == 3)
        {
            instruction = state.reg[reg_Two];
            ++state.pc;
        }

        //beq
        else if (operation == 4) {
            if(state.reg[reg_One] == state.reg[reg_Two]){
                state.pc = (state.pc + offsetField + 1);
            }
            else{
                ++state.pc;
            }
        }

        //jalr 
        else if (operation == 5)
        {
            state.reg[reg_Two] = (state.pc) + 1;
            state.pc = state.reg[reg_One];
        }

        //halt
        else if (operation == 6){
            ++state.pc;
            halt_Q = 1;
            printStats();
           // break;
        }

        //noop
        else if (operation == 7){
            ++state.pc;
        }
        
    }
    printf("end state\n");
    printf("total of %d instructions executed \n", instruction_num);
    printf("final state of machine:\n");
    printState(&state);

    return (0);
}

void printState(stateType *statePtr)
{
    int i;
    printf("\n@@@\nstate:\n");
    printf("\tpc %d\n", statePtr->pc);
    printf("\tmemory:\n");
    for (i = 0; i < statePtr->numMemory; i++)
    {
        printf("\t\tmem[ %d ] %d\n", i, statePtr->mem[i]);
    }
    printf("\tregisters:\n");
    for (i = 0; i < NUMREGS; i++)
    {
        printf("\t\treg[ %d ] %d\n", i, statePtr->reg[i]);
    }
    printf("end state\n");
}

int convertNum(int num)
{
    /* convert a 16-bit number into a 32-bit Linux integer */
    if (num & (1 << 15))
    {
        num -= (1 << 16);
    }
    return (num);
}
