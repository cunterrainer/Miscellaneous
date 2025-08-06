package main

import "fmt"

func get_input(msg string) int {
	var input int

	for true {
		fmt.Print(msg)

		_, err := fmt.Scanln(&input)
		if err != nil {
			var discard string
			fmt.Scanln(&discard)
		} else {
			break
		}
	}

	return input
}

func main() {
	var current_altitude = get_input("Current altitude: ")
    var airport_altitude = get_input("Airport altitude: ")

	var altitude_to_lose = (float32(current_altitude - airport_altitude) / 100) / 3;

	var current_speed = get_input("Current speed: ");
    var avg_dsc_speed = get_input("Average descend speed: ");

	var speed_knots = float32(current_speed - avg_dsc_speed) / 10

	var wind_speed = float32(get_input("Wind speed: ")) / 10
    var tail_wind = get_input("Tail or head wind [1 for tail | 0 for head]: ")
    
	var descend_distance = altitude_to_lose + speed_knots
	if tail_wind == 1 {
		descend_distance += wind_speed
	} else {
		descend_distance -= wind_speed
	}

	fmt.Println("------------------------------")
	fmt.Printf("Altitude to lose nm: %v\n", altitude_to_lose)
	fmt.Printf("Speed nm: %v\n", speed_knots)
	fmt.Printf("Wind  nm: %v\n", wind_speed)
	fmt.Printf("Total distance nm: %v\n", descend_distance)
}
