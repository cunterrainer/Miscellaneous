function get_input(msg)
    while true do
        io.write(msg)
        local num = tonumber(io.read())
        if num ~= nil then
            return num
        end
    end
end

function main()
    local current_altitude = get_input("Current altitude: ")
    local airport_altitude = get_input("Airport altitude: ")

    local altitude_to_lose = ((current_altitude - airport_altitude) / 100.0) / 3.0

    local current_speed = get_input("Current speed: ")
    local avg_dsc_speed = get_input("Average descend speed: ")

    local speed_knots = (current_speed - avg_dsc_speed) / 10.0

    local wind_speed = get_input("Wind speed: ") / 10.0
    local tail_wind = get_input("Tail or head wind [1 for tail | 0 for head]: ")
    
    local descend_distance = altitude_to_lose + speed_knots + (tail_wind == 1 and wind_speed or -wind_speed)

    print("------------------------------")
    print(string.format("Altitude to lose nm: %f", altitude_to_lose))
    print(string.format("Speed nm: %f", speed_knots))
    print(string.format("Altitude to lose nm: %f", wind_speed))
    print(string.format("Altitude to lose nm: %f", descend_distance))
end

main()