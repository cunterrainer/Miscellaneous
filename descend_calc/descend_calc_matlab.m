function f = get_input(msg)
    while true
        prompt = sprintf('%s', msg);
        data = input(prompt, "s");
        f = str2double(data);
    
        if ~isnan(f) && fix(f) == f
            return
        end
    end
end

function f = main()
    current_altitude = get_input("Current altitude: ");
    airport_altitude = get_input("Airport altitude: ");

    altitude_to_lose = ((current_altitude - airport_altitude) / 100.0) / 3.0;

    current_speed = get_input("Current speed: ");
    avg_dsc_speed = get_input("Average descend speed: ");

    speed_knots = (current_speed - avg_dsc_speed) / 10.0;

    wind_speed = get_input("Wind speed: ") / 10.0;
    tail_wind = get_input("Tail or head wind [1 for tail | 0 for head]: ");
    
    descend_distance = altitude_to_lose + speed_knots;
    if tail_wind == 1
        descend_distance = descend_distance + wind_speed;
    else
        descend_distance = descend_distance - wind_speed;
    end

    fprintf("------------------------------\n");
    fprintf("Altitude to lose nm: %g\n", altitude_to_lose);
    fprintf("Speed nm: %g\n", speed_knots);
    fprintf("Wind  nm: %g\n", wind_speed);
    fprintf("Total distance: %g\n", descend_distance);
end

main()