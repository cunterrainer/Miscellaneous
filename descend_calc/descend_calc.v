import os
import strconv

fn get_input(msg string) int
{
  for
  {
    print(msg)
    input := os.get_line().trim_space()
    if num := strconv.parse_int(input, 10, 32)
    {
      return int(num)
    }
  }
  return 0
}

fn main()
{
  current_altitude := get_input('Current altitude: ')
  airport_altitude := get_input('Airport altitude: ')

  altitude_to_lose := (f32(current_altitude - airport_altitude) / 100.0) / 3.0

  current_speed := get_input('Current speed: ')
  avg_dsc_speed := get_input('Average descend speed: ')

  speed_knots := f32(current_speed - avg_dsc_speed) / 10.0

  wind_speed := f32(get_input('Wind speed: ')) / 10.0
  tail_wind := get_input('Tail or head wind [1 for tail | 0 for head]: ')

  descend_distance := altitude_to_lose + speed_knots + if tail_wind == 1 { wind_speed } else { -wind_speed }

  println('------------------------------')
  println('Altitude to lose nm: $altitude_to_lose')
  println('Speed nm: $speed_knots')
  println('Wind  nm: $wind_speed')
  println('Total distance: $descend_distance')
}