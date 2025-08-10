<?php

function get_input(string $msg): int
{
    while (true)
    {
        echo $msg;
        $line = trim(fgets(STDIN));
        if (preg_match('/^-?\d+$/', $line))
        {
            return (int)$line;
        }
    }
}

function main()
{
    $currentAltitude = get_input("Current altitude: ");
    $airportAltitude = get_input("Airport altitude: ");

    $altitudeToLose = (($currentAltitude - $airportAltitude) / 100.0) / 3.0;

    $currentSpeed = get_input("Current speed: ");
    $avgDscSpeed = get_input("Average descend speed: ");

    $speedKnots = ($currentSpeed - $avgDscSpeed) / 10.0;

    $windSpeed = get_input("Wind speed: ") / 10.0;
    $tailWind = get_input("Tail or head wind [1 for tail | 0 for head]: ");

    $descendDistance = $altitudeToLose + $speedKnots + ($tailWind === 1 ? $windSpeed : -$windSpeed);

    echo "------------------------------\n";
    echo "Altitude to lose nm: " . $altitudeToLose . "\n";
    echo "Speed nm: " . $speedKnots . "\n";
    echo "Wind  nm: " . $windSpeed . "\n";
    echo "Total distance: " . $descendDistance . "\n";
}

main();