open System

let rec getInput (msg: string) : int =
    printf "%s" msg
    let input = Console.ReadLine()
    match Int32.TryParse(input) with
    | (true, x) -> x
    | _ -> getInput msg

[<EntryPoint>]
let main argv =
    let currentAltitude = getInput "Current altitude: "
    let airportAltitude = getInput "Airport altitude: "

    let altitudeToLose = ((float currentAltitude - float airportAltitude) / 100.0) / 3.0

    let currentSpeed = getInput "Current speed: "
    let avgDscSpeed = getInput "Average descend speed: "

    let speedKnots = (float currentSpeed - float avgDscSpeed) / 10.0

    let windSpeed = (float (getInput "Wind speed: ")) / 10.0
    let tailWind = getInput "Tail or head wind [1 for tail | 0 for head]: "

    let descendDistance =
        altitudeToLose + speedKnots + (if tailWind = 1 then windSpeed else -windSpeed)

    printfn "------------------------------"
    printfn "Altitude to lose nm: %f" altitudeToLose
    printfn "Speed nm: %f" speedKnots
    printfn "Wind  nm: %f" windSpeed
    printfn "Total distance: %f" descendDistance

    0 // return an integer exit code