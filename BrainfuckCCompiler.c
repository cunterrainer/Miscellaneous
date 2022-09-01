#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#define ARRAY_SIZE  30000
#define VALID_CHARS "-+<>[],."
#define OUTPUT_FILE "out.c"

bool CharIsValid(char lookFor)
{
    // 9 is the size of VALID_CHARS
    for (size_t i = 0; i < 9; ++i)
        if (VALID_CHARS[i] == lookFor)
            return true;
    return false;
}


typedef struct
{
    char* code;
    size_t size;
    bool hasInput;
    bool hasOutput;
    bool hasIncrement;
    bool hasDecrement;
} FileParse;


FileParse ReadFile(const char* path)
{
    FileParse file;
    file.code = NULL;
    file.size = 0;
    file.hasInput = false;
    file.hasOutput = false;
    file.hasIncrement = false;
    file.hasDecrement = false;

    FILE* fp = fopen(path, "r");
    if (fp == NULL) {
        fprintf(stderr, "Failed to open file [%s] | Error: %s\n", path, strerror(errno));
        return file;
    }

    char c;
    size_t numOfchars = 0;
    while ((c = (char)fgetc(fp)) != EOF)
    {
        if (CharIsValid(c) == true)
        {
            if (c == '.')
                file.hasOutput = true;
            else if (c == ',')
                file.hasInput = true;
            else if (c == '+')
                file.hasIncrement = true;
            else if (c == '-')
                file.hasDecrement = true;
            ++numOfchars;
        }
    }
    if (numOfchars == 0)
    {
        fclose(fp);
        return file;
    }

    size_t index = 0;
    file.code = malloc((numOfchars + 1) * sizeof(char));
    file.code[numOfchars] = 0;

    fseek(fp, 0, SEEK_SET);
    while ((c = (char)fgetc(fp)) != EOF)
    {
        if (CharIsValid(c) == true)
        {
            file.code[index] = c;
            ++index;
        }
    }

    fclose(fp);
    file.size = numOfchars;
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


size_t ChangeIndex(FILE* fp, char current, char isNot, size_t inRow, const char* function, size_t indentLevel)
{
    if (current != isNot)
    {
        if (inRow > 0)
        {
            PrintIndentation(fp, indentLevel);
            fprintf(fp, "\tindex = %s(index, %lld);\n", function, (uint64_t)inRow);
        }
        return 0;
    }
    return inRow;
}


typedef struct
{
    bool compile;
    bool generateCFile;
    bool endProgram;
    bool useOptimizations;
    bool linkStatic;
    char* compileCmd;
} Input;


void PrintHelpMessage(const char* process)
{
    fputs("Brainfuck Compiler\n", stdout);
    printf("Usage: %s [filepath] [options]\n", process);
    fputs("       -h: print this help message\n", stdout);
    fputs("       -f: generate the .c file when using the compile option\n", stdout);
    fputs("       -s: link statically with libc\n", stdout);
    fputs("       -o: compile C code with optimizations\n", stdout);
    fputs("       -c: compile the C code instead of generating a .c file (using gcc)\n", stdout);
    fputs("       -clang: compile using clang\n", stdout);
}


Input HandleInput(int argc, char** argv)
{
    Input in;
    in.compile = false;
    in.generateCFile = false;
    in.endProgram = true;
    in.useOptimizations = false;
    in.linkStatic = false;
    in.compileCmd = NULL;

    if (argc == 1) {
        PrintHelpMessage(argv[0]);
        return in;
    }
    for (int i = 2; i < argc; ++i)
    {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "-H") == 0)
        {
            PrintHelpMessage(argv[0]);
            return in;
        }

        if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "-F") == 0)
            in.generateCFile = true;
        else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "-C") == 0)
        {
            in.compile = true;
            in.compileCmd = "gcc " OUTPUT_FILE " ";
        }
        else if (strcmp(argv[i], "-clang") == 0 || strcmp(argv[i], "-Clang") == 0)
        {
            in.compile = true;
            in.compileCmd = "clang " OUTPUT_FILE " ";
        }
        else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "-O") == 0)
            in.useOptimizations = true;
        else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "-S") == 0)
            in.linkStatic = true;
        else
        {
            printf("Unknown argument [%s]\n", argv[i]);
            PrintHelpMessage(argv[0]);
            return in;
        }
    }

    if (in.compile == false)
        in.generateCFile = true;
    else
    {
        size_t lenCmd = strlen(in.compileCmd);
        size_t sizeTotal = lenCmd + 4 + 8 + 1;
        char* buff = calloc(sizeTotal, sizeof(char));
        if (buff == NULL)
        {
            fprintf(stderr, "Failed to alloca memory for compile command: %lld bytes\n", (uint64_t)sizeTotal);
            return in;
        }
        memcpy(buff, in.compileCmd, lenCmd);

        if (in.useOptimizations)
        {
            strcpy(buff + lenCmd, "-O2 ");
            lenCmd += 4;
        }
        if (in.linkStatic)
            strcpy(buff + lenCmd, "-static ");
        in.compileCmd = buff;
    }
    in.endProgram = false;
    return in;
}


int main(int argc, char** argv)
{
    const clock_t startTime = clock();
    const Input in = HandleInput(argc, argv);
    if (in.endProgram == true)
        return 1;

    const FileParse code = ReadFile(argv[1]);
    if (code.code == NULL)
    {
        if(in.compileCmd != NULL)
            free(in.compileCmd);
        return 1;
    }

    FILE* fp = fopen(OUTPUT_FILE, "w");
    if (fp == NULL)
    {
        fprintf(stderr, "Failed to open output file [%s]\n", OUTPUT_FILE);
        return 1;
    }
    fputs("-- Starting to generate C code\n", stdout);

    if (code.hasInput || code.hasOutput)
        fprintf(fp, "#include <stdio.h>\n");
    fprintf(fp, "#include <stdint.h>\n#include <stdlib.h>\n\n");
    fprintf(fp, "#define ARRAY_SIZE %d\n\n", ARRAY_SIZE);
    if(code.hasIncrement)
        fprintf(fp, "uint16_t IncrementIndex(size_t index, size_t toAdd)\n{\n\tsize_t tmp = index + toAdd;\n\tif (tmp >= ARRAY_SIZE)\n\t{\n\t\ttmp = ARRAY_SIZE - 1 - index;\n\t\ttoAdd -= tmp;\n\t\treturn toAdd;\n\t}\n\treturn tmp;\n}\n\n");
    if(code.hasDecrement)
        fprintf(fp, "uint16_t DecrementIndex(size_t index, size_t toSub)\n{\n\tint64_t tmp = index - toSub;\n\tif (tmp < 0)\n\t{\n\t\ttoSub -= index;\n\t\treturn ARRAY_SIZE - 1 - toSub;\n\t\t}\n\treturn tmp;\n}\n\n");
    fprintf(fp, "int main()\n{\n\tuint8_t* arr = calloc(ARRAY_SIZE, sizeof(uint8_t));\n\tif (arr == NULL)\n\t{\n\t\tfprintf(stderr, \"Failed to allocate memory: %%lld bytes\", (uint64_t)ARRAY_SIZE);\n\t\treturn 1;\n\t}\n\tuint16_t index = 0;\n\n");
    

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
        const char currentChar = code.code[i];
        incrementValueInRow = ChangeValueAtIndex(fp, currentChar, '+', incrementValueInRow, currentIndentation);
        decrementValueInRow = ChangeValueAtIndex(fp, currentChar, '-', decrementValueInRow, currentIndentation);

        incrementIndexInRow = ChangeIndex(fp, currentChar, '>', incrementIndexInRow, "IncrementIndex", currentIndentation);
        decrementIndexInRow = ChangeIndex(fp, currentChar, '<', decrementIndexInRow, "DecrementIndex", currentIndentation);


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
            fprintf(fp, "\tarr[index] = getchar(); fflush(stdout);\n");
            break;
        case '.':
            PrintIndentation(fp, currentIndentation);
            fprintf(fp, "\tfputc((int)arr[index], stdout);\n");
            break;
        case '[':
            lastOpenBrackedPos = i + 1;
            fputc('\n', fp);
            PrintIndentation(fp, currentIndentation);
            fprintf(fp, "\twhile (arr[index] != 0)\n");
            PrintIndentation(fp, currentIndentation);
            fprintf(fp, "\t{\n");
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
            fprintf(fp, "}\n\n");
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
    ChangeIndex(fp, ' ', '>', incrementIndexInRow, "IncrementIndex", currentIndentation);
    ChangeIndex(fp, ' ', '<', decrementIndexInRow, "DecrementIndex", currentIndentation);

    goto CLEANUP;
    CLEANUP_ERROR:
    error = true;

    CLEANUP:
    fprintf(fp, "\n\tfree(arr);\n\treturn 0;\n}");
    fclose(fp);
    free(code.code);

    if (error == false)
    {
        fputs("-- Done\n", stdout);
        if (in.compile == true)
        {
            printf("-- Compiling: %s\n", in.compileCmd);
            system(in.compileCmd);
            fputs("-- Done\n", stdout);
        }
    }
    if(in.compileCmd != NULL)
        free(in.compileCmd);

    if ((error || in.generateCFile == false) && remove(OUTPUT_FILE) != 0)
        fprintf(stderr, "Failed to delete file [%s] | Error: %s\n", OUTPUT_FILE, strerror(errno));

    const clock_t endTime = clock();
    const double timeSpent = (double)(endTime - startTime) / CLOCKS_PER_SEC;
    printf("Execution time: %.2lf sec(s)\n", timeSpent);
}