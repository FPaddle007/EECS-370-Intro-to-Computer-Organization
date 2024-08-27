/**
 *Christopher Felix
 *EECS 370
 * Project 1
 * Assembler code fragment for LC-2K
 */

#define MAXINSTR 1000
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAXLINELENGTH 1000

struct passes
{
    int tracker;
    char *Mylabel;
};
enum operations
{
    add,
    nor,
    lw,
    sw,
    beq,
    jalr,
    halt,
    noop
};
enum
{
    false,
    true
};

int readAndParse(FILE *, char *, char *, char *, char *, char *);
int isNumber(char *);

int main(int argc, char *argv[])
{

    struct passes *LabelPasses;
    LabelPasses = malloc(sizeof(struct passes) * MAXINSTR);

    int counter = 0;
    int trackerLabel = false;
    int trackerLine = 0;
    int machineCode = 0;

    char *inFileString, *outFileString;
    FILE *inFilePtr, *outFilePtr;
    char label[MAXLINELENGTH], opcode[MAXLINELENGTH], arg0[MAXLINELENGTH],
        arg1[MAXLINELENGTH], arg2[MAXLINELENGTH];

    if (argc != 3)
    {
        printf("error: usage: %s <assembly-code-file> <machine-code-file>\n",
               argv[0]);
        exit(1);
    }

    inFileString = argv[1];
    outFileString = argv[2];

    inFilePtr = fopen(inFileString, "r");
    if (inFilePtr == NULL)
    {
        printf("error in opening %s\n", inFileString);
        exit(1);
    }
    outFilePtr = fopen(outFileString, "w");
    if (outFilePtr == NULL)
    {
        printf("error in opening %s\n", outFileString);
        exit(1);
    }

    // 1st pass
    while (readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2))
    {
        LabelPasses[trackerLine].tracker = trackerLine;
        if (label == '\0')
        {
            strcpy(LabelPasses[trackerLine].Mylabel, "");
        }
        else
        {
            LabelPasses[trackerLine].Mylabel = malloc(sizeof(label));
            strcpy(LabelPasses[trackerLine].Mylabel, label);
        }
        ++trackerLine;
    }
    // Ensure no labels are repeated
    for (int i = 0; i < trackerLine; i++)
    {
        if (strcmp(LabelPasses[i].Mylabel, ""))
        {
            for (int j = 0; j < trackerLine; j++)
            {
                if (j != i)
                {
                    if (!strcmp(LabelPasses[i].Mylabel, LabelPasses[j].Mylabel))
                    {
                        fprintf(outFilePtr, "Duplicate Label\n");
                        exit(1);
                    }
                }
            }
        }
    }

    /* this is how to rewind the file ptr so that you start reading from the
        beginning of the file */
    rewind(inFilePtr);

    // 2nd pass

    /* after doing a readAndParse, you may want to do the following to test the
        opcode */
    while (readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2))
    {
        /* do whatever you need to do for opcode "add" */

        //(Add, Nor Instructions)
        if (!strcmp(opcode, "add"))
        {
            machineCode = (add << 22);
            // arg0 = regA
            if (isNumber(arg0))
            {
                machineCode = machineCode + (atoi(arg0) << 19);
            }
            else
            {
                exit(1);
            }
            // arg1 = regB
            if (isNumber(arg1))
            {
                machineCode = machineCode + (atoi(arg1) << 16);
            }
            else
            {
                exit(1);
            }
            // destReg
            if (isNumber(arg2))
            {
                machineCode = machineCode + (atoi(arg2) << 0);
            }
            else
            {
                exit(1);
            }
        }
        else if (!strcmp(opcode, "nor"))
        {
            machineCode = (nor << 22);
            // arg0 = regA
            if (isNumber(arg0))
            {
                machineCode = machineCode + (atoi(arg0) << 19);
            }
            else
            {
                exit(1);
            }
            // arg1 = regB
            if (isNumber(arg1))
            {
                machineCode = machineCode + (atoi(arg1) << 16);
            }
            else
            {
                exit(1);
            }
            // destReg
            if (isNumber(arg2))
            {
                machineCode = machineCode + (atoi(arg2) << 0);
            }
            else
            {
                exit(1);
            }
        }

        //(lw & sw instructions)
        else if (!strcmp(opcode, "lw") || !strcmp(opcode, "sw"))
        {
            if (!strcmp(opcode, "lw"))
            {
                machineCode = (lw << 22);
                // arg0 = regA
                if (isNumber(arg0))
                {
                    machineCode = machineCode + (atoi(arg0) << 19);
                }
                else
                {
                    exit(1);
                }
                // arg1 = regB
                if (isNumber(arg1))
                {
                    machineCode = machineCode + (atoi(arg1) << 16);
                }
                else
                {
                    exit(1);
                }
            }
            else
            {
                machineCode = (sw << 22);
                // arg0 = regA
                if (isNumber(arg0))
                {
                    machineCode = machineCode + (atoi(arg0) << 19);
                }
                else
                {
                    exit(1);
                }
                // arg1 = regB
                if (isNumber(arg1))
                {
                    machineCode = machineCode + (atoi(arg1) << 16);
                }
                else
                {
                    exit(1);
                }
            }
            // destReg offset
            if (isNumber(arg2))
            {
                int offsetField = atoi(arg2);
                if (offsetField > 32767 || offsetField < -32768)
                {
                    exit(1);
                }
                if (offsetField < 0)
                {
                    offsetField = offsetField + (1 << 16);
                }
                machineCode = machineCode + offsetField;
            }
            else
            {
                for (int p = 0; p < trackerLine; p++)
                {
                    if (!strcmp(LabelPasses[p].Mylabel, arg2))
                    {
                        int offsetField = LabelPasses[p].tracker;
                        if (offsetField > 32767 || offsetField < -32768)
                        {
                            exit(1);
                        }
                        if (offsetField < 0)
                        {
                            offsetField = offsetField + (1 << 16);
                        }
                        machineCode = (machineCode + offsetField) << 0;

                        trackerLabel = true; // must be true for error
                        break;
                    }
                }
                if (!trackerLabel)
                { // if not found, exit
                    exit(1);
                }
            }
        }

        // beq instructions
        else if (!strcmp(opcode, "beq"))
        {
            machineCode = (beq << 22);
            // arg0 = regA
            if (isNumber(arg0))
            {
                machineCode = machineCode + (atoi(arg0) << 19);
            }
            else
            {
                exit(1);
            }
            // arg1 = regB
            if (isNumber(arg1))
            {
                machineCode = machineCode + (atoi(arg1) << 16);
            }
            else
            {
                exit(1);
            }
            // destReg offset
            if (isNumber(arg2))
            {
                int offsetField = atoi(arg2);
                if (offsetField > 32767 || offsetField < -32768)
                {
                    exit(1);
                }
                if (offsetField < 0)
                {
                    offsetField = offsetField + (1 << 16);
                }
                machineCode = machineCode + offsetField;
            }
            else
            {
                for (int p = 0; p < trackerLine; p++)
                {
                    if (!strcmp(LabelPasses[p].Mylabel, arg2))
                    {
                        int value = LabelPasses[p].tracker;
                        trackerLabel = true; // must be true for error
                        if (value < counter)
                        {
                            value = LabelPasses[p].tracker - counter - 1;
                            // value = 65535 ^(-1*value);
                            if (value > 32767 || value < -32768)
                            {
                                exit(1);
                            }
                            value = value + (1 << 16);
                            value = (value);
                            machineCode = machineCode + value;
                        }
                        else
                        {
                            value = LabelPasses[p].tracker - counter - 1;
                            if (value > 32767 || value < -32768)
                            {
                                exit(1);
                            }
                            machineCode = machineCode + value;
                            break;
                        }
                    }
                }
                if (!trackerLabel)
                { // if not found, exit
                    exit(1);
                }
            }
        }
        else if(!strcmp(opcode, "jalr")){
            machineCode = jalr << 22;
            if (isNumber(arg0))
            {
                machineCode = machineCode + (atoi(arg0) << 19);
            }
            else
            {
                exit(1);
            }
            // arg1 = regB
            if (isNumber(arg1))
            {
                machineCode = machineCode + (atoi(arg1) << 16);
            }
            else
            {
                exit(1);
            }
        }

        //(NOOP, HALT Instructions)
        else if (!strcmp(opcode, "noop") || !strcmp(opcode, "halt"))
        {
            if (!strcmp(opcode, "noop"))
            {
                machineCode = (noop << 22);
            }
            else
            {
                machineCode = (halt << 22);
            }
        }

        //(.fill Instructions)
        else if (!strcmp(opcode, ".fill"))
        {
            // arg0
            if (isNumber(arg0))
            {
                machineCode = (atoi(arg0));
            }
            else
            {
                for (int p = 0; p < trackerLine; p++)
                {
                    if (!strcmp(LabelPasses[p].Mylabel, arg0))
                    {
                        machineCode = LabelPasses[p].tracker << 0;
                        trackerLabel = true; // must be true for error
                        break;
                    }
                }
                if (!trackerLabel)
                { // if not found, exit
                    exit(1);
                }
            }
        }
        // if none of the above opcodes
        //(add,nor,lw,sw,beq,.fill,halt,noop)
        // were inputted, exit with error
        else
        {
            exit(1);
        }
        counter++;
        fprintf(outFilePtr, "%i\n", machineCode);
    }
    exit(0);
}

/*
 * Read and parse a line of the assembly-language file.  Fields are returned
 * in label, opcode, arg0, arg1, arg2 (these strings must have memory already
 * allocated to them).
 *
 * Return values:
 *     0 if reached end of file
 *     1 if all went well
 *
 * exit(1) if line is too long.
 */
int readAndParse(FILE *inFilePtr, char *label, char *opcode, char *arg0,
                 char *arg1, char *arg2)
{
    char line[MAXLINELENGTH];
    char *ptr = line;

    /* delete prior values */
    label[0] = opcode[0] = arg0[0] = arg1[0] = arg2[0] = '\0';

    /* read the line from the assembly-language file */
    if (fgets(line, MAXLINELENGTH, inFilePtr) == NULL)
    {
        /* reached end of file */
        return (0);
    }

    /* check for line too long */
    if (strlen(line) == MAXLINELENGTH - 1)
    {
        printf("error: line too long\n");
        exit(1);
    }

    /* is there a label? */
    ptr = line;
    if (sscanf(ptr, "%[^\t\n ]", label))
    {
        /* successfully read label; advance pointer over the label */
        ptr += strlen(label);
    }

    /*
     * Parse the rest of the line.  Would be nice to have real regular
     * expressions, but scanf will suffice.
     */
    sscanf(ptr, "%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]",
           opcode, arg0, arg1, arg2);
    return (1);
}

int isNumber(char *string)
{
    /* return 1 if string is a number */
    int i;
    return ((sscanf(string, "%d", &i)) == 1);
}