/*
 * EECS 370, University of Michigan
 * Project 3: LC-2K Pipeline Simulator
 * Instructions are found in the project spec.
 * Make sure NOT to modify printState or any of the associated functions
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Machine Definitions
#define NUMMEMORY 65536 // maximum number of data words in memory
#define NUMREGS 8 // number of machine registers

#define ADD 0
#define NOR 1
#define LW 2
#define SW 3
#define BEQ 4
#define JALR 5 // will not implemented for Project 3
#define HALT 6
#define NOOP 7

#define NOOPINSTRUCTION (NOOP << 22)

typedef struct IFIDStruct {
	int instr;
	int pcPlus1;
} IFIDType;

typedef struct IDEXStruct {
	int instr;
	int pcPlus1;
	int readRegA;
	int readRegB;
	int offset;
} IDEXType;

typedef struct EXMEMStruct {
	int instr;
	int branchTarget;
    int eq;
	int aluResult;
	int readRegB;
} EXMEMType;

typedef struct MEMWBStruct {
	int instr;
	int writeData;
} MEMWBType;

typedef struct WBENDStruct {
	int instr;
	int writeData;
} WBENDType;

typedef struct stateStruct {
	int pc;
	int instrMem[NUMMEMORY];
	int dataMem[NUMMEMORY];
	int reg[NUMREGS];
	int numMemory;
	IFIDType IFID;
	IDEXType IDEX;
	EXMEMType EXMEM;
	MEMWBType MEMWB;
	WBENDType WBEND;
	int cycles; // number of cycles run so far
} stateType;

static inline int opcode(int instruction) {
    return instruction>>22;
}

static inline int field0(int instruction) {
    return (instruction>>19) & 0x7;
}

static inline int field1(int instruction) {
    return (instruction>>16) & 0x7;
}

static inline int field2(int instruction) {
    return instruction & 0xFFFF;
}

// convert a 16-bit number into a 32-bit Linux integer
static inline int convertNum(int num) {
    return num - ( (num & (1<<15)) ? 1<<16 : 0 );
}

void printState(stateType*);
void printInstruction(int);
void readMachineCode(stateType*, char*);

int main(int argc, char *argv[]) {

    int val_in_regA = 0;
    int val_in_regB = 0;
    int dest_MEMWB = 0;
    int dest_WBEND = 0;
    int dest_EXEM = 0;

    stateType state, newState;

    int currentInstruction;

    if (argc != 2) {
        printf("error: usage: %s <machine-code file>\n", argv[0]);
        exit(1);
    }

    readMachineCode(&state, argv[1]);

    while (opcode(state.MEMWB.instr) != HALT) {
        printState(&state);

        newState = state;
        newState.cycles++;

        /* ---------------------- IF stage --------------------- */
        // fetch instructions & PC + 1 and store them in IF/ID reg
        newState.IFID.instr = state.instrMem[state.pc];
        currentInstruction = newState.IFID.instr;
        newState.IFID.pcPlus1 = state.pc + 1;
        newState.pc++;

        /* ---------------------- ID stage --------------------- */
        // decode instructions & PC + 1 and store them into ID/EX reg
        newState.IDEX.instr = state.IFID.instr;
        currentInstruction = newState.IDEX.instr;
        newState.IDEX.pcPlus1 = state.IFID.pcPlus1;

        //stall for lw instruction
        if(opcode(state.IDEX.instr) == LW && (field0(newState.IDEX.instr) == field1(state.IDEX.instr) || field1(newState.IDEX.instr) == field1(state.IDEX.instr))){
            newState.IDEX.instr = NOOPINSTRUCTION;
            newState.pc = state.pc;
            newState.IFID = state.IFID;
        }

        //if there is no lw hazard, 
        //get the values of regA & regB and store them into ID/EX reg

        else{
            newState.IDEX.readRegA = state.reg[field0(state.IFID.instr)];
            newState.IDEX.readRegB = state.reg[field1(state.IFID.instr)];
            newState.IDEX.offset = convertNum(field2(state.IFID.instr));
        }
        /* ---------------------- EX stage --------------------- */
        // store pc + 1 + offset to EX/MEM branch
        newState.EXMEM.instr = state.IDEX.instr;
        newState.EXMEM.branchTarget = state.IDEX.pcPlus1 + state.IDEX.offset;
        currentInstruction = newState.EXMEM.instr;
        
        //keep track of current values in regs
        int regA_Current;
        int regB_Current;

        val_in_regB = state.IDEX.readRegB;
        val_in_regA = state.IDEX.readRegA;

        regB_Current = field1(newState.EXMEM.instr);
        regA_Current = field0(newState.EXMEM.instr);

        //dest_WBEND will be 1st field if instruction is lw
        if(opcode(state.WBEND.instr) == LW){
            dest_WBEND = field1(state.WBEND.instr);
        }

        //dest_WBEND wil be 2nd field if any instruction is anything other than lw
        else{
            dest_WBEND = field2(state.WBEND.instr);
        }

        // check for data hazard; if there is, forwards the correct value
        // 3 instructions away
        if (opcode(state.WBEND.instr) == ADD || opcode(state.WBEND.instr) == NOR || opcode(state.WBEND.instr) == LW){
            // check if regA matches with dest_WBEND and if it does, get the new value of regA
            if(regA_Current == dest_WBEND){
                val_in_regA = state.WBEND.writeData;
            }

            // check if regB matches with dest_WBEND and if it does, get the new value of regB
            if(regB_Current == dest_WBEND){
                val_in_regB = state.WBEND.writeData;
            }
        }

        // if instruction is lw, dest_MEMWB = regB
        if(opcode(state.MEMWB.instr) == LW){
            dest_MEMWB = field1(state.MEMWB.instr);
        }

        // put dest_MEMWB in field2 (3rd val)
        else{
            dest_WBEND = field2(state.MEMWB.instr);
        }

        // check for data hazard; if there is, forwards the correct value
        // 2 instructions away
        if(opcode(state.MEMWB.instr) == ADD || opcode(state.MEMWB.instr) == NOR || opcode(state.MEMWB.instr) == LW){
            // use new WB value if the destination of MEM/WB register equals regA of new instruction
            if(regA_Current == dest_MEMWB){
                val_in_regA = state.MEMWB.writeData;
            }
            // use new WB value if the destination of MEM/WB register equals regB of new instruction
            if(regB_Current == dest_MEMWB){
                val_in_regB = state.MEMWB.writeData;
            }
        }
        
        if (opcode(state.EXMEM.instr) == LW){
            dest_EXEM = field1(state.EXMEM.instr);
        }
        else{
            dest_EXEM = field2(state.EXMEM.instr);
        }

        if(opcode(state.EXMEM.instr) == ADD || opcode(state.EXMEM.instr) == NOR || opcode(state.EXMEM.instr) == LW){
            // regA = EXMEM
            if(regA_Current == dest_EXEM){
                val_in_regA = state.EXMEM.aluResult;
            }
            // regB = EXMEM
            else if(regB_Current == dest_EXEM){
                val_in_regB = state.EXMEM.aluResult;
            }
        }

        // do the alu result and store it depending on what operation is done
        if(opcode(currentInstruction) == ADD){
            newState.EXMEM.aluResult = val_in_regA + val_in_regB;
        }

        else if(opcode(currentInstruction) == NOR){
            newState.EXMEM.aluResult = ~(val_in_regA | val_in_regB);
        }

        else if(opcode(currentInstruction) == LW){
            newState.EXMEM.aluResult = val_in_regA + state.IDEX.offset;
        }

        else if(opcode(currentInstruction) == SW){
            newState.EXMEM.aluResult = val_in_regA + state.IDEX.offset;
        }

        else if(opcode(currentInstruction) == BEQ){
            newState.EXMEM.aluResult = val_in_regA - val_in_regB;
        }

        if(opcode(currentInstruction) == NOOP){
            newState.EXMEM.readRegB = val_in_regB;
        }

        /* --------------------- MEM stage --------------------- */
        newState.MEMWB.instr = state.EXMEM.instr;
        currentInstruction = newState.MEMWB.instr;

        if(opcode(currentInstruction) == LW){
            newState.MEMWB.writeData = state.dataMem[state.EXMEM.aluResult];
        }

        else if(opcode(currentInstruction) == SW){
            newState.dataMem[state.EXMEM.aluResult] = state.EXMEM.readRegB;
        }

        else if(opcode(currentInstruction) == BEQ){
            //if branch is taken, disregard preceeding instructions and reset pc counter
            if(state.EXMEM.aluResult == 0){
                newState.pc = state.EXMEM.branchTarget;
                newState.IFID.instr = NOOPINSTRUCTION;
                newState.IDEX.instr = NOOPINSTRUCTION;
                newState.EXMEM.instr = NOOPINSTRUCTION;
            }
        }
        else if(opcode(currentInstruction) != NOOP && opcode(currentInstruction) != HALT){
            newState.MEMWB.writeData = state.EXMEM.aluResult;
        }

        /* ---------------------- WB stage --------------------- */
        newState.WBEND.instr = state.MEMWB.instr;
        currentInstruction = newState.WBEND.instr;
        newState.WBEND.writeData = state.MEMWB.writeData;
        
        if(opcode(state.MEMWB.instr) == LW){
            newState.reg[field1(currentInstruction)] = state.MEMWB.writeData;
        }

        if(opcode(state.MEMWB.instr) == ADD || opcode(state.MEMWB.instr) == NOR){
            newState.reg[field2(currentInstruction)] = state.MEMWB.writeData;
        }

        /* ------------------------ END ------------------------ */
        state = newState; /* this is the last statement before end of the loop. It marks the end 
        of the cycle and updates the current state with the values calculated in this cycle */
    }
    printf("machine halted\n");
    printf("total of %d cycles executed\n", state.cycles);
    printf("final state of machine:\n");
    printState(&state);
}

void printInstruction(int instr) {
    switch (opcode(instr)) {
        case ADD:
            printf("add");
            break;
        case NOR:
            printf("nor");
            break;
        case LW:
            printf("lw");
            break;
        case SW:
            printf("sw");
            break;
        case BEQ:
            printf("beq");
            break;
        case JALR:
            printf("jalr");
            break;
        case HALT:
            printf("halt");
            break;
        case NOOP:
            printf("noop");
            break;
        default:
            printf(".fill %d", instr);
            return;
    }
    printf(" %d %d %d", field0(instr), field1(instr), field2(instr));
}

void printState(stateType *statePtr) {
    printf("\n@@@\n");
    printf("state before cycle %d starts:\n", statePtr->cycles);
    printf("\tpc = %d\n", statePtr->pc);

    printf("\tdata memory:\n");
    for (int i=0; i<statePtr->numMemory; ++i) {
        printf("\t\tdataMem[ %d ] = %d\n", i, statePtr->dataMem[i]);
    }
    printf("\tregisters:\n");
    for (int i=0; i<NUMREGS; ++i) {
        printf("\t\treg[ %d ] = %d\n", i, statePtr->reg[i]);
    }

    // IF/ID
    printf("\tIF/ID pipeline register:\n");
    printf("\t\tinstruction = %d ( ", statePtr->IFID.instr);
    printInstruction(statePtr->IFID.instr);
    printf(" )\n");
    printf("\t\tpcPlus1 = %d", statePtr->IFID.pcPlus1);
    if(opcode(statePtr->IFID.instr) == NOOP){
        printf(" (Don't Care)");
    }
    printf("\n");
    
    // ID/EX
    int idexOp = opcode(statePtr->IDEX.instr);
    printf("\tID/EX pipeline register:\n");
    printf("\t\tinstruction = %d ( ", statePtr->IDEX.instr);
    printInstruction(statePtr->IDEX.instr);
    printf(" )\n");
    printf("\t\tpcPlus1 = %d", statePtr->IDEX.pcPlus1);
    if(idexOp == NOOP){
        printf(" (Don't Care)");
    }
    printf("\n");
    printf("\t\treadRegA = %d", statePtr->IDEX.readRegA);
    if (idexOp >= HALT || idexOp < 0) {
        printf(" (Don't Care)");
    }
    printf("\n");
    printf("\t\treadRegB = %d", statePtr->IDEX.readRegB);
    if(idexOp == LW || idexOp > BEQ || idexOp < 0) {
        printf(" (Don't Care)");
    }
    printf("\n");
    printf("\t\toffset = %d", statePtr->IDEX.offset);
    if (idexOp != LW && idexOp != SW && idexOp != BEQ) {
        printf(" (Don't Care)");
    }
    printf("\n");

    // EX/MEM
    int exmemOp = opcode(statePtr->EXMEM.instr);
    printf("\tEX/MEM pipeline register:\n");
    printf("\t\tinstruction = %d ( ", statePtr->EXMEM.instr);
    printInstruction(statePtr->EXMEM.instr);
    printf(" )\n");
    printf("\t\tbranchTarget %d", statePtr->EXMEM.branchTarget);
    if (exmemOp != BEQ) {
        printf(" (Don't Care)");
    }
    printf("\n");
    printf("\t\teq ? %s", (statePtr->EXMEM.eq ? "True" : "False"));
    if (exmemOp != BEQ) {
        printf(" (Don't Care)");
    }
    printf("\n");
    printf("\t\taluResult = %d", statePtr->EXMEM.aluResult);
    if (exmemOp > SW || exmemOp < 0) {
        printf(" (Don't Care)");
    }
    printf("\n");
    printf("\t\treadRegB = %d", statePtr->EXMEM.readRegB);
    if (exmemOp != SW) {
        printf(" (Don't Care)");
    }
    printf("\n");

    // MEM/WB
	int memwbOp = opcode(statePtr->MEMWB.instr);
    printf("\tMEM/WB pipeline register:\n");
    printf("\t\tinstruction = %d ( ", statePtr->MEMWB.instr);
    printInstruction(statePtr->MEMWB.instr);
    printf(" )\n");
    printf("\t\twriteData = %d", statePtr->MEMWB.writeData);
    if (memwbOp >= SW || memwbOp < 0) {
        printf(" (Don't Care)");
    }
    printf("\n");     

    // WB/END
	int wbendOp = opcode(statePtr->WBEND.instr);
    printf("\tWB/END pipeline register:\n");
    printf("\t\tinstruction = %d ( ", statePtr->WBEND.instr);
    printInstruction(statePtr->WBEND.instr);
    printf(" )\n");
    printf("\t\twriteData = %d", statePtr->WBEND.writeData);
    if (wbendOp >= SW || wbendOp < 0) {
        printf(" (Don't Care)");
    }
    printf("\n");

    printf("end state\n");
}

// File
#define MAXLINELENGTH 1000 // MAXLINELENGTH is the max number of characters we read

void readMachineCode(stateType *state, char* filename) {
    char line[MAXLINELENGTH];
    FILE *filePtr = fopen(filename, "r");
    if (filePtr == NULL) {
        printf("error: can't open file %s", filename);
        exit(1);
    }

    printf("instruction memory:\n");
    for (state->numMemory = 0; fgets(line, MAXLINELENGTH, filePtr) != NULL; ++state->numMemory) {
        if (sscanf(line, "%d", state->instrMem+state->numMemory) != 1) {
            printf("error in reading address %d\n", state->numMemory);
            exit(1);
        }
        printf("\tinstrMem[ %d ] = ", state->numMemory);
        printInstruction(state->dataMem[state->numMemory] = state->instrMem[state->numMemory]);
        printf("\n");
    }
}