#!/usr/bin/perl
use strict;
use warnings;

sub get_input
{
    my ($msg) = @_;
    while (1)
    {
        print $msg;
        my $input = <STDIN>;
        chomp $input;
        if ($input =~ /^-?\d+$/)
        {
            return int($input);
        }
    }
}

sub main
{
    my $currentAltitude = get_input("Current altitude: ");
    my $airportAltitude = get_input("Airport altitude: ");

    my $altitudeToLose = (($currentAltitude - $airportAltitude) / 100) / 3;

    my $currentSpeed = get_input("Current speed: ");
    my $avgDscSpeed = get_input("Average descend speed: ");

    my $speedKnots = ($currentSpeed - $avgDscSpeed) / 10;

    my $windSpeed = get_input("Wind speed: ") / 10;
    my $tailWind = get_input("Tail or head wind [1 for tail | 0 for head]: ");

    my $descendDistance = $altitudeToLose + $speedKnots + ($tailWind == 1 ? $windSpeed : -$windSpeed);

    print "------------------------------\n";
    printf "Altitude to lose nm: %.6f\n", $altitudeToLose;
    printf "Speed nm: %.6f\n", $speedKnots;
    printf "Wind  nm: %.6f\n", $windSpeed;
    printf "Total distance: %.6f\n", $descendDistance;
}

main();