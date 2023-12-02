#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

size_t g_LastPressedKey = 0;

#ifdef _WIN32
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


    void elapse(size_t milliseconds)
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
#elif defined(__linux__)
    #include <fcntl.h>
    #include <errno.h>
    #include <unistd.h>
    #include <bits/time.h>
    #include <linux/input.h>

    #define COMMAND_BUFFER_SIZE 10

    bool execute_command(const char *cmd, char* result)
    {
        FILE *pipe = popen(cmd, "r");
        if(!pipe)
        {
            printf("Failed to open the command pipe: %s\n", strerror(errno));
            return false; 
        }

        char buffer[COMMAND_BUFFER_SIZE] = { 0 };
      
        while (!feof(pipe))
        {
            if (fgets(buffer, COMMAND_BUFFER_SIZE, pipe) != NULL)
            {
                memcpy(result, buffer, COMMAND_BUFFER_SIZE);
            }
        }

        pclose(pipe);
        return true;
    }


    bool get_keyboard_device_path(char* path)
    {
        static const char* command_get_keyboard_device_path =
            "grep -E 'Handlers|EV=' /proc/bus/input/devices |"
            "grep -B1 'EV=120013' |"
            "grep -Eo 'event[0-9]+' |"
            "grep -Eo '[0-9]+' |"
            "tr -d '\n'";

        const char* event_path = "/dev/input/event";
        const size_t event_path_size = strlen(event_path);
        char device_number[COMMAND_BUFFER_SIZE];

        if (!execute_command(command_get_keyboard_device_path, device_number)) return false;
        memcpy(path, "/dev/input/event", event_path_size);
        memcpy(path + event_path_size, device_number, COMMAND_BUFFER_SIZE);
        return true;
    }


    int g_KeyboardFd = 0;
    bool init_keyboard_listener()
    {
        char path[100];
        if (!get_keyboard_device_path(path)) return false;

        g_KeyboardFd = open(path, O_RDONLY);
        if (g_KeyboardFd < 0)
        {
            printf("Failed to open keyboard device path '%s': %s\nDid you start the program as root?\n", path, strerror(errno));
            return false;
        }

        const int flags = fcntl(g_KeyboardFd, F_GETFL, 0);
        fcntl(g_KeyboardFd, F_SETFL, flags | O_NONBLOCK);
        return true;
    }


    void close_keyboard_listener()
    {
        close(g_KeyboardFd);
    }

    
    void query_keys()
    {
        struct input_event event;
        if (read(g_KeyboardFd, &event, sizeof(event)) > 0)
        {
            if (event.type == EV_KEY && event.value == 1)
            {
                g_LastPressedKey = event.code;
            }
        }
    }
#elif defined(__APPLE__)
    #include <Carbon/Carbon.h>

    #define KEY_W     kVK_ANSI_W
    #define KEY_A     kVK_ANSI_A
    #define KEY_S     kVK_ANSI_S
    #define KEY_D     kVK_ANSI_D
    #define KEY_UP    kVK_UpArrow
    #define KEY_LEFT  kVK_LeftArrow
    #define KEY_DOWN  kVK_DownArrow
    #define KEY_RIGHT kVK_RightArrow
    #define KEY_ESC   kVK_Escape
    #define KEY_SPACE kVK_Space

    #define KEY_PRESSED(keycode) CGEventSourceKeyState(kCGEventSourceStateCombinedSessionState, keycode)

    void query_keys()
    {
        if (KEY_PRESSED(KEY_W) || KEY_PRESSED(KEY_UP))
        {
            g_LastPressedKey = KEY_W;
        }
        if (KEY_PRESSED(KEY_A) || KEY_PRESSED(KEY_LEFT))
        {
            g_LastPressedKey = KEY_A;
        }
        if (KEY_PRESSED(KEY_S) || KEY_PRESSED(KEY_DOWN))
        {
            g_LastPressedKey = KEY_S;
        }
        if (KEY_PRESSED(KEY_D) || KEY_PRESSED(KEY_RIGHT))
        {
            g_LastPressedKey = KEY_D;
        }
        if (KEY_PRESSED(KEY_SPACE))
        {
            g_LastPressedKey = KEY_SPACE;
        }
        if (KEY_PRESSED(KEY_ESC))
        {
            g_LastPressedKey = KEY_ESC;
        }
    }
#endif

#if defined(__linux__) || defined(__APPLE__)
    void elapse(size_t milliseconds)
    {
        struct timespec start, end;
        clock_gettime(CLOCK_REALTIME, &start);

        double elapsedTime = 0.0;
        while (elapsedTime < milliseconds)
        {
            query_keys();
            clock_gettime(CLOCK_REALTIME, &end);
            elapsedTime = ((end.tv_sec - start.tv_sec) * 1.0e9 + (end.tv_nsec - start.tv_nsec)) / 1000000;
        }
    }
#endif


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
    #ifdef _WIN32
        system("cls");
    #elif defined(__linux__) || defined(__APPLE__)
        system("clear");
    #endif
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
        b->board[s.body[i].y * b->width + s.body[i].x] = i == 0 ? 'o' : '~';
    }
}


void board_add_apple(Board* b, Tile apple)
{
    b->board[apple.y * b->width + apple.x] = 'a';
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
    case KEY_UP:
        newHeadPos.y--;
        break;
    case KEY_A:
    case KEY_LEFT:
        newHeadPos.x--;
        break;
    case KEY_S:
    case KEY_DOWN:
        newHeadPos.y++;
        break;
    case KEY_D:
    case KEY_RIGHT:
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


void print_help(const char* name, size_t width, size_t height, size_t timeToWait)
{
    printf("Usage: %s [options]\n", name);
    printf("  -h  | --help            Print this help message\n");
    printf("  -w  | --width=<value>   Set the board width (min: 4, default: %zu)\n", width);
    printf("  -h  | --hight=<value>   Set the board hight (min: 4, default: %zu)\n", height);
    printf("  -s  | --sleep=<value>   Time to wait between moving in milliseconds (min: 1, default: %zu)\n", timeToWait);
    puts("\nIt's recommended for the width to be twice the size of the height");

    #ifdef __linux__
        puts("Linux users have to run this program with root privileges");
    #endif

    puts("Controlls:");
    puts("- Esc                 Pause");
    puts("- Space               Quit");
    puts("- W|A|S|D             Move");
    puts("- Up|Left|Down|Right  Move");
}


bool parse_args(int argc, char** argv, long* width, long* height, long* timeToWait)
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
            if (*timeToWait < 1)
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
    #ifdef __linux__
        if (!init_keyboard_listener()) return EXIT_FAILURE;
    #endif

    g_LastPressedKey = KEY_ESC;
    size_t timeToWait = 200;
    Board board = { .width = 20, .height = 10, .board = NULL };
    if (!parse_args(argc, argv, (long*)&board.width, (long*)&board.height, (long*)&timeToWait)) return EXIT_SUCCESS;

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
        elapse(timeToWait);
    }

    free(snake.body);
    free(board.board);
    #ifdef __linux__
        close_keyboard_listener();
    #endif
    return EXIT_SUCCESS;
}
