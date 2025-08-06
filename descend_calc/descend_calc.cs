using System;

public class Program
{
    private static int GetInput(string msg)
    {
        while (true)
        {
            Console.Write(msg);
            string input = Console.ReadLine();
            int x = 0;
            if (Int32.TryParse(input, out x)) return x;
        }
    }

    public static void Main(string[] args)
    {
        int currentAltitude = GetInput("Current altitude: ");
        int airportAltitude = GetInput("Airport altitude: ");

        float altitudeToLose = ((currentAltitude - airportAltitude) / 100.0F) / 3.0F;

        int currentSpeed = GetInput("Current speed: ");
        int avgDscSpeed = GetInput("Average descend speed: ");

        float speedKnots = (currentSpeed - avgDscSpeed) / 10.0F;

        float windSpeed = GetInput("Wind speed: ") / 10.0F;
        int tailWind = GetInput("Tail or head wind [1 for tail | 0 for head]: ");

        float descendDistance = altitudeToLose + speedKnots + (tailWind == 1 ? windSpeed : -windSpeed);

        Console.WriteLine("------------------------------");
        Console.WriteLine("Altitude to lose nm: " + altitudeToLose);
        Console.WriteLine("Speed nm: " + speedKnots);
        Console.WriteLine("Wind  nm: " + windSpeed);
        Console.WriteLine("Total distance: " + descendDistance);
    }
}