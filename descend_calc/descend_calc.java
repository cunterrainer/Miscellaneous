import java.util.Scanner;

class Program
{
    private static Scanner reader = new Scanner(System.in);

    private static int GetInput(String msg)
    {
        
        while (true)
        {
            try
            {
                System.out.print(msg);
                return Integer.parseInt(reader.nextLine());
            }
            catch (NumberFormatException e) {}
        }
    }

    public static void main(String[] args)
    {
        final int currentAltitude = GetInput("Current altitude: ");
        final int airportAltitude = GetInput("Airport altitude: ");

        final float altitudeToLose = ((currentAltitude - airportAltitude) / 100.f) / 3.f;

        final int currentSpeed = GetInput("Current speed: ");
        final int avgDscSpeed  = GetInput("Average descend speed: ");

        final float speedKnots = (currentSpeed - avgDscSpeed) / 10.f;

        final float windSpeed = GetInput("Wind speed: ") / 10.f;
        final int tailWind    = GetInput("Tail or head wind [1 for tail | 0 for head]: ");
    
        final float descendDistance = altitudeToLose + speedKnots + (tailWind == 1 ? windSpeed : -windSpeed);
        
        System.out.println("------------------------------");
        System.out.println("Altitude to lose nm: " + altitudeToLose);
        System.out.println("Speed nm: " + speedKnots);
        System.out.println("Wind  nm: " + windSpeed);
        System.out.println("Total distance: " + descendDistance);
        reader.close();
    }
}