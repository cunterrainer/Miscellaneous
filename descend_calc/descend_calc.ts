import * as readline from 'readline';

async function get_input(msg: string): Promise<number>
{
    const rl = readline.createInterface({
      input: process.stdin,
      output: process.stdout
    });

    while (true)
    {
      const line: string = await new Promise<string>(resolve =>
        rl.question(msg, (answer: string) => resolve(answer))
      );

      const num: number = parseInt(line.trim(), 10);
      if (!isNaN(num))
      {
        rl.close();
        return num;
      }
  }
}

async function main()
{
    const current_altitude: number = await get_input("Current altitude: ")
    const airport_altitude: number = await get_input("Airport altitude: ")

    const altitude_to_lose: number = ((current_altitude - airport_altitude) / 100.0) / 3.0

    const current_speed: number = await get_input("Current speed: ")
    const avg_dsc_speed: number = await get_input("Average descend speed: ")

    const speed_knots: number = (current_speed - avg_dsc_speed) / 10.0

    const wind_speed: number = await get_input("Wind speed: ") / 10.0
    const tail_wind: number = await get_input("Tail or head wind [1 for tail | 0 for head]: ")
    
    const descend_distance: number = altitude_to_lose + speed_knots + (tail_wind == 1 ? wind_speed : -wind_speed)

    console.log("------------------------------");
    console.log("Altitude to lose nm: " + altitude_to_lose);
    console.log("Speed nm: " + speed_knots);
    console.log("Wind  nm: " + wind_speed);
    console.log("Total distance: " + descend_distance);
}

main()