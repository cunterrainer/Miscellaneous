#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

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


int main(int argc, char** argv)
{
    if (argc == 1)
    {
        fprintf(stderr, "Usage: %s [filepath]\n", argv[0]);
        return 1;
    }

    FileParse code = ReadFile(argv[1]);
    if (code.code == NULL)
        return 1;

    FILE* fp = fopen(OUTPUT_FILE, "w");
    if (fp == NULL)
    {
        fprintf(stderr, "Failed to open output file [%s]\n", OUTPUT_FILE);
        return 1;
    }
    if (code.hasInput || code.hasOutput)
        fprintf(fp, "#include <stdio.h>\n");
    fprintf(fp, "#include <stdint.h>\n#include <stdlib.h>\n\n");
    fprintf(fp, "#define ARRAY_SIZE %d\n\n", ARRAY_SIZE);
    if(code.hasIncrement)
        fprintf(fp, "uint16_t IncrementIndex(size_t index, size_t toAdd)\n{\n\tsize_t tmp = index + toAdd;\n\tif (tmp >= ARRAY_SIZE)\n\t{\n\t\ttmp = ARRAY_SIZE - 1 - index;\n\t\ttoAdd -= tmp;\n\t\treturn toAdd;\n\t}\n\treturn tmp;\n}\n\n");
    if(code.hasDecrement)
        fprintf(fp, "uint16_t DecrementIndex(size_t index, size_t toSub)\n{\n\tint64_t tmp = index - toSub;\n\tif (tmp < 0)\n\t{\n\t\ttoSub -= index;\n\t\treturn ARRAY_SIZE - 1 - toSub;\n\t\t}\n\treturn tmp;\n}\n\n");
    if(code.hasInput)
        fprintf(fp, "uint8_t GetUserInput()\n{\n\tchar buff[2];\n\tfgets(buff, sizeof(buff), stdin);\n\tfseek(stdin, 0, SEEK_END);\n\treturn buff[0] == '\\n' ? 0 : buff[0];\n}\n\n");
    fprintf(fp, "int main()\n{\n\tuint8_t* arr = calloc(ARRAY_SIZE, sizeof(uint8_t));\n\tuint16_t index = 0;\n\n");
    

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
            fprintf(fp, "\tarr[index] = GetUserInput();\n");
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
    if (error && remove(OUTPUT_FILE) != 0)
        fprintf(stderr, "Failed to delete file [%s] | Error: %s\n", OUTPUT_FILE, strerror(errno));
}