/*
 * Project 1
 * EECS 370 LC-2K Instruction-level simulator
 *
 * Make sure *not* to modify printState or any of the associated functions
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Machine Definitions
#define NUMMEMORY 65536 /* maximum number of words in memory */
#define NUMREGS 8 /* number of machine registers */

// File
#define MAXLINELENGTH 1000 /* MAXLINELENGTH is the max number of characters we read */

typedef struct stateStruct {
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
    int blockSize = atoi(argv[2]);
    int numSets =  atoi(argv[3]);
    int blocksPerSet = atoi(argv[4]);
    cache_init(blockSize, numSets, blocksPerSet);
    
    //Make sure legal
    if(blockSize > 256 || ((numSets * blocksPerSet) > 256)) {
        exit(1);
    }
    
    char line[MAXLINELENGTH];
    FILE *filePtr;

    if (argc != 5) {
        printf("error: usage: %s <machine-code file>\n", argv[0]);
        exit(1);
    }

    filePtr = fopen(argv[1], "r");
    if (filePtr == NULL) {
        printf("error: can't open file %s", argv[1]);
        perror("fopen");
        exit(1);
    }

    /* read the entire machine-code file into memory */
    for (state.numMemory = 0; fgets(line, MAXLINELENGTH, filePtr) != NULL; state.numMemory++) {
        if (sscanf(line, "%d", state.mem+state.numMemory) != 1) {
            printf("error in reading address %d\n", state.numMemory);
            exit(1);
        }
    }
    //Zero out the registers & pc
    state.pc = 0;
    for(int i = 0; i < NUMREGS; i++) {
        state.reg[i] = 0;
    }

    //Line by line decoding
    int count = 0; //Instruction count
    
    int j = 0;
    while(j < state.numMemory) {
        int instruction = cache_access(j, 0, 0);
        int opcode = (instruction >> 22) & 7;
        if(opcode == 0b000) {
            //ADD opcode
            int regA = (instruction >> 19) & 7;
            int regB = (instruction >> 16) & 7;
            int destReg = instruction & 7;
            state.reg[destReg] = state.reg[regA] + state.reg[regB];
            
            state.pc++;
            j++;
            count++;
            
        }else if(opcode == 0b001) {
            //NOR opcode
            int regA = (instruction >> 19) & 7;
            int regB = (instruction >> 16) & 7;
            int destReg = instruction & 7;
            state.reg[destReg] = ~(state.reg[regA] | state.reg[regB]);
            
            state.pc++;
            j++;
            count++;
            
        }else if(opcode == 0b010) {
            //LW opcode
            int regA = (instruction >> 19) & 7;
            int regB = (instruction >> 16) & 7;
            int offsetField = convertNum(instruction & 65535);
            state.reg[regB] = cache_access(state.reg[regA] + offsetField, 0, 0);
            
            state.pc++;
            j++;
            count++;
            
        }else if(opcode == 0b011) {
            //SW opcode
            int regA = (instruction >> 19) & 7;
            int regB = (instruction >> 16) & 7;
            int offsetField = convertNum(instruction & 65535);
            cache_access(state.reg[regA] + offsetField, 1, state.reg[regB]);

            state.pc++;
            j++;
            count++;
            
        }else if(opcode == 0b100) {
            //BEQ opcode
            int regA = (instruction >> 19) & 7;
            int regB = (instruction >> 16) & 7;
            int offsetField = convertNum(instruction & 65535);
            if(state.reg[regA] == state.reg[regB]) {
                //Branch
                j = state.pc + offsetField + 1;
                state.pc = j;
                count++;
            }else {
                //Just move forward
                state.pc++;
                j++;
                count++;
            }
            
        }else if(opcode == 0b101) {
            //JALR opcode
            int regA = (instruction >> 19) & 7;
            int regB = (instruction >> 16) & 7;
            //Store
            state.reg[regB] = state.pc + 1;
            //Jump
            j = state.reg[regA];
            state.pc = j;
            count++;
            
        }else if(opcode == 0b110) {
            //HALT opcode
            state.pc++;
            count++;
            printf("machine halted\ntotal of %i instructions executed\nfinal state of machine:\n", count);
            break;
            
        }else if(opcode == 0b111) {
            //NOOP opcode
            state.pc++;
            j++;
            count++;
            
        }
    }
    printState(&state);
    printStats();

    return(0);
}

void printState(stateType *statePtr)
{
    int i;
    printf("\n@@@\nstate:\n");
    printf("\tpc %d\n", statePtr->pc);
    printf("\tmemory:\n");
    for (i=0; i<statePtr->numMemory; i++) {
              printf("\t\tmem[ %d ] %d\n", i, statePtr->mem[i]);
    }
    printf("\tregisters:\n");
    for (i=0; i<NUMREGS; i++) {
              printf("\t\treg[ %d ] %d\n", i, statePtr->reg[i]);
    }
    printf("end state\n");
}

int convertNum(int num)
{
    /* convert a 16-bit number into a 32-bit Linux integer */
    if (num & (1<<15) ) {
        num -= (1<<16);
    }
    return(num);
}



