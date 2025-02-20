/* findsyms
 * ========
 *
 * findsyms <options>
 *
 * -Fi=<file_in>
 * -Fo=<file_out>
 * -A=<format_spec>
 * -V=<val_spec> Can't see any code for this one
 */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>

# if (defined(_MSC_VER))
#  if (_MSC_VER>=1000)
#   if (defined(_DEBUG))

#ifndef _CRTDBG_MAP_ALLOC
#define _CRTDBG_MAP_ALLOC
#endif /* _CRTDBG_MAP_ALLOC */

#include <windows.h>
#include <crtdbg.h>

#define FINDASSERT(x) _ASSERTE(x)

#   endif /* (defined(_DEBUG)) */
#  endif /* (_MSC_VER>=1000) */
# endif /* (defined(_MSC_VER)) */

#if (!defined(FINDASSERT))
#include <assert.h>
#define FINDASSERT(x) assert(x)
#endif /* (!defined(FINDASSERT)) */

enum _AsmType
{
    ASMTYPE_NONE = 0,
    ASMTYPE_ML,
    ASMTYPE_GAS,
    ASMTYPE_SH4,
    ASMTYPE_CWPS2,
    ASMTYPE_CWGCN
};
typedef enum _AsmType AsmType;

int Nasm = 0;

static char *
readFile(const char *fileName, int *length)
{
    FILE    *fptr;
    char    *buffer;

    /* Read file in */
    fptr = fopen(fileName, "rb");
    FINDASSERT(NULL != fptr);

    fseek(fptr, 0, SEEK_END);
    if ((*length = (int)ftell(fptr)) == -1)
    {
        /* Failed to get length */
        fclose(fptr);
        return (NULL);
    }

    buffer = malloc((*length)+1);
    if (!buffer)
    {
        fclose(fptr);
        return (NULL);
    }

    fseek(fptr, 0, SEEK_SET);
    fread(buffer, *length, 1, fptr);
    fclose(fptr);

    /* Terminate it */
    buffer[*length] = 0;

    return(buffer);
}

static void
replaceChars(char *buffer, int length, char replaceThis, char withThis)
{
    int     i;

    for (i = 0; i < length; i++)
    {
        if (buffer[i] == replaceThis)
        {
            buffer[i] = withThis;
        }
    }
}

static void
ml_asm(int  numLines, char **lines, FILE *fptr)
{
    int i;
    char    *colonStr = NULL;
    char    *dbStr = NULL;
    char    *ddStr = NULL;
    char    *dupStr = NULL;
    char    *symbol = NULL;

    /* Search for DB and DD, and grab size from same line */
    for (i = 0; i < numLines; i++)
    {
        char    *thisLine = lines[i];
        int     lineLength = strlen(thisLine);

        if ((dupStr = strstr(thisLine, "DUP")))
        {
            /* Break it up */
            replaceChars(thisLine, lineLength, 32, 0);
            replaceChars(thisLine, lineLength, 9, 0);

            /* Field 1 is symbol, value is 0 */
            /* +1 to remove leading underscore */
            symbol = &thisLine[1];
            fprintf(fptr, "%s\t\tequ 0\n", symbol);
        }
        else
        if ((ddStr = strstr(thisLine, "DD")))
        {
            /* Break it up */
            replaceChars(thisLine, lineLength, 32, 0);
            replaceChars(thisLine, lineLength, 9, 0);

            /* Field 1 is symbol, field 3 is the value */
            ddStr += strlen(ddStr);
            while (*ddStr == 0)
            {
                ddStr++;
            }
            /* +1 to remove leading underscore */
            symbol = &thisLine[1];
            fprintf(fptr, "%s\t\tequ %s\n", symbol, ddStr);
        }
        else
        if ((colonStr = strstr(thisLine, ":")))
        {
            *colonStr = '\0';
            /* +1 to remove leading underscore */
            symbol = &thisLine[1];
        }
        else
        if (symbol && (dbStr = strstr(thisLine, "DB")))
        {
            /* Break it up */
            replaceChars(thisLine, lineLength, 32, 0);
            replaceChars(thisLine, lineLength, 18, 0);

            /* Field 1 is symbol, value is 0 */
            fprintf(fptr, "%s\t\tequ 0\n", symbol);
        }
    }
    return;
}

static void
gas_asm(int  numLines, char **lines, FILE *fptr)
{
    char gas[] = "\t.equ\t\t%s, %s\n", nasm[] = "%s\t\tequ %s\n";
    char *outputFormat = gas;
    int i;

    if (Nasm)
    {
        outputFormat = nasm;
    }

    /* Search for .long */
    for (i = 0; i < numLines; i++)
    {
        char    *thisLine = lines[i];
        int     lineLength = strlen(thisLine);
        char    *longStr;

        longStr = strstr(thisLine, ".long");
        if (!longStr)
        {
            longStr = strstr(thisLine, ".word");
        }

        if (longStr)
        {
            char    *labelString = NULL;
            int     labelLine = i;

            /* Break it up */
            replaceChars(thisLine, lineLength, 32, 0);
            replaceChars(thisLine, lineLength, 9, 0);

            /* Now look back for a label */
            while (!labelString)
            {
                if (strchr(lines[labelLine], ':'))
                {
                    /* Got it */
                    labelString = lines[labelLine];
                }
                labelLine--;
            }

            /* labelString is the label, field after longStr is value */
            longStr += strlen(longStr);
            while (*longStr == 0)
            {
                longStr++;
            }
            if (isdigit(longStr[0]))
            {
                /* Strip off the colon */
                *strchr(labelString, ':') = 0;

                fprintf(fptr, outputFormat, labelString, longStr);
            }
        }
    }
    return;
}


static void
sh4_asm(int  numLines, char **lines, FILE *fptr)
{
    int i;

    /* Search for .DATA */
    for (i = 0; i < numLines; i++)
    {
        char    *thisLine = lines[i];
        int     lineLength = strlen(thisLine);

        if (strstr(thisLine, ".DATA"))
        {
            char    *dataStr = strstr(thisLine, ".DATA");
            char    *labelString = NULL;
            int     labelLine = i;

            /* Break it up */
            replaceChars(thisLine, lineLength, 32, 0);
            replaceChars(thisLine, lineLength, 9, 0);

            /* Now look back for a label */
            while (!labelString)
            {
                if (strchr(lines[labelLine], ':'))
                {
                    /* Got it */
                    labelString = lines[labelLine];
                }
                labelLine--;
            }

            /* labelString is the label, field after dataStr is value */
            dataStr += strlen(dataStr);
            while (*dataStr == 0)
            {
                dataStr++;
            }

            /* Keep the colon here, but terminate after */
            strchr(labelString, ':')[1] = '\0';
            fprintf(fptr, "%s\t.equ\t%s\n", labelString, dataStr);
        }
    }
    return;
}

static void
cwps2_asm(int  numLines, char **lines, FILE *fptr)
{
    int i, j;
    int numSymbs = 0;
    int numSdata = 0;
    int start = 0;
    int finishpass = 0;
    char buf[1024];
    char **table;

    /* first we figure out how many symbols there are */
    for (i = 0; i < numLines; i++)
    {
        if ((sscanf(lines[i], "%*d %*x %*x GLOBAL OBJECT DEFAULT .sbss %s",
                    buf)==1)
            ||(sscanf(lines[i], "%*d %*x %*x GLOBAL OBJECT DEFAULT .sdata %s",
                      buf)==1)
            ||(sscanf(lines[i], "%*d %*x %*x GLOBAL OBJECT DEFAULT .bss %s",
                      buf)==1)
            ||(sscanf(lines[i], "%*d %*x %*x GLOBAL OBJECT DEFAULT .data %s",
                      buf)==1))
        {
            numSymbs++;
        }
    }
    if (numSymbs == 0)
    {
        fprintf(stderr, "FINDSYMS: No symbols found\n");
        return;
    }
    /* Now we build a table of pointers to those lines, which we will */
    /* use as intermediate store */
    if (!(table = (char **)malloc(sizeof(char*)*numSymbs)))
    {
        fprintf(stderr, "FINDSYMS: Malloc failed\n");
        return;
    }
    j = 0;
    numSdata = numSymbs;
    for (i = 0; i < numLines; i++)
    {
        if ((sscanf(lines[i], "%*d %*x %*x GLOBAL OBJECT DEFAULT .sbss %s",
                    buf)==1)
            || (sscanf(lines[i], "%*d %*x %*x GLOBAL OBJECT DEFAULT .bss %s",
                       buf)==1))
        {
            table[j++] = lines[i];
            /* Match the results of the gas version */
            sprintf(lines[i], "\t.equ\t\t%s, 0\n", buf);
            numSdata--;
        }
        else if ((sscanf(lines[i],
                  "%*d %*x %*x GLOBAL OBJECT DEFAULT .sdata %s", buf)==1)
                 || (sscanf(lines[i],
                            "%*d %*x %*x GLOBAL OBJECT DEFAULT .data %s", buf)
                     ==1))
        {
            table[j++] = lines[i];
        }
    }
    if (numSdata)
    {
        int a, b, c, d;

        j = 0;
        /* Now find the first initialised data section */
        for (i = 1; i < numLines; i++)
        {
            if ((sscanf(lines[i], "0x00000000:  %x %x %x %x '", &a, &b, &c, &d)
                == 4)
                && ((!(strcmp(lines[i-1],
                              "\t\t\t*** INITIALIZED DATA (.sdata) ***")))
                    || (!(strcmp(lines[i-1],
                                 "\t\t\t*** INITIALIZED DATA (.data) ***")))))
            {
                /* Find next sdata and asign it */
                while (*table[j] == '\t')
                {
                    j++;
                }
                if ((sscanf(table[j],
                            "%*d %*x %*x GLOBAL OBJECT DEFAULT .sdata %s", buf)
                     != 1)
                    && (sscanf(table[j],
                               "%*d %*x %*x GLOBAL OBJECT DEFAULT .data %s",
                               buf) != 1))
                {
                    fprintf(stderr, "FINDSYMS: error on line: %d\n%s\n", j,
                            table[j]);
                }
                /* Match the results of the gas version */
                sprintf(table[j++], "\t.equ\t\t%s, %d\n", buf, a+b*0x100
                                                                +c*0x10000
                                                                +d*0x1000000);
                numSdata--;
            }
        }
        if (numSdata !=0)
        {
            fprintf(stderr, "FINDSYMS: %d sdata objects remained\n", numSdata);
        }
    }
    /* Now dump results to file */
    for (i=0; i<numSymbs; i++)
    {
        fprintf(fptr, table[i]);
    }
    free(table);
    return;
}

static void
cwGCN_asm(int numLines, char **lines, FILE *fptr)
{
    int i;
    int num;

    for (i = 0; i < numLines; i++)
    {
        if ((sscanf(lines[i], "%*x: %x '", &num) == 1) &&
            (NULL == strstr(lines[i - 1], "$\0")))
        {
            lines[i - 1][strlen(lines[i - 1]) - 1] = '\0';
            fprintf(fptr, "#define %s 0x%.8X\n", lines[i - 1], num);
        }
    }

    return;
}

static int
findSyms(const char *inFile, const char *outFile, AsmType asmType)
{
    int     result = 1;
    int     bufferLen;
    char    *fileBuffer, **lines;
    int     i;
    FILE    *fptr;
    int     inLine, numLines;

    fileBuffer = readFile(inFile, &bufferLen);
    if (!fileBuffer)
    {
        fprintf(stderr, "FINDSYMS: Cannot read file %s\n", inFile);
        return (0);
    }

    /*
     * Split all the lines up into strings replacing spaces and tabs with
     * string terminators.
     */
    replaceChars(fileBuffer, bufferLen, 10, 0);
    replaceChars(fileBuffer, bufferLen, 13, 0);

    /* Build an array of pointers to lines */
    inLine = 0;
    numLines = 0;
    for (i = 0; i < bufferLen; i++)
    {
        if (inLine)
        {
            if (fileBuffer[i] == 0)
            {
                inLine = 0;
            }
        }
        else
        {
            if (fileBuffer[i])
            {
                inLine = 1;
                numLines++;
            }
        }
    }

    lines = (char **)malloc(numLines * sizeof(char *));
    if (!lines)
    {
        fprintf(stderr, "FINDSYMS: Memory allocation failure for %d bytes\n", numLines * sizeof(char *));
        free(fileBuffer);
        return (0);
    }

    inLine = 0;
    numLines = 0;
    for (i = 0; i < bufferLen; i++)
    {
        if (inLine)
        {
            if (fileBuffer[i] == 0)
            {
                inLine = 0;
            }
        }
        else
        {
            if (fileBuffer[i])
            {
                inLine = 1;
                lines[numLines++] = &fileBuffer[i];
            }
        }
    }

    /* Get ready to write symbols out */
    fptr = fopen(outFile, "wt");
    if (!fptr)
    {
        free(lines);
        free(fileBuffer);
        fprintf(stderr, "FINDSYMS: Cannot open results file %s\n", outFile);
        return (0);
    }

    switch (asmType)
    {
        case (ASMTYPE_ML):
            ml_asm(numLines, lines, fptr);
            break;

        case (ASMTYPE_GAS):
            gas_asm(numLines, lines, fptr);
            break;

        case (ASMTYPE_SH4):
            sh4_asm(numLines, lines, fptr);
            break;

        case (ASMTYPE_CWPS2):
            cwps2_asm(numLines, lines, fptr);
            break;

        case (ASMTYPE_CWGCN):
            cwGCN_asm(numLines, lines, fptr);
            break;

        default:
            fprintf(stderr, "FINDSYMS: Unknown assembler type\n");
            result = 0;
            break;

    }

    fclose(fptr);
    free(lines);
    free(fileBuffer);

    return (result);
}

static void
usage(void)
{
    fprintf(stderr, "FINDSYMS: Usage\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "findsyms -Fi<inputFile> -Fo<outputFile> -A<formatSpec>\n");
    fprintf(stderr, "\n");
}

int
main(int argc, char *argv[])
{
    char    *inFile = NULL;
    char    *outFile = NULL;
    AsmType asmType = ASMTYPE_NONE;

    ++argv, --argc;  /* skip over program name */

    while (argc)
    {
        if (argv[0][0] == '-')
        {
            /* Its an option */
            switch (argv[0][1])
            {
                case ('F'): case ('f'):
                {
                    switch (argv[0][2])
                    {
                        case ('I'): case ('i'):
                        {
                            inFile = &argv[0][3];
                            break;
                        }
                        case ('O'): case ('o'):
                        {
                            outFile = &argv[0][3];
                            break;
                        }
                        default:
                        {
                            break;
                        }
                    }
                    break;
                }
                case ('A'): case ('a'):
                {
                    if (strstr(&argv[0][2], "ml"))
                    {
                        asmType = ASMTYPE_ML;
                    }
                    else if (strstr(&argv[0][2], "sh4"))
                    {
                        asmType = ASMTYPE_SH4;
                    }
                    else if (strstr(&argv[0][2], "nasm"))
                    {
                        asmType = ASMTYPE_GAS;
                        Nasm = 1;
                    }
                    else if (strstr(&argv[0][2], "asm_r5900_elf"))
                    {
                        asmType = ASMTYPE_CWPS2;
                    }
                    else if (strstr(&argv[0][2], "mwasmeppc"))
                    {
                        asmType = ASMTYPE_CWGCN;
                    }
                    else if (strstr(&argv[0][2], "as"))
                    {
                        asmType = ASMTYPE_GAS;
                    }
                    else if (strstr(&argv[0][2], "gcc"))
                    {
                        asmType = ASMTYPE_GAS;
                    }
                    else if (strstr(&argv[0][2], "ps2cc"))
                    {
                        asmType = ASMTYPE_GAS;
                    }
                    break;
                }
                default:
                {
                    break;
                }
            }
        }

        /* Next */
        argv++;
        argc--;
    }

    if (!inFile)
    {
        usage();
        fprintf(stderr, "FINDSYMS: No input file specified\n");
        return (-1);
    }

    if (!outFile)
    {
        usage();
        fprintf(stderr, "FINDSYMS: No output file specified\n");
        return (-1);
    }

    if (asmType == ASMTYPE_NONE)
    {
        usage();
        fprintf(stderr, "FINDSYMS: Asm type not specified\n");
        return (-1);
    }


    if (!findSyms(inFile, outFile, asmType))
    {
        fprintf(stderr, "FINDSYMS: Symbol search failed\n");
        return (1);
    }

    return (0);
}
