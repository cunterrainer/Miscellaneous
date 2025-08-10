#!/bin/bash

get_input()
{
    local msg="$1"
    local num
    while true; do
        read -p "$msg" num
        if [[ "$num" =~ ^-?[0-9]+$ ]]; then
            echo "$num"
            return
        else
            echo "Please enter a valid integer."
        fi
    done
}

main()
{
    currentAltitude=$(get_input "Current altitude: ")
    airportAltitude=$(get_input "Airport altitude: ")

    altitudeToLose=$(echo "scale=6; (($currentAltitude - $airportAltitude) / 100) / 3" | bc -l)

    currentSpeed=$(get_input "Current speed: ")
    avgDscSpeed=$(get_input "Average descend speed: ")

    speedKnots=$(echo "scale=6; ($currentSpeed - $avgDscSpeed) / 10" | bc -l)

    windSpeed=$(get_input "Wind speed: ")
    windSpeed=$(echo "scale=6; $windSpeed / 10" | bc -l)

    tailWind=$(get_input "Tail or head wind [1 for tail | 0 for head]: ")

    if [ "$tailWind" -eq 1 ]; then
        sign=1
    else
        sign=-1
    fi

    descendDistance=$(echo "scale=6; $altitudeToLose + $speedKnots + ($sign * $windSpeed)" | bc -l)

    echo "------------------------------"
    echo "Altitude to lose nm: $altitudeToLose"
    echo "Speed nm: $speedKnots"
    echo "Wind  nm: $windSpeed"
    echo "Total distance: $descendDistance"
}

main