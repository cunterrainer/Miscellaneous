func get_input(msg: String) -> Int
{
    while true
    {
        print(msg, terminator: "")
        if let num = Int(readLine()!)
        {
            return num
        }
    }
}

func main() -> Void
{
    let current_altitude = get_input(msg: "Current altitude: ")
    let airport_altitude = get_input(msg: "Airport altitude: ")

    let altitude_to_lose = (Float(current_altitude - airport_altitude) / 100.0 as Float) / 3.0 as Float

    let current_speed = get_input(msg: "Current speed: ")
    let avg_dsc_speed = get_input(msg: "Average descend speed: ")

    let speed_knots = Float(current_speed - avg_dsc_speed) / 10.0 as Float

    let wind_speed = Float(get_input(msg: "Wind speed: ")) / 10.0 as Float
    let tail_wind = get_input(msg: "Tail or head wind [1 for tail | 0 for head]: ")
    
    let descend_distance = altitude_to_lose + speed_knots + (tail_wind == 1 ? wind_speed : -wind_speed)

    print("------------------------------");
    print("Altitude to lose nm: \(altitude_to_lose)");
    print("Speed nm: \(speed_knots)");
    print("Wind  nm: \(wind_speed)");
    print("Total distance: \(descend_distance)");
}

main()