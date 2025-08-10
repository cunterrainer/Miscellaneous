const std = @import("std");

fn get_input(msg: []const u8) !i32
{
    const stdout = std.io.getStdOut().writer();
    const stdin = std.io.getStdIn().reader();
    const allocator = std.heap.page_allocator;

    while (true)
    {
        try stdout.print("{s}", .{msg});

        const result = try stdin.readUntilDelimiterOrEofAlloc(allocator, '\n', 2500);
        if (result) |value|
        {
            defer allocator.free(value);

            // Trim trailing '\r' if present
            const line = std.mem.trimRight(u8, value, "\r");
            const parse_res = std.fmt.parseInt(i32, line, 10) catch null;

            if (parse_res) |num|
            {
                return num;
            }
        }
    }
}

pub fn main() !void
{
    const current_altitude: i32 = try get_input("Current altitude: ");
    const airport_altitude: i32 = try get_input("Airport altitude: ");

    const altitude_to_lose: f32 = (@as(f32, @floatFromInt(current_altitude - airport_altitude)) / 100.0) / 3.0;

    const current_speed: i32 = try get_input("Current speed: ");
    const avg_dsc_speed: i32 = try get_input("Average descend speed: ");

    const speed_knots: f32 = @as(f32, @floatFromInt(current_speed - avg_dsc_speed)) / 10.0;

    const wind_speed: f32 = @as(f32, @floatFromInt(try get_input("Wind speed: "))) / 10.0;
    const tail_wind: i32 = try get_input("Tail or head wind [1 for tail | 0 for head]: ");
    
    const descend_distance: f32 = altitude_to_lose + speed_knots + (if (tail_wind == 1) wind_speed else -wind_speed);
    
    const stdout = std.io.getStdOut().writer();
    try stdout.print("------------------------------\n", .{});
    try stdout.print("Altitude to lose nm: {d}\n", .{altitude_to_lose});
    try stdout.print("Speed nm: {d}\n", .{speed_knots});
    try stdout.print("Wind  nm: {d}\n", .{wind_speed});
    try stdout.print("Total distance: {d}\n", .{descend_distance});
}
