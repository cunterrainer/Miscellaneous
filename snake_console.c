#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <Windows.h>

#define KEY_W     0x57
#define KEY_A     0x41
#define KEY_S     0x53
#define KEY_D     0x44
#define KEY_UP    VK_UP
#define KEY_LEFT  VK_LEFT
#define KEY_DOWN  VK_DOWN
#define KEY_RIGHT VK_RIGHT
#define KEY_ESC   VK_ESCAPE
#define KEY_SPACE VK_SPACE

#define KEY_PRESSED(keystate) ((1 << 15) & keystate)
#define MAX(a, b) (((a) > (b)) ? (a) : (b))


size_t g_LastPressedKey = KEY_ESC;
void query_keys()
{
    if (KEY_PRESSED(GetAsyncKeyState(KEY_W) || KEY_PRESSED(GetAsyncKeyState(KEY_UP))))
    {
        g_LastPressedKey = KEY_W;
    }
    if (KEY_PRESSED(GetAsyncKeyState(KEY_A) || KEY_PRESSED(GetAsyncKeyState(KEY_LEFT))))
    {
        g_LastPressedKey = KEY_A;
    }
    if (KEY_PRESSED(GetAsyncKeyState(KEY_S) || KEY_PRESSED(GetAsyncKeyState(KEY_DOWN))))
    {
        g_LastPressedKey = KEY_S;
    }
    if (KEY_PRESSED(GetAsyncKeyState(KEY_D) || KEY_PRESSED(GetAsyncKeyState(KEY_RIGHT))))
    {
        g_LastPressedKey = KEY_D;
    }
    if (KEY_PRESSED(GetAsyncKeyState(KEY_SPACE)))
    {
        g_LastPressedKey = KEY_SPACE;
    }
    if (KEY_PRESSED(GetAsyncKeyState(KEY_ESC)))
    {
        g_LastPressedKey = KEY_ESC;
    }
}


typedef struct
{
    size_t width;
    size_t height;
    char* board;
} Board;


typedef struct
{
    size_t x;
    size_t y;
} Tile;


typedef struct
{
    Tile* body;
    size_t size;
} Snake;


void board_reset(Board* b)
{
    memset(b->board, ' ', b->width * b->height);
    memset(b->board, '#', b->width);
    memset(&b->board[b->width * b->height - b->width], '#', b->width);

    for (size_t i = 1; i < b->height; ++i)
    {
        b->board[i*b->width] = '#';
        b->board[i*b->width - 1] = '#';
    }
}


void board_print(const Board* b, size_t score)
{
    system("cls");
    printf("Score: %zu\n", score);

    const size_t size = b->width * b->height;
    for (size_t i = 0; i < size; ++i)
    {
        putc(b->board[i], stdout);
        if ((i + 1) % b->width == 0) putc('\n', stdout);
    }
}


void board_add_snake(Board* b, Snake s)
{
    for (size_t i = 0; i < s.size; ++i)
    {
        b->board[s.body[i].y * b->width + s.body[i].x] = i == 0 ? 'H' : 'S';
    }
}


void board_add_apple(Board* b, Tile apple)
{
    b->board[apple.y * b->width + apple.x] = 'A';
}


size_t board_empty_tiles(Board b)
{
    size_t empty = 0;
    for (size_t i = 0; i < b.width * b.height; ++i)
    {
        if (b.board[i] == ' ')
            empty++;
    }
    return empty;
}


Tile board_get_empty_tile(Board b, size_t num)
{
    Tile t = { .x = 0, .y = 0 };
    size_t empty = 0;
    for (size_t i = 0; i < b.width * b.height; ++i)
    {
        if (b.board[i] == ' ')
        {
            empty++;
            if (empty == num)
            {
                t.x = i % b.width;
                t.y = i / b.width;
            }
        }
    }

    assert(t.x > 0 && t.y > 0 && "Error, not a valid empty tile");
    return t;
}


bool snake_move(Snake* s)
{
    query_keys();
    Tile newHeadPos = s->body[0];
    switch (g_LastPressedKey)
    {
    case KEY_W:
        newHeadPos.y--;
        break;
    case KEY_A:
        newHeadPos.x--;
        break;
    case KEY_S:
        newHeadPos.y++;
        break;
    case KEY_D:
        newHeadPos.x++;
        break;
    default:
        return false;
    }

    for (size_t i = s->size; i > 0; --i)
    {
        s->body[i] = s->body[i-1];
    }
    s->body[0] = newHeadPos;
    return true;
}


bool snake_internal_collision(Snake s)
{
    Tile head = s.body[0];
    for (size_t i = 1; i < s.size; ++i)
    {
        if (head.x == s.body[i].x && head.y == s.body[i].y)
            return true;
    }
    return false;
}


size_t get_random_num(size_t min, size_t max)
{
    return (rand() % (max - min + 1)) + min;
}


void wait(size_t milliseconds)
{
    /*
        We don't use the sleep function because then we wouldn't be able to query
        the keys while waiting, resulting in a laggy experience, since the user
        has to press the key in the exact moment we ask for it in snake_move()
    */
    LARGE_INTEGER frequency;        // ticks per second
    LARGE_INTEGER t1, t2;           // ticks

    QueryPerformanceFrequency(&frequency); // get ticks per second
    QueryPerformanceCounter(&t1); // start timer

    double elapsedTime = 0.0;
    while (elapsedTime < milliseconds)
    {
        query_keys();
        QueryPerformanceCounter(&t2);
        elapsedTime = (t2.QuadPart - t1.QuadPart) * 1000.0 / frequency.QuadPart;
    }
}


void print_help(const char* name, size_t width, size_t height, size_t timeToWait)
{
    printf("Usage: %s [options]\n", name);
    printf("  -h  | --help            Print this help message\n");
    printf("  -w  | --width=<value>   Set the board width (min: 4, default: %zu)\n", width);
    printf("  -h  | --hight=<value>   Set the board hight (min: 4, default: %zu)\n", height);
    printf("  -s  | --sleep=<value>   Time to wait between moving in milliseconds (min: 1, default: %zu)\n", timeToWait);
    puts("\nIt's recommended for the width to be twice the size of the height");
    puts("Controlls:");
    puts("- Esc                 Pause");
    puts("- Space               Quit");
    puts("- W|A|S|D             Move");
    puts("- Up|Left|Down|Right  Move");
}


bool parse_args(int argc, char** argv, size_t* width, size_t* height, size_t* timeToWait)
{
    const size_t defaultWidth = *width;
    const size_t defaultHeight = *height;
    const size_t defaultTimeToWait = *timeToWait;

    for (int i = 1; i < argc; ++i)
    {
        const size_t argvSize = strlen(argv[i]);
        for (size_t k = 0; k < argvSize; ++k)
        {
            argv[i][k] = tolower(argv[i][k]);
        }

        if (strcmp("--help", argv[i]) == 0 || strcmp("-h", argv[i]) == 0)
        {
            print_help(argv[0], defaultWidth, defaultHeight, defaultTimeToWait);
            return false;
        }

        else if (strncmp("--width=", argv[i], 8) == 0 || strncmp("-w=", argv[i], 3) == 0)
        {
            char* end;
            const char* result = strchr(argv[i], '=') + 1;
            *width = strtol(result, &end, 10);
            if (*width < 4)
            {
                fprintf(stderr, "Width must be greater than or equal to 4\nTry '--help' for additional help\n");
                return false;
            }
        }

        else if (strncmp("--height=", argv[i], 9) == 0 || strncmp("-h=", argv[i], 3) == 0)
        {
            char* end;
            const char* result = strchr(argv[i], '=') + 1;
            *height = strtol(result, &end, 10);
            if (*height < 4)
            {
                fprintf(stderr, "Height must be greater than or equal to 4\nTry '--help' for additional help\n");
                return false;
            }
        }

        else if (strncmp("--sleep=", argv[i], 8) == 0 || strncmp("-s=", argv[i], 3) == 0)
        {
            char* end;
            const char* result = strchr(argv[i], '=') + 1;
            *timeToWait = strtol(result, &end, 10);
            if (*timeToWait < 4)
            {
                fprintf(stderr, "Sleep must be greater than or equal to 1\nTry '--help' for additional help\n");
                return false;
            }
        }

        else
        {
            fprintf(stderr, "Unknow options '%s'\nTry '--help' for additional help\n", argv[i]);
            return false;
        }
    }
    return true;
}


int main(int argc, char** argv)
{
    size_t timeToWait = 200;
    Board board = { .width = 20, .height = 10, .board = NULL };
    if (!parse_args(argc, argv, &board.width, &board.height, &timeToWait)) return EXIT_SUCCESS;

    board.board = malloc(board.width * board.height * sizeof(char));
    if (board.board == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for the board\n");
        return EXIT_FAILURE;
    }
    board_reset(&board);
    
    Snake snake;
    snake.size = 1;
    snake.body = malloc(board.width * board.height * sizeof(Tile));
    if (snake.body == NULL)
    {
        free(board.board);
        fprintf(stderr, "Failed to allocate memory for the snake\n");
        return EXIT_FAILURE;
    }
    memset(snake.body, 0, board.width * board.height * sizeof(Tile));
    snake.body[0].x = MAX(board.width / 2, 1);
    snake.body[0].y = MAX(board.height / 2, 1);
    board_add_snake(&board, snake);

    srand(time(0));
    Tile apple = board_get_empty_tile(board, get_random_num(1, board_empty_tiles(board)));

    board_add_apple(&board, apple);
    board_print(&board, snake.size - 1);

    while (1)
    {
        if (g_LastPressedKey == KEY_SPACE) break;
        if (!snake_move(&snake)) continue;

        board_reset(&board);
        board_add_snake(&board, snake);

        if (snake.body[0].x == 0 || snake.body[0].x == board.width-1 || snake.body[0].y == 0 || snake.body[0].y == board.height - 1 || snake_internal_collision(snake))
        {
            board_add_apple(&board, apple);
            board_print(&board, snake.size - 1);
            printf("Game over, you lost!\nScore: %zu\n", snake.size - 1);
            break;
        }

        if (apple.x == snake.body[0].x && apple.y == snake.body[0].y)
        {
            snake.size++;
            const size_t freeTiles = board_empty_tiles(board);
            if (freeTiles == 0)
            {
                board_print(&board, snake.size - 1);
                printf("Congratulations, you won!\nScore: %zu\n", snake.size - 1);
                break;
            }
            apple = board_get_empty_tile(board, get_random_num(1, freeTiles));
        }

        board_add_apple(&board, apple);
        board_print(&board, snake.size - 1);
        wait(timeToWait);
    }

    free(snake.body);
    free(board.board);
    return EXIT_SUCCESS;
}
