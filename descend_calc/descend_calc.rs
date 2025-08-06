use std::io;
use std::io::Write;

fn get_input(msg: &str) -> i32
{
    loop
    {
        let mut input = String::new();

        print!("{}", msg);
        io::stdout().flush().unwrap();

        let _ = io::stdin().read_line(&mut input);
        match input.trim().parse::<i32>() {
            Ok(n) => return n,
            Err(_e) => continue,
        }
    }
}

fn main()
{
    let current_altitude = get_input("Current altitude: ");
    let airport_altitude = get_input("Airport altitude: ");

    let altitude_to_lose: f32 = ((current_altitude - airport_altitude) as f32 / 100.0f32) / 3.0f32;

    let current_speed = get_input("Current speed: ");
    let avg_dsc_speed = get_input("Average descend speed: ");

    let speed_knots: f32 = (current_speed - avg_dsc_speed) as f32 / 10.0f32;

    let wind_speed: f32 = get_input("Wind speed: ") as f32 / 10.0f32;
    let tail_wind = get_input("Tail or head wind [1 for tail | 0 for head]: ");
    
    let descend_distance: f32 = altitude_to_lose + speed_knots + (if tail_wind == 1 { wind_speed } else { -wind_speed });

    println!("------------------------------");
    println!("Altitude to lose nm: {}", altitude_to_lose);
    println!("Speed nm: {}", speed_knots);
    println!("Wind  nm: {}", wind_speed);
    println!("Total distance: {}", descend_distance);
}