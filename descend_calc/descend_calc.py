def get_input(msg):
    while True:
        inp = input(msg)
        try:
            return int(inp)
        except ValueError:
            continue

def main():
    current_altitude = get_input("Current altitude: ")
    airport_altitude = get_input("Airport altitude: ")

    altitude_to_lose = ((current_altitude - airport_altitude) / 100) / 3 # remove last to digits

    current_speed = get_input("Current speed: ")
    avg_dsc_speed = get_input("Average descend speed: ")

    speed_knots = (current_speed - avg_dsc_speed) / 10

    wind_speed = get_input("Wind speed: ") / 10
    tail_wind = get_input("Tail or head wind [1 for tail | 0 for head]: ")

    descend_distance = altitude_to_lose + speed_knots

    if tail_wind == 1:
        descend_distance += wind_speed
    else:
        descend_distance -= wind_speed

    print("------------------------------")
    print(f"Altitude to lose nm: {altitude_to_lose}")
    print(f"Speed nm: {speed_knots}")
    print(f"Wind  nm: {wind_speed}")
    print(f"Total distance: {descend_distance}")

if __name__ == "__main__":
    main()