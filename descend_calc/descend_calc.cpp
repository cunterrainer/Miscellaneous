#include <ios>
#include <limits>
#include <string>
#include <cstdint>
#include <iostream>
#include <string_view>

std::int32_t get_input(std::string_view msg)
{
    while (true)
    {        
        std::cout << msg;
        
        std::string input;
        std::getline(std::cin, input);

        try
        {
            return std::stoi(input);
        }
        catch(const std::exception&) {}
    }
}

int main()
{
    const std::int32_t current_altitude = get_input("Current altitude: ");
    const std::int32_t airport_altitude = get_input("Airport altitude: ");

    const float altitude_to_lose = ((current_altitude - airport_altitude) / 100.f) / 3.f;

    const std::int32_t current_speed = get_input("Current speed: ");
    const std::int32_t avg_dsc_speed = get_input("Average descend speed: ");

    const float speed_knots = (current_speed - avg_dsc_speed) / 10.f;

    const float wind_speed = get_input("Wind speed: ") / 10.f;
    const std::int32_t tail_wind = get_input("Tail or head wind [1 for tail | 0 for head]: ");
    
    const float descend_distance = altitude_to_lose + speed_knots + (tail_wind == 1 ? wind_speed : -wind_speed);

    std::cout << "------------------------------\n";
    std::cout << "Altitude to lose nm: " << altitude_to_lose << '\n';
    std::cout << "Speed nm: " << speed_knots << '\n';
    std::cout << "Wind  nm: " << wind_speed << '\n';
    std::cout << "Total distance: " << descend_distance << '\n';
    return 0;
}