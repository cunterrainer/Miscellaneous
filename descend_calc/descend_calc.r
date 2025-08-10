get_input <- function(msg)
{
    repeat
    {
        cat(msg)
        input <- trimws(readLines(con = "stdin", n = 1))
        if (grepl("^-?\\d+$", input))
        {
            return(as.integer(input))
        }
    }
}

main <- function()
{
    currentAltitude <- get_input("Current altitude: ")
    airportAltitude <- get_input("Airport altitude: ")

    altitudeToLose <- ((currentAltitude - airportAltitude) / 100.0) / 3.0

    currentSpeed <- get_input("Current speed: ")
    avgDscSpeed <- get_input("Average descend speed: ")

    speedKnots <- (currentSpeed - avgDscSpeed) / 10.0

    windSpeed <- get_input("Wind speed: ") / 10.0
    tailWind <- get_input("Tail or head wind [1 for tail | 0 for head]: ")

    descendDistance <- altitudeToLose + speedKnots + ifelse(tailWind == 1, windSpeed, -windSpeed)

    cat("------------------------------\n")
    cat(sprintf("Altitude to lose nm: %.4f\n", altitudeToLose))
    cat(sprintf("Speed nm: %.4f\n", speedKnots))
    cat(sprintf("Wind  nm: %.4f\n", windSpeed))
    cat(sprintf("Total distance: %.4f\n", descendDistance))
}

main()