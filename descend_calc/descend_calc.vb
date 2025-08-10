Imports System

Public Class Program
    Private Shared Function GetInput(msg As String) As Integer
        While True
            Console.Write(msg)
            Dim input As String = Console.ReadLine()
            Dim x As Integer
            If Int32.TryParse(input, x) Then
                Return x
            End If
        End While
    End Function

    Public Shared Sub Main(args As String())
        Dim currentAltitude As Integer = GetInput("Current altitude: ")
        Dim airportAltitude As Integer = GetInput("Airport altitude: ")

        Dim altitudeToLose As Single = ((currentAltitude - airportAltitude) / 100.0F) / 3.0F

        Dim currentSpeed As Integer = GetInput("Current speed: ")
        Dim avgDscSpeed As Integer = GetInput("Average descend speed: ")

        Dim speedKnots As Single = (currentSpeed - avgDscSpeed) / 10.0F

        Dim windSpeed As Single = GetInput("Wind speed: ") / 10.0F
        Dim tailWind As Integer = GetInput("Tail or head wind [1 for tail | 0 for head]: ")

        Dim descendDistance As Single = altitudeToLose + speedKnots + If(tailWind = 1, windSpeed, -windSpeed)

        Console.WriteLine("------------------------------")
        Console.WriteLine("Altitude to lose nm: " & altitudeToLose)
        Console.WriteLine("Speed nm: " & speedKnots)
        Console.WriteLine("Wind  nm: " & windSpeed)
        Console.WriteLine("Total distance: " & descendDistance)
    End Sub

End Class
