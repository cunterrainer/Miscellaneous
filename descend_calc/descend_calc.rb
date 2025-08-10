def get_input(msg)
    loop do
        print msg
        input = gets.chomp
        if input =~ /^-?\d+$/
            return input.to_i
        end
    end
end

def main
    current_altitude = get_input("Current altitude: ")
    airport_altitude = get_input("Airport altitude: ")

    altitude_to_lose = ((current_altitude - airport_altitude).to_f / 100.0) / 3.0

    current_speed = get_input("Current speed: ")
    avg_dsc_speed = get_input("Average descend speed: ")

    speed_knots = (current_speed - avg_dsc_speed).to_f / 10.0

    wind_speed = get_input("Wind speed: ").to_f / 10.0
    tail_wind = get_input("Tail or head wind [1 for tail | 0 for head]: ")

    descend_distance = altitude_to_lose + speed_knots + (tail_wind == 1 ? wind_speed : -wind_speed)

    puts "------------------------------"
    puts "Altitude to lose nm: #{altitude_to_lose}"
    puts "Speed nm: #{speed_knots}"
    puts "Wind  nm: #{wind_speed}"
    puts "Total distance: #{descend_distance}"
end

main
