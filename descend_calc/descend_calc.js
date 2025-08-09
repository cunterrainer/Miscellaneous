const readline = require('readline');

async function get_input(msg)
{
    const rl = readline.createInterface({
      input: process.stdin,
      output: process.stdout
    });

    while (true)
    {
      const line = await new Promise(resolve =>
        rl.question(msg, resolve)
      );

      const num = parseInt(line.trim(), 10);
      if (!isNaN(num))
      {
        rl.close();
        return num;
      }
  }
}

async function main()
{
    const current_altitude = await get_input("Current altitude: ")
    const airport_altitude = await get_input("Airport altitude: ")

    const altitude_to_lose = ((current_altitude - airport_altitude) / 100.0) / 3.0

    const current_speed = await get_input("Current speed: ")
    const avg_dsc_speed = await get_input("Average descend speed: ")

    const speed_knots = (current_speed - avg_dsc_speed) / 10.0

    const wind_speed = await get_input("Wind speed: ") / 10.0
    const tail_wind = await get_input("Tail or head wind [1 for tail | 0 for head]: ")
    
    const descend_distance = altitude_to_lose + speed_knots + (tail_wind == 1 ? wind_speed : -wind_speed)

    console.log("------------------------------");
    console.log("Altitude to lose nm: " + altitude_to_lose);
    console.log("Speed nm: " + speed_knots);
    console.log("Wind  nm: " + wind_speed);
    console.log("Total distance: " + descend_distance);
}

main()