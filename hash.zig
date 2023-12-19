const std = @import("std");


pub fn print_err(comptime format: []const u8, args: anytype) void
{
    std.io.getStdErr().writer().print(format, args) catch return;
}


pub fn print(comptime format: []const u8, args: anytype) void
{
    std.io.getStdOut().writer().print(format, args) catch return;
}


pub fn print_help(path: [] const u8) void
{
    print("Usage: {s} [input] [function] [options]\nOptions:\n", .{path});
    print("        -i   | --input           Force the next argument to be the input (e.g. to get the hash of '-h')\n", .{});
    print("        -h   | --help            Show this info message\n", .{});
    print("        -t   | --text            Treat input as text\n", .{});
    print("        -f   | --file            Treat input as file\n", .{});
    print("        -u   | --upper           Print hash upper case\n", .{});
    print("Supported functions are: md5, sha1, sha224, sha256, sha384, sha512, sha512-256, sha3-224, sha3-256, sha3-384, sha3-512\n", .{});
    print("If neither the '-f' nor '-t' options are specified, the program will hash the file if it is a valid file path; otherwise, it will be treated as a text string.\n", .{});
}


pub fn strcmp(buff: [] const u8, buff2: [] const u8) bool
{
    return std.mem.eql(u8, buff, buff2);
}


const HashFunctions = enum {
    MD5,
    Sha1,
    Sha224,
    Sha256,
    Sha384,
    Sha512,
    Sha512_256,
    Sha3_224,
    Sha3_256,
    Sha3_384,
    Sha3_512
};


pub fn generate_hash_functions_map() !std.StringHashMap(HashFunctions)
{
    // md5, sha1, sha2-224, sha2-256, sha2-384, sha2-512, sha2-512-256, sha3-224, sha3-256, sha3-384, sha3-512
    var hash_functions_map = std.StringHashMap(HashFunctions).init(std.heap.page_allocator);
    try hash_functions_map.put("md5", HashFunctions.MD5);
    try hash_functions_map.put("sha1", HashFunctions.Sha1);
    try hash_functions_map.put("sha224", HashFunctions.Sha224);
    try hash_functions_map.put("sha256", HashFunctions.Sha256);
    try hash_functions_map.put("sha384", HashFunctions.Sha384);
    try hash_functions_map.put("sha512", HashFunctions.Sha512);
    try hash_functions_map.put("sha512-256", HashFunctions.Sha512_256);
    try hash_functions_map.put("sha3-224", HashFunctions.Sha3_224);
    try hash_functions_map.put("sha3-256", HashFunctions.Sha3_256);
    try hash_functions_map.put("sha3-384", HashFunctions.Sha3_384);
    try hash_functions_map.put("sha3-512", HashFunctions.Sha3_512);
    return hash_functions_map;
}


pub fn print_hash(buff: [] u8, length: usize, upper_case: bool) !void
{
    for (0..length) |i|
    {
        if (upper_case)
        {
            print("{X:0>2}", .{ buff[i] });
        }
        else
        {
            print("{x:0>2}", .{ buff[i] });
        }
    }
}


pub fn hash_text(buffer: [] const u8, hasher: HashFunctions, upper_case: bool) void
{
    var length: usize = 0;
    var out_buffer_int: [64] u8 = undefined;
    switch(hasher)
    {
        .MD5 => {
            print("[Text] MD5: ", .{});
            length = std.crypto.hash.Md5.digest_length;
            std.crypto.hash.Md5.hash(buffer, out_buffer_int[0..std.crypto.hash.Md5.digest_length], .{});
        },
        .Sha1 => {
            print("[Text] Sha1: ", .{});
            length = std.crypto.hash.Sha1.digest_length;
            std.crypto.hash.Sha1.hash(buffer, out_buffer_int[0..std.crypto.hash.Sha1.digest_length], .{});
        },
        .Sha224 => {
            print("[Text] Sha224: ", .{});
            length = std.crypto.hash.sha2.Sha224.digest_length;
            std.crypto.hash.sha2.Sha224.hash(buffer, out_buffer_int[0..std.crypto.hash.sha2.Sha224.digest_length], .{});
        },
        .Sha256 => {
            print("[Text] Sha256: ", .{});
            length = std.crypto.hash.sha2.Sha256.digest_length;
            std.crypto.hash.sha2.Sha256.hash(buffer, out_buffer_int[0..std.crypto.hash.sha2.Sha256.digest_length], .{});
        },
        .Sha384 => {
            print("[Text] Sha384: ", .{});
            length = std.crypto.hash.sha2.Sha384.digest_length;
            std.crypto.hash.sha2.Sha384.hash(buffer, out_buffer_int[0..std.crypto.hash.sha2.Sha384.digest_length], .{});
        },
        .Sha512 => {
            print("[Text] Sha512: ", .{});
            length = std.crypto.hash.sha2.Sha512.digest_length;
            std.crypto.hash.sha2.Sha512.hash(buffer, out_buffer_int[0..std.crypto.hash.sha2.Sha512.digest_length], .{});
        },
        .Sha512_256 => {
            print("[Text] Sha512-256: ", .{});
            length = std.crypto.hash.sha2.Sha512256.digest_length;
            std.crypto.hash.sha2.Sha512256.hash(buffer, out_buffer_int[0..std.crypto.hash.sha2.Sha512256.digest_length], .{});
        },
        .Sha3_224 => {
            print("[Text] Sha3-224: ", .{});
            length = std.crypto.hash.sha3.Sha3_224.digest_length;
            std.crypto.hash.sha3.Sha3_224.hash(buffer, out_buffer_int[0..std.crypto.hash.sha3.Sha3_224.digest_length], .{});
        },
        .Sha3_256 => {
            print("[Text] Sha3-256: ", .{});
            length = std.crypto.hash.sha3.Sha3_256.digest_length;
            std.crypto.hash.sha3.Sha3_256.hash(buffer, out_buffer_int[0..std.crypto.hash.sha3.Sha3_256.digest_length], .{});
        },
        .Sha3_384 => {
            print("[Text] Sha3-384: ", .{});
            length = std.crypto.hash.sha3.Sha3_384.digest_length;
            std.crypto.hash.sha3.Sha3_384.hash(buffer, out_buffer_int[0..std.crypto.hash.sha3.Sha3_384.digest_length], .{});
        },
        .Sha3_512 => {
            print("[Text] Sha3-512: ", .{});
            length = std.crypto.hash.sha3.Sha3_512.digest_length;
            std.crypto.hash.sha3.Sha3_512.hash(buffer, out_buffer_int[0..std.crypto.hash.sha3.Sha3_512.digest_length], .{});
        }
    }

    try print_hash(&out_buffer_int, length, upper_case);
}


pub fn hash_file_impl(comptime T: type, file: std.fs.File, upper_case: bool) !void
{
    var buf = std.io.bufferedReader(file.reader());
    var in_stream = buf.reader();

    var hasher: T = T.init(.{});

    var buff: [4096] u8 = undefined;
    var bytes_read: usize = buff.len;
    while (bytes_read == buff.len)
    {
        bytes_read = try in_stream.readAll(&buff);
        hasher.update(buff[0..bytes_read]);
    }

    const length: usize = T.digest_length;
    var out_buffer_int: [64] u8 = undefined;
    hasher.final(out_buffer_int[0..length]);
    try print_hash(&out_buffer_int, length, upper_case);
}


pub fn hash_file(file: std.fs.File, hasher: HashFunctions, upper_case: bool) !void
{
    switch(hasher)
    {
        .MD5 => {
            print("MD5: ", .{});
            try hash_file_impl(std.crypto.hash.Md5, file, upper_case);
        },
        .Sha1 => {
            print("Sha1: ", .{});
            try hash_file_impl(std.crypto.hash.Sha1, file, upper_case);
        },
        .Sha224 => {
            print("Sha224: ", .{});
            try hash_file_impl(std.crypto.hash.sha2.Sha224, file, upper_case);
        },
        .Sha256 => {
            print("Sha256: ", .{});
            try hash_file_impl(std.crypto.hash.sha2.Sha256, file, upper_case);
        },
        .Sha384 => {
            print("Sha384: ", .{});
            try hash_file_impl(std.crypto.hash.sha2.Sha384, file, upper_case);
        },
        .Sha512 => {
            print("Sha512: ", .{});
            try hash_file_impl(std.crypto.hash.sha2.Sha512, file, upper_case);
        },
        .Sha512_256 => {
            print("Sha512-256: ", .{});
            try hash_file_impl(std.crypto.hash.sha2.Sha512256, file, upper_case);
        },
        .Sha3_224 => {
            print("Sha3-224: ", .{});
            try hash_file_impl(std.crypto.hash.sha3.Sha3_224, file, upper_case);
        },
        .Sha3_256 => {
            print("Sha3-256: ", .{});
            try hash_file_impl(std.crypto.hash.sha3.Sha3_256, file, upper_case);
        },
        .Sha3_384 => {
            print("Sha3-384: ", .{});
            try hash_file_impl(std.crypto.hash.sha3.Sha3_384, file, upper_case);
        },
        .Sha3_512 => {
            print("Sha3-512: ", .{});
            try hash_file_impl(std.crypto.hash.sha3.Sha3_512, file, upper_case);
        }
    }
}


pub fn main() !void
{
    const allocator = std.heap.page_allocator;
    const args = std.process.argsAlloc(allocator) catch |e| {
        return print_err("Failed to allocate memory for args: {}", .{e});
    };
    defer std.process.argsFree(allocator, args);

    var skip: bool = false;
    var input: [] u8 = "";
    var force_file: bool = false;
    var force_text: bool = false;
    var upper_case: bool = false;
    var function: HashFunctions = HashFunctions.Sha256;
    const hash_functions_map = try generate_hash_functions_map();

    for (1..args.len) |i|
    {
        if (skip)
        {
            skip = false;
            continue;
        }

        const arg = try allocator.alloc(u8, args[i].len);
        defer allocator.free(arg);
        _ = std.ascii.lowerString(arg, args[i]);

        const hash_function = hash_functions_map.get(arg);
        if (hash_function) |v|
        {
            function = v;
        }
        else if (strcmp(arg, "-u") or strcmp(arg, "--upper"))
        {
            upper_case = true;
        }
        else if (strcmp(arg, "-f") or strcmp(arg, "--file"))
        {
            force_file = true;
            force_text = false;
        }
        else if (strcmp(arg, "-t") or strcmp(arg, "--text"))
        {
            force_file = false;
            force_text = true;
        }
        else if (strcmp(arg, "-h") or strcmp(arg, "--help"))
        {
            return print_help(args[0]);
        }
        else if (strcmp(arg, "-i") or strcmp(arg, "--input"))
        {
            if (i + 1 == args.len)
            {
                return print_err("Missing input after '{s}'\nTry '--help' for additional information\n", .{args[i]});
            }

            input = args[i+1];
            skip = true;
        }
        else
        {
            if (input.len > 0)
            {
                print_err("Previous input: {s}\nNew input: {s}\nWas this change intentional?\nTry '--help' for additional information\n\n", .{ input, args[i] });
            }
            input = args[i];
        }
    }

    if (input.len == 0)
    {
        force_text = true;
    }

    if (force_text)
    {
        return hash_text(input, function, upper_case);
    }


    const in_file = std.fs.cwd().openFile(input, .{}) catch |e| {
        if (e != std.fs.File.OpenError.FileNotFound or force_file)
        {
            return print_err("Failed to open file '{s}': {}\n", .{input, e});
        }
        return hash_text(input, function, upper_case);
    };
    defer in_file.close();
    print("[{s}] ", .{input});
    try hash_file(in_file, function, upper_case);
}