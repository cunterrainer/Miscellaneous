fun get_input(msg: String): Int
{
    while (true)
    {
        try
        {
            print(msg);
            return readln().toInt();
        }
        catch (e: NumberFormatException) {}
    }
}


fun main()
{
    val currentAltitude = get_input("Current altitude: ");
    val airportAltitude = get_input("Airport altitude: ");

    val altitudeToLose = ((currentAltitude - airportAltitude) / 100.0f) / 3.0f;

    val currentSpeed = get_input("Current speed: ");
    val avgDscSpeed  = get_input("Average descend speed: ");

    val speedKnots = (currentSpeed - avgDscSpeed) / 10.0f;

    val windSpeed = get_input("Wind speed: ") / 10.0f;
    val tailWind    = get_input("Tail or head wind [1 for tail | 0 for head]: ");

    val descendDistance = altitudeToLose + speedKnots + (if (tailWind == 1) windSpeed else -windSpeed);
    
    println("------------------------------");
    println("Altitude to lose nm: $altitudeToLose");
    println("Speed nm: $speedKnots");
    println("Wind  nm: $windSpeed");
    println("Total distance: $descendDistance");
}