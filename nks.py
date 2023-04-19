import numpy as np
import matplotlib.pyplot as plt
from progress.bar import IncrementalBar
from decimal import Decimal, getcontext
from math import tan
from sys import argv
from time import time
from pi_million import pi_million

def factorial(n):
    if n <= 1:
        return Decimal(1)
    else:
        result = Decimal(1)
        for i in range(2, n+1):
            result *= Decimal(i)
        return result

def Chudnovsky(i):
    i -= 1
    num = factorial(6*i)*Decimal(545140134*i+13591409)
    denom = factorial(3*i)*pow(factorial(i), 3)*Chudnovsky.longNumPow
    Chudnovsky.prevNumDenom += num/denom
    Chudnovsky.longNumPow *= Decimal(-262537412640768000)
    return Chudnovsky.sumNum/Chudnovsky.prevNumDenom

def Newton_Raphson(i):
    Newton_Raphson.prevSum = Newton_Raphson.prevSum - Decimal(tan(Newton_Raphson.prevSum))
    return Newton_Raphson.prevSum

def Archimedes(i):
    return i*np.sin(np.deg2rad(180)/i)

def Matches(decNum:Decimal, start):
    same = 0
    decStr = str(decNum)[2:]
    for i in range(start, len(decStr)):
        if decStr[i] != pi_million[i]:
            break
        same += 1
    return same

def Measure(iterations, func):
    xpoints = np.array([])
    ypoints = np.array([])
    start = 0
    Newton_Raphson.prevSum = 3
    Chudnovsky.prevNumDenom = 0
    Chudnovsky.longNumPow = 1 #-262537412640768000^0 = 1
    Chudnovsky.sumNum = Decimal(426880)*Decimal(10005).sqrt()
    bar = IncrementalBar("Iteration", max=iterations, suffix="%(index)d/%(max)d %(percent).2f%%")

    for i in range(1, iterations+1):
        bar.next()
        same = Matches(func(i), start)
        start += same
        xpoints = np.append(xpoints, i)
        ypoints = np.append(ypoints, start)
    bar.finish()
    return xpoints, ypoints

def Smooth(xpoints, ypoints):
    for i in range(len(ypoints)-1, 1, -1):
        if ypoints[i] == ypoints[i-1]:
            xpoints = np.delete(xpoints, i)
            ypoints = np.delete(ypoints, i)
    return xpoints, ypoints

def PrintHelp():
    print("Usage: {} [options]".format(argv[0]))
    print("Options:")
    print("  --help / -h                Display this information")
    print("  --smooth / -s              Smooth the graph (Tries to remove the edges)")
    print("  --iter=[value={}] / -i    Set the number of iterations your selected algorithm is supposed to execute".format(Settings.iterations))
    print("                             Must be an integer value that satisfies [value] > 0")
    print("  --prec=[value={}] / -p   Set the desired precision of the underlying decimal type".format(Settings.precision))
    print("                             Must be an integer value that satisfies [value] > 0")
    print("                             If you happen to get invalid results for the number of decimal places try increasing this")
    print("                             value, however be cautios setting more decimal places increases the computational cost")
    print("  --func=[value=1] / -f      Select the function to measure")
    print("                             [value] can either be the name or the associated number of the function")
    print("                             (1) Chudnovsky")
    print("                             (2) Newton-Raphson (short: Newton)")
    print("                             (3) Archimedes")

class Settings:
    smooth = False
    iterations = 500
    precision = 1000
    selectedFunc = None
    functions = {
        1 : Chudnovsky,
        2 : Newton_Raphson,
        3 : Archimedes
    }

    def __init__(self):
        self.selectedFunc = self.functions[1]

    def Print(self):
        print("Settings----------------------")
        print("Smooth:     {}".format(self.smooth))
        print("Function:   ({}) {}".format(list(self.functions.keys())[list(self.functions.values()).index(self.selectedFunc)], self.GetFuncName()))
        print("Iterations: {}".format(self.iterations))
        print("Precision:  {}".format(self.precision))
        print("------------------------------\n")

    def GetFuncName(self):
        return self.selectedFunc.__name__.replace("_", "-")

def StrToAbsInt(string, defaultValue):
    try:
        return max(1, abs(int(string)))
    except ValueError as ex:
        print("'{}' can not be converted to an int: {}. Deverting to default argument: {}".format(string, ex, defaultValue))
    except Exception as ex:
        print("Failed to convert string to int due to an unknown reason: {}".format(ex))
    return defaultValue

def ParseCmd() -> Settings:
    settings = Settings()
    largv = [x.lower() for x in argv][1:]
    for arg in largv:
        if arg == "--help" or arg == "-h":
            PrintHelp()
            return None
        if arg == "--smooth" or arg == "-s":
            settings.smooth = True
            continue

        splitArg = arg.split("=")
        if len(splitArg) != 2:
            print("Invalid argument [{}]\nTry --help for further assistance".format(arg))
            return None
        if splitArg[0] == "--iter" or splitArg[0] == "-i":
            settings.iterations = StrToAbsInt(splitArg[1], settings.iterations)
        elif splitArg[0] == "--prec" or splitArg[0] == "-p":
            settings.precision = StrToAbsInt(splitArg[1], settings.precision)
        elif splitArg[0] == "--func" or splitArg[0] == "-f":
            if len(splitArg[1]) == 1:
                funcNum = StrToAbsInt(splitArg[1], 1)
                if funcNum < 1 or funcNum > 3:
                    funcNum = 1
                settings.selectedFunc = settings.functions[funcNum]
            elif splitArg[1] == "chudnovsky":
                settings.selectedFunc = settings.functions[1]
            elif splitArg[1] == "newton-raphson" or splitArg[1] == "newton":
                settings.selectedFunc = settings.functions[2]
            elif splitArg[1] == "archimedes":
                settings.selectedFunc = settings.functions[3]
            else:
                print("Invalid --func selected [{}]. Deverting to default argument: {}\nTry --help for further assistance".format(splitArg[1], settings.GetFuncName()))
        else:
            print("Invalid argument [{}]\nTry --help for further assistance".format(arg))
            return None
    return settings

def main():
    settings = ParseCmd()
    if settings == None:
        return
    settings.Print()
    getcontext().prec = settings.precision

    start = time()
    xpoints, ypoints = Measure(settings.iterations, settings.selectedFunc)
    end = time()
    print("Execution time: %.2f sec(s)" % (end-start))

    if settings.smooth:
        xpoints, ypoints = Smooth(xpoints, ypoints)
    plt.plot(xpoints, ypoints)
    plt.annotate("({0}|{1})".format(int(xpoints[-1]), int(ypoints[-1])), (xpoints[-1], ypoints[-1]), xytext=(-50, 8), textcoords='offset pixels')
    plt.title(settings.GetFuncName())
    plt.xlabel("Iterationen")
    plt.ylabel("Nachkommastellen")
    plt.show()

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt as ex:
        print("\nStopped due to keyboard interrupt")
    except BaseException as ex:
        print("\nStopped due to an unhandled exception: {}".format(ex))