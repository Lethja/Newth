const std = @import("std");
const platform = @import("builtin").target.os;

pub fn build(b: *std.Build) !void {
    const enable_poll = b.option(bool, "poll", "Use poll() instead of select()") orelse false;
    const enable_readline = b.option(bool, "readline", "Enable GNU readline support in dl") orelse true;
    const enable_w32_socket_1 = b.option(bool, "wsock1", "Enable WinSock 1.1 support for a truly portable Windows 32-bit binary") orelse false;

    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // Executables
    const dl = b.addExecutable(.{
        .name = "dl",
        .target = target,
        .optimize = optimize,
    });

    const th = b.addExecutable(.{
        .name = "th",
        .target = target,
        .optimize = optimize,
    });

    // Source and flag arrays
    const common_src = &[_][]const u8{
        "../src/common/hex.c",        "../src/common/signal.c",
        "../src/platform/platform.c",
    };

    const dl_common_src = &[_][]const u8{
        "../src/cli/dl.c",          "../src/client/err.c",  "../src/client/io.c",        "../src/client/queue.c",
        "../src/client/recvbufr.c", "../src/client/site.c", "../src/client/site/file.c", "../src/client/site/http.c",
        "../src/client/uri.c",      "../src/client/xml.c",
    } ++ common_src;

    const th_common_src = &[_][]const u8{
        "../src/cli/th.c",        "../src/server/event.c",    "../src/server/http.c", "../src/server/routine.c",
        "../src/server/server.c", "../src/server/sendbufr.c",
    } ++ common_src;

    const dl_common_flags = &[_][]const u8{
        "-D_FILE_OFFSET_BITS=64", "-DPLATFORM_SYS_ARGV=1",
        "-DPLATFORM_SYS_WRITE=1", "-DPLATFORM_SYS_EXEC=1",
    };

    const th_common_flags = &[_][]const u8{
        "-D_FILE_OFFSET_BITS=64",  "-DPLATFORM_NET_ADAPTER=1",
        "-DPLATFORM_NET_LISTEN=1", "-DPLATFORM_SYS_ARGV=1",
    };

    var dl_src = std.ArrayList([]const u8).init(b.allocator);
    defer dl_src.deinit();
    var th_src = std.ArrayList([]const u8).init(b.allocator);
    defer th_src.deinit();
    var dl_flags = std.ArrayList([]const u8).init(b.allocator);
    defer dl_flags.deinit();
    var th_flags = std.ArrayList([]const u8).init(b.allocator);
    defer th_flags.deinit();

    try dl_src.appendSlice(dl_common_src);
    try th_src.appendSlice(th_common_src);

    const isWindows = (target.query.os_tag != null and target.query.os_tag == .windows) or platform.tag == .windows;

    if (isWindows) {
        const sock2 = &[_][]const u8{
            "../src/platform/winsock/wsock2.c",
            "../src/platform/winsock/wsipv6.c",
        };

        try th_src.appendSlice(sock2);
        try dl_src.append("../src/platform/mscrtdl.c");
        try th_src.append("../src/platform/mscrtdl.c");

        try dl_flags.append("-D_WIN32=1");
        try th_flags.append("-D_WIN32=1");

        dl.linkSystemLibrary("wsock32");
        th.linkSystemLibrary("wsock32");

        if (enable_w32_socket_1) {
            try dl_src.append("../src/platform/winsock/wsock1.c");
            try th_src.append("../src/platform/winsock/wsock1.c");
        }
    } else {
        try dl_src.append("../src/platform/posix01.c");
        try th_src.append("../src/platform/posix01.c");
    }

    try dl_flags.appendSlice(dl_common_flags);
    try th_flags.appendSlice(th_common_flags);

    if (enable_readline) {
        try dl_flags.append("-DREADLINE=1");
        dl.linkSystemLibrary("readline");
    }

    if (enable_poll)
        try th_src.append("../src/server/mltiplex/poll.c")
    else
        try th_src.append("../src/server/mltiplex/select.c");

    var src: []const []const u8 = try dl_src.toOwnedSlice();
    var flags: []const []const u8 = try dl_flags.toOwnedSlice();

    dl.addCSourceFiles(.{
        .files = src,
        .flags = flags,
    });

    dl.linkLibC();

    src = try th_src.toOwnedSlice();
    flags = try th_flags.toOwnedSlice();

    th.addCSourceFiles(.{
        .files = src,
        .flags = flags,
    });

    th.linkLibC();

    const dl_output = b.addInstallArtifact(dl, .{
        .dest_dir = .{
            .override = .{
                .custom = try target.result.zigTriple(b.allocator),
            },
        },
    });

    const th_output = b.addInstallArtifact(th, .{
        .dest_dir = .{
            .override = .{
                .custom = try target.result.zigTriple(b.allocator),
            },
        },
    });

    b.getInstallStep().dependOn(&dl_output.step);
    b.getInstallStep().dependOn(&th_output.step);
}
