import System.IO (stdout, hFlush)
import Text.Read (readMaybe)

getInput :: String -> IO Int
getInput msg = do
  putStr msg
  hFlush stdout
  line <- getLine
  case readMaybe line of
    Just n  -> return n
    Nothing -> do
      getInput msg

main :: IO ()
main = do
  currentAltitude <- getInput "Current altitude: "
  airportAltitude <- getInput "Airport altitude: "

  let altitudeToLose = fromIntegral (currentAltitude - airportAltitude) / 100.0 / 3.0

  currentSpeed <- getInput "Current speed: "
  avgDscSpeed <- getInput "Average descend speed: "

  let speedKnots = fromIntegral (currentSpeed - avgDscSpeed) / 10.0

  windSpeedInput <- getInput "Wind speed: "
  let windSpeed = fromIntegral windSpeedInput / 10.0

  tailWind <- getInput "Tail or head wind [1 for tail | 0 for head]: "

  let descendDistance =
        altitudeToLose + speedKnots + if tailWind == 1 then windSpeed else (-windSpeed)

  putStrLn "------------------------------"
  putStrLn $ "Altitude to lose nm: " ++ show altitudeToLose
  putStrLn $ "Speed nm: " ++ show speedKnots
  putStrLn $ "Wind  nm: " ++ show windSpeed
  putStrLn $ "Total distance: " ++ show descendDistance