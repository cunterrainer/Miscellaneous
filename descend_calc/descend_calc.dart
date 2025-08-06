import "dart:io";

int get_input(String msg)
{
    while (true)
    {
        stdout.write(msg);
        final String? input = stdin.readLineSync();
        if (input == null) continue;

        final int? result = int.tryParse(input);
        if (result != null) return result;
    }
}

void main()
{
    final int current_altitude = get_input("Current altitude: ");
    final int airport_altitude = get_input("Airport altitude: ");
   
    final double altitude_to_lose = ((current_altitude - airport_altitude) / 100.0) / 3.0;

    final int current_speed = get_input("Current speed: ");
    final int avg_dsc_speed = get_input("Average descend speed: ");

    final double speed_knots = (current_speed - avg_dsc_speed) / 10.0;

    final double wind_speed = get_input("Wind speed: ") / 10.0;
    final int tail_wind = get_input("Tail or head wind [1 for tail | 0 for head]: ");
    
    final double descend_distance = altitude_to_lose + speed_knots + (tail_wind == 1 ? wind_speed : -wind_speed);

    print("------------------------------");
    print("Altitude to lose nm:" + altitude_to_lose.toString());
    print("Speed nm: " + speed_knots.toString());
    print("Wind  nm: " + wind_speed.toString());
    print("Total distance: " + descend_distance.toString());
}