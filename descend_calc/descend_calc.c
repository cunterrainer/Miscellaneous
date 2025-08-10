#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int get_input(const char* msg)
{
    while (1)
    {
        printf(msg);

        char input[12]; // int is max -> 1 2 billion is 10 digits + negative sign + extra space for \0
        fgets(input, sizeof(input), stdin);

        // if character left in stdin
        if (strchr(input, '\n') == NULL)
        {
            // Read until newline or EOF, flush stdin
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
        }

        input[sizeof(input) - 1] = '\0';
        input[strcspn(input, "\n")] = '\0';
        if (strlen(input) == 0) continue;

        char* end;
        const long result = strtol(input, &end, 10);
        if (*end == '\0') return result;
    }
}

int main()
{
    const int current_altitude = get_input("Current altitude: ");
    const int airport_altitude = get_input("Airport altitude: ");

    const float altitude_to_lose = ((current_altitude - airport_altitude) / 100.f) / 3.f;

    const int current_speed = get_input("Current speed: ");
    const int avg_dsc_speed = get_input("Average descend speed: ");

    const float speed_knots = (current_speed - avg_dsc_speed) / 10.f;

    const float wind_speed = get_input("Wind speed: ") / 10.f;
    const int tail_wind = get_input("Tail or head wind [1 for tail | 0 for head]: ");
    
    const float descend_distance = altitude_to_lose + speed_knots + (tail_wind == 1 ? wind_speed : -wind_speed);

    puts("------------------------------");
    printf("Altitude to lose nm: %g\n", altitude_to_lose);
    printf("Speed nm: %g\n", speed_knots);
    printf("Wind  nm: %g\n", wind_speed);
    printf("Total distance: %g\n", descend_distance);
    return 0;
}