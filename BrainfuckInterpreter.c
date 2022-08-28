#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define ARRAY_SIZE  30000
#define VALID_CHARS "-+<>[],."

uint8_t CharIsValid(char lookFor)
{
    // 9 is the size of VALID_CHARS
    for (size_t i = 0; i < 9; ++i)
        if (VALID_CHARS[i] == lookFor)
            return 1;
    return 0;
}


char* ReadFile(const char* path, size_t* codeSize)
{
    FILE* fp = fopen(path, "r");
    if (fp == NULL) {
        fprintf(stderr, "\nFailed to open file [%s]\n", path);
        return NULL;
    }

    char c;
    size_t numOfchars = 0;
    while ((c = (char)fgetc(fp)) != EOF)
    {
        if (CharIsValid(c) == 1)
            ++numOfchars;
    }
    if (numOfchars == 0)
        return NULL;

    size_t index = 0;
    char* result = malloc((numOfchars + 1) * sizeof(char));
    result[numOfchars] = 0;

    fseek(fp, 0, SEEK_SET);
    while ((c = (char)fgetc(fp)) != EOF)
    {
        if (CharIsValid(c) == 1)
        {
            result[index] = c;
            ++index;
        }
    }

    fclose(fp);
    *codeSize = numOfchars;
    return result;
}


int main()
{
    uint8_t* arr = calloc(ARRAY_SIZE, sizeof(char));
    size_t codeSize = 0;
    char* sourceCode = ReadFile("test.bf", &codeSize);
    printf("%s\n%s\n", sourceCode, &sourceCode[100]);
    char buff[2]; // 2 because of '\n'
    if (sourceCode == NULL) goto CLEANUP;

    uint16_t currentCell = 0;
    size_t openLoopIndex = codeSize;
    size_t nestedLevel = 0;

    for (size_t i = 0; i < codeSize; ++i)
    {
        switch (sourceCode[i])
        {
        case '+':
            ++arr[currentCell];
            break;
        case '-':
            --arr[currentCell];
            break;
        case '<':
            currentCell = currentCell == 0 ? ARRAY_SIZE - 1 : currentCell - 1;
            break;
        case '>':
            currentCell = currentCell == ARRAY_SIZE - 1 ? 0 : currentCell + 1;
            break;
        case '[':
            ++nestedLevel;
            openLoopIndex = i;
            if (arr[currentCell] == 0) // move to end of loop
            {
                size_t numOfOpenings = 0;
                for (size_t k = i + 1; k < codeSize; ++k)
                {
                    if (sourceCode[k] == '[')
                        ++numOfOpenings;
                    else if (sourceCode[k] == ']')
                    {
                        if(numOfOpenings == 0)
                        {
                            i = k;
                            openLoopIndex = codeSize;
                            break;
                        }
                        numOfOpenings = numOfOpenings == 0 ? 0 : numOfOpenings - 1;
                    }
                    else if (k == codeSize - 1)
                    {
                        fprintf(stderr, "\nSyntax error: missing ']' for opening bracket('[') in position [%lld]\n", (uint64_t)i+1);
                        i = codeSize;
                    }
                }
            }
            break;
        case ']':
            if (openLoopIndex == codeSize && nestedLevel == 0)
            {
                fprintf(stderr, "\nSyntax error: missing '[' for closing bracket(']') in position [%lld]\n", (uint64_t)i+1);
                i = codeSize;
                break;
            }
            else if (openLoopIndex == codeSize)
            {
                size_t numOfClosings = 0;
                for (int32_t k = i-1; k >= 0; --k)
                {
                    if (sourceCode[k] == ']')
                        ++numOfClosings;
                    else if (sourceCode[k] == '[')
                    {
                        if (numOfClosings == 0)
                        {
                            openLoopIndex = k;
                            break;
                        }
                        numOfClosings = numOfClosings == 0 ? 0 : numOfClosings - 1;
                    }
                }
            }
            --nestedLevel;
            i = openLoopIndex - 1;
            openLoopIndex = codeSize;
            break;
        case ',':
            fgets(buff, sizeof(buff), stdin);
            arr[currentCell] = buff[0];
            fseek(stdin, 0, SEEK_END);
            break;
        case '.':
            fputc((int)arr[currentCell], stdout);
            break;
        default:
            fprintf(stderr, "\nForbidden char found [%c, %d]\n", sourceCode[i], (int)sourceCode[i]);
            break;
        }
        //printf("%d\n", i);
    }
    if(openLoopIndex != codeSize)
        fprintf(stderr, "\nSyntax error: missing ']' for opening bracket('[') in position [%lld]\n", (uint64_t)openLoopIndex + 1);

CLEANUP:
    fflush(stdout);
    free(sourceCode);
    free(arr);
    fgetc(stdin);
}