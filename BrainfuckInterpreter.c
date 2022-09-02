#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARRAY_SIZE  30000

uint8_t CharIsValid(char lookFor)
{
    static const char* validChars = "-+<>[],.";
    for (size_t i = 0; i < 8; ++i)
        if (validChars[i] == lookFor)
            return 1;
    return 0;
}


char* ReadFile(const char* path)
{
    FILE* fp = fopen(path, "r");
    if (fp == NULL) 
    {
        fprintf(stderr, "Failed to open file [%s]\n", path);
        return NULL;
    }

    fseek(fp, 0L, SEEK_END);
    const size_t fileSize = ftell(fp);
    rewind(fp);

    char* sourceCode = malloc((fileSize + 2) * sizeof(char));
    if (sourceCode == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for the source code: %lld bytes\n", (uint64_t)(fileSize + 2));
        fclose(fp);
        return NULL;
    }
    sourceCode[0] = 0;
    sourceCode[fileSize] = 0;

    char c;
    char* pSourceCode = sourceCode + 1;
    while ((c = (char)fgetc(fp)) != EOF)
    {
        if (CharIsValid(c))
            *pSourceCode++ = c;
    }

    fclose(fp);
    return sourceCode;
}


char HandleInput(int argc, char** argv)
{
    if (argc == 1 || argc > 2 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "-H") == 0)
    {
        puts("Brainfuck Interpreter");
        printf("Usage: %s [filepath]\n", argv[0]);
        puts("       -h: print this help message\n");
        return 0;
    }
    return 1;
}


char* AdvanceToEnd(char* sCode)
{
    size_t openings = 1;
    for (; openings && *sCode; sCode++)
    {
        openings += *sCode == '[';
        openings -= *sCode == ']';
    }
    if (openings > 0)
    {
        fputs("\nSyntax error: missing ']' for opening bracket('[')\n", stderr);
        return NULL;
    }
    return sCode;
}


char* ResetToBegin(char* sCode)
{
    sCode -= 2;
    size_t closings = 1;
    for (; closings && *sCode; sCode--)
    {
        closings += *sCode == ']';
        closings -= *sCode == '[';
    }
    if (closings > 0)
    {
        fputs("\nSyntax error: missing '[' for closing bracket(']')\n", stderr);
        return NULL;
    }
    return sCode + 2;
}


void InterpretCode(char* sCode, uint8_t* arr)
{
    uint16_t index = 0; // using an index instead of pointer since it turned out to be faster

    while (*sCode)
    {
        switch (*sCode++)
        {
        case '+':
            ++arr[index];
            break;
        case '-':
            --arr[index];
            break;
        case '<':
            --index;
            break;
        case '>':
            ++index;
            break;
        case ',':
            arr[index] = (uint8_t)getchar();
            fflush(stdout);
            break;
        case '.':
            putchar(arr[index]);
            break;
        case '[':
            if (arr[index] == 0)
            {
                sCode = AdvanceToEnd(sCode);
                if (sCode == NULL) return;
            }
            break;
        case ']':
            if (arr[index] != 0)
            {
                sCode = ResetToBegin(sCode);
                if (sCode == NULL) return;
            }
            break;
        }
    }
}


int main(int argc, char** argv)
{
    if (!HandleInput(argc, argv))
        return 1;

    uint8_t* allocArray = calloc(ARRAY_SIZE, sizeof(uint8_t));
    if (allocArray == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for the internal array: %lld bytes\n", (uint64_t)ARRAY_SIZE);
        return 1;
    }

    char* sourceCode = ReadFile(argv[1]);
    if (sourceCode == NULL)
    {
        free(allocArray);
        return 1;
    }

    InterpretCode(sourceCode + 1, allocArray);

    free(sourceCode);
    free(allocArray);
}