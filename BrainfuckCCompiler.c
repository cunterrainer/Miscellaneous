#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#define ARRAY_SIZE  30000
#define OUTPUT_FILE "out.c"

bool CharIsValid(char lookFor)
{
    static const char* validChars = "-+<>[]"; // . & , removed since they are alreay being checked for
    for (size_t i = 0; i < 6; ++i)
        if (validChars[i] == lookFor)
            return true;
    return false;
}


typedef struct
{
    char* sCode;
    size_t size;
    bool hasIO;
} FileParse;


FileParse ReadFile(const char* path)
{
    FileParse file;
    file.sCode = NULL;
    file.size = 0;
    file.hasIO = false;

    FILE* fp = fopen(path, "r");
    if (fp == NULL) {
        fprintf(stderr, "Failed to open file [%s] | Error: %s\n", path, strerror(errno));
        return file;
    }

    fseek(fp, 0L, SEEK_END);
    const size_t fileSize = ftell(fp);
    rewind(fp);

    file.sCode = malloc((fileSize + 1) * sizeof(char));
    if (file.sCode == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for the source code: %lld bytes\n", (uint64_t)(fileSize + 1));
        fclose(fp);
        return file;
    }
    file.sCode[fileSize] = 0;

    char c;
    char* pSourceCode = file.sCode;
    while ((c = (char)fgetc(fp)) != EOF)
    {
        if (c == '.' || c == ',')
        {
            ++file.size;
            file.hasIO = true;
            *pSourceCode++ = c;
        }
        else if (CharIsValid(c))
        {
            ++file.size;
            *pSourceCode++ = c;
        }
    }

    fclose(fp);
    return file;
}


void PrintIndentation(FILE* fp, size_t level)
{
    for (size_t i = 0; i < level; ++i)
        fputc('\t', fp);
}


size_t ChangeValueAtIndex(FILE* fp, char current, char isNot, size_t inRow, size_t indentLevel)
{
    if (current != isNot)
    {
        if (inRow > 0)
        {
            PrintIndentation(fp, indentLevel);
            fprintf(fp, "\tarr[index] %c= %lld;\n", isNot, (uint64_t)inRow);
        }
        return 0;
    }
    return inRow;
}


size_t ChangeIndex(FILE* fp, char current, char isNot, size_t inRow, char operator, size_t indentLevel)
{
    if (current != isNot)
    {
        if (inRow > 0)
        {
            PrintIndentation(fp, indentLevel);
            fprintf(fp, "\tindex %c= %lld;\n", operator, (uint64_t)inRow);
        }
        return 0;
    }
    return inRow;
}


typedef enum
{
    OptLevel_None, OptLevel_O0, OptLevel_O1, OptLevel_O2, OptLevel_O3, OptLevel_Os, OptLevel_Of, OptLevel_Og
} OptLevel;


typedef struct
{
    bool compile;
    bool generateCFile;
    bool endProgram;
    bool linkStatic;
    OptLevel optLevel;
    char* compileCmd;
} Input;


void PrintHelpMessage(const char* process)
{
    puts("Brainfuck Compiler");
    printf("Usage: %s [filepath] [options]\n", process);
    puts("       -h: print this help message");
    puts("       -f: generate the .c file when using the compile option");
    puts("       -s: link statically with libc");
    puts("       -c: compile the C code instead of generating a .c file (using gcc)");
    puts("       -clang: compile using clang\n");
    puts("       Optimization options (Will be passed to the C compiler):");
    puts("       -O0");
    puts("       -O1");
    puts("       -O2");
    puts("       -O3");
    puts("       -Of (corresponds to -Ofast)");
    puts("       -Os");
    puts("       -Og");
}


bool ArgvCmp(const char* isThisStr, const char* str1, const char* str2)
{
    return strcmp(isThisStr, str1) == 0 || strcmp(isThisStr, str2) == 0;
}


Input HandleClArguments(int argc, char** argv)
{
    Input in;
    in.compile = false;
    in.generateCFile = false;
    in.endProgram = true;
    in.optLevel = OptLevel_None;
    in.linkStatic = false;
    in.compileCmd = NULL;

    if (argc == 1) {
        PrintHelpMessage(argv[0]);
        return in;
    }

    for (int i = 2; i < argc; ++i)
    {
        if (ArgvCmp(argv[i], "-h", "-H"))
        {
            PrintHelpMessage(argv[0]);
            return in;
        }

        if (ArgvCmp(argv[i], "-f", "-F"))
            in.generateCFile = true;
        else if (ArgvCmp(argv[i], "-c", "-C"))
        {
            in.compile = true;
            in.compileCmd = "gcc " OUTPUT_FILE " ";
        }
        else if (ArgvCmp(argv[i], "-clang", "-Clang"))
        {
            in.compile = true;
            in.compileCmd = "clang " OUTPUT_FILE " ";
        }
        else if (ArgvCmp(argv[i], "-o0", "-O0"))
            in.optLevel = OptLevel_O0;
        else if (ArgvCmp(argv[i], "-o1", "-O1"))
            in.optLevel = OptLevel_O1;
        else if (ArgvCmp(argv[i], "-o2", "-O2"))
            in.optLevel = OptLevel_O2;
        else if (ArgvCmp(argv[i], "-o3", "-O3"))
            in.optLevel = OptLevel_O3;
        else if (ArgvCmp(argv[i], "-os", "-Os"))
            in.optLevel = OptLevel_Os;
        else if (ArgvCmp(argv[i], "-of", "-Of"))
            in.optLevel = OptLevel_Of;
        else if (ArgvCmp(argv[i], "-og", "-Og"))
            in.optLevel = OptLevel_Og;
        else if (ArgvCmp(argv[i], "-s", "-S"))
            in.linkStatic = true;
        else
        {
            printf("Unknown argument [%s]\n", argv[i]);
            PrintHelpMessage(argv[0]);
            return in;
        }
    }
    in.endProgram = false;
    return in;
}


Input CreateCompileCommand(Input in)
{
    size_t lenCmd = strlen(in.compileCmd);
    const size_t sizeTotal = lenCmd + 7 + 8 + 1;
    // 7 = size of optimization flag, 8 = -static flag, 1 = 0 termination character

    char* buff = calloc(sizeTotal, sizeof(char));
    if (buff == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for compile command: %lld bytes\n", (uint64_t)sizeTotal);
        in.endProgram = true;
        return in;
    }
    memcpy(buff, in.compileCmd, lenCmd);


    switch (in.optLevel)
    {
    case OptLevel_O0:
        strcpy(buff + lenCmd, "-O0 "); lenCmd += 4;
        break;
    case OptLevel_O1:
        strcpy(buff + lenCmd, "-O1 "); lenCmd += 4;
        break;
    case OptLevel_O2:
        strcpy(buff + lenCmd, "-O2 "); lenCmd += 4;
        break;
    case OptLevel_O3:
        strcpy(buff + lenCmd, "-O3 "); lenCmd += 4;
        break;
    case OptLevel_Os:
        strcpy(buff + lenCmd, "-Os "); lenCmd += 4;
        break;
    case OptLevel_Of:
        strcpy(buff + lenCmd, "-Ofast "); lenCmd += 7;
        break;
    case OptLevel_Og:
        strcpy(buff + lenCmd, "-Og "); lenCmd += 4;
        break;
    case OptLevel_None:
        break;
    }

    if (in.linkStatic)
        strcpy(buff + lenCmd, "-static ");
    in.compileCmd = buff;
    return in;
}


Input HandleInput(int argc, char** argv)
{
    Input in = HandleClArguments(argc, argv);
    if (in.compile == false)
        in.generateCFile = true;
    else
        in = CreateCompileCommand(in);
    return in;
}


int CleanUp( char* compileCmd, char* sourceCode, FILE* fp, int returnCode)
{
    if (compileCmd != NULL)
        free(compileCmd);
    if (sourceCode != NULL)
        free(sourceCode);
    if (fp != NULL)
        fclose(fp);
    return returnCode;
}


int main(int argc, char** argv)
{
    const clock_t startTime = clock();

    const Input in = HandleInput(argc, argv);
    if (in.endProgram == true)
        return CleanUp(in.compileCmd, NULL, NULL, 1);

    const FileParse code = ReadFile(argv[1]);
    if (code.sCode == NULL)
        return CleanUp(in.compileCmd, NULL, NULL, 1);

    FILE* fp = fopen(OUTPUT_FILE, "w");
    if (fp == NULL)
    {
        fprintf(stderr, "Failed to open output file [%s]\n", OUTPUT_FILE);
        return CleanUp(in.compileCmd, code.sCode, NULL, 1);
    }
    puts("-- Starting to generate C code");

    if (code.hasIO)
        fputs("#include <stdio.h>\n", fp);
    fputs("#include <stdint.h>\n#include <stdlib.h>\n\n", fp);
    fprintf(fp, "#define ARRAY_SIZE %d\n\n", ARRAY_SIZE);
    fputs("int main()\n{\n\tuint8_t* arr = calloc(ARRAY_SIZE, sizeof(uint8_t));\n\tif (arr == NULL)\n\t{\n\t\tfprintf(stderr, \"Failed to allocate memory: %lld bytes\", (uint64_t)ARRAY_SIZE);\n\t\treturn 1;\n\t}\n\tuint16_t index = 0;\n\n", fp);
    

    size_t incrementValueInRow = 0;
    size_t decrementValueInRow = 0;
    size_t incrementIndexInRow = 0;
    size_t decrementIndexInRow = 0;
    size_t currentIndentation  = 0;
    size_t openLoops = 0;
    size_t lastOpenBrackedPos = 0;
    bool error = false;

    for (size_t i = 0; i < code.size; ++i)
    {
        const char currentChar = code.sCode[i];
        incrementValueInRow = ChangeValueAtIndex(fp, currentChar, '+', incrementValueInRow, currentIndentation);
        decrementValueInRow = ChangeValueAtIndex(fp, currentChar, '-', decrementValueInRow, currentIndentation);
        
        incrementIndexInRow = ChangeIndex(fp, currentChar, '>', incrementIndexInRow, '+', currentIndentation);
        decrementIndexInRow = ChangeIndex(fp, currentChar, '<', decrementIndexInRow, '-', currentIndentation);


        switch (currentChar)
        {
        case '+':
            ++incrementValueInRow;
            break;
        case '-':
            ++decrementValueInRow;
            break;
        case '>':
            ++incrementIndexInRow;
            break;
        case '<':
            ++decrementIndexInRow;
            break;
        case ',':
            PrintIndentation(fp, currentIndentation);
            fputs("\tarr[index] = getchar(); fflush(stdout);\n", fp);
            break;
        case '.':
            PrintIndentation(fp, currentIndentation);
            fputs("\tputchar(arr[index]);\n", fp);
            break;
        case '[':
            lastOpenBrackedPos = i + 1;
            fputc('\n', fp);
            PrintIndentation(fp, currentIndentation);
            fputs("\twhile (arr[index])\n", fp);
            PrintIndentation(fp, currentIndentation);
            fputs("\t{\n", fp);
            ++currentIndentation;
            ++openLoops;
            break;
        case ']':
            if (openLoops == 0)
            {
                fprintf(stderr, "Syntax error: missing '[' for closing bracket (']') in position %llu\n", (uint64_t)i+1);
                goto CLEANUP_ERROR;
            }

            PrintIndentation(fp, currentIndentation);
            fputs("}\n\n", fp);
            --currentIndentation;
            --openLoops;
            break;
        default:
            fprintf(stderr, "\nNon valid char found [%c, %d]\n", currentChar, (int)currentChar);
            break;
        }
    }
    if (openLoops > 0)
    {
        fprintf(stderr, "\nSyntax error: missing ']' for opening bracket('[') in position %llu\n", (uint64_t)lastOpenBrackedPos);
        goto CLEANUP_ERROR;
    }

    ChangeValueAtIndex(fp, ' ', '+', incrementValueInRow, currentIndentation);
    ChangeValueAtIndex(fp, ' ', '-', decrementValueInRow, currentIndentation);
    ChangeIndex(fp, ' ', '>', incrementIndexInRow, '+', currentIndentation);
    ChangeIndex(fp, ' ', '<', decrementIndexInRow, '-', currentIndentation);

    goto CLEANUP;
    CLEANUP_ERROR:
    error = true;

    CLEANUP:
    fputs("\n\tfree(arr);\n\treturn 0;\n}", fp);
    fclose(fp);

    if (error == false)
    {
        puts("-- Done");
        if (in.compile == true)
        {
            printf("-- Compiling: %s\n", in.compileCmd);
            system(in.compileCmd);
            puts("-- Done");
        }
    }
    CleanUp(in.compileCmd, code.sCode, NULL, 0);

    if ((error || in.generateCFile == false) && remove(OUTPUT_FILE) != 0)
        fprintf(stderr, "Failed to delete file [%s] | Error: %s\n", OUTPUT_FILE, strerror(errno));

    const clock_t endTime = clock();
    const double timeSpent = (double)(endTime - startTime) / CLOCKS_PER_SEC;
    printf("Execution time: %.2lf sec(s)\n", timeSpent);
}