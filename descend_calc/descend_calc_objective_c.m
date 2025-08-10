#import <Foundation/Foundation.h>

int get_input(const NSString* msg)
{
    while (true)
    {
        printf("%s", [msg UTF8String]);
        fflush(stdout);
        
        NSFileHandle *input = [NSFileHandle fileHandleWithStandardInput];
        NSData *data = [input availableData];
        
        NSString *str = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
        str = [str stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];

        NSNumberFormatter *formatter = [[NSNumberFormatter alloc] init];
        formatter.numberStyle = NSNumberFormatterDecimalStyle;

        NSNumber *number = [formatter numberFromString:str];
        if (number != nil)
        {
            return [number intValue];
        }
    }
}

int main (int argc, const char * argv[])
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    const int current_altitude = get_input(@"Current altitude: ");
    const int airport_altitude = get_input(@"Airport altitude: ");

    const float altitude_to_lose = ((current_altitude - airport_altitude) / 100.f) / 3.f;

    const int current_speed = get_input(@"Current speed: ");
    const int avg_dsc_speed = get_input(@"Average descend speed: ");

    const float speed_knots = (current_speed - avg_dsc_speed) / 10.f;

    const float wind_speed = get_input(@"Wind speed: ") / 10.f;
    const int tail_wind = get_input(@"Tail or head wind [1 for tail | 0 for head]: ");
    
    const float descend_distance = altitude_to_lose + speed_knots + (tail_wind == 1 ? wind_speed : -wind_speed);

    NSLog(@"------------------------------");
    NSLog(@"Altitude to lose nm: %g", altitude_to_lose);
    NSLog(@"Speed nm: %g", speed_knots);
    NSLog(@"Wind  nm: %g", wind_speed);
    NSLog(@"Total distance: %g", descend_distance);

    [pool drain];
    return 0;
}