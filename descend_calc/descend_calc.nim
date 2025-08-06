import std/strutils

proc get_input(msg: string): int =
    while true:
        stdout.write(msg)
        try: return parseInt(stdin.readLine())
        except ValueError: continue


proc main() =
    let current_altitude = get_input("Current altitude: ")
    let airport_altitude = get_input("Airport altitude: ")

    let altitude_to_lose = (float(current_altitude - airport_altitude) / 100.0) / 3.0

    let current_speed = get_input("Current speed: ")
    let avg_dsc_speed = get_input("Average descend speed: ")

    let speed_knots = float(current_speed - avg_dsc_speed) / 10.0

    let wind_speed = float(get_input("Wind speed: ")) / 10.0
    let tail_wind = get_input("Tail or head wind [1 for tail | 0 for head]: ")
    
    let descend_distance = altitude_to_lose + speed_knots + (if tail_wind == 1: wind_speed else: -wind_speed);

    echo("------------------------------");
    echo("Altitude to lose nm: ", altitude_to_lose);
    echo("Speed nm: ", speed_knots);
    echo("Wind  nm: ", wind_speed);
    echo("Total distance: ", descend_distance);

main()