const std = @import("std");

pub fn build(b: *std.Build) !void {
    const enable_poll = b.option(bool, "poll", "Use poll() instead of select()") orelse false;
    const enable_readline = b.option(bool, "readline", "Enable GNU readline support in dl") orelse false;
    const enable_wsock = b.option(bool, "wsock1", "Enable WinSock 1.1 support for a truly portable Windows 32-bit binary") orelse false;

    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

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

    const isWindows = (target.query.isNative() and b.graph.host.query.os_tag == .windows) or target.query.os_tag == .windows;

    if (isWindows) {
        const sock2 = &[_][]const u8{
            "../src/platform/winsock/wsock2.c",
            "../src/platform/winsock/wsipv6.c",
        };

        //TODO: Is targeting NT5+ possible?
        const w2k = &[_][]const u8{
            "-D_WIN32=1",
            "-DWINVER=0x0500",
            "-D_WIN32_WINNT=0x0500",
            "-static"
        };

        try th_src.appendSlice(sock2);
        try dl_src.append("../src/platform/mscrtdl.c");
        try th_src.append("../src/platform/mscrtdl.c");

        try dl_flags.appendSlice(w2k);
        try th_flags.appendSlice(w2k);

        dl.linkSystemLibrary("wsock32");
        th.linkSystemLibrary("wsock32");

        if (enable_wsock) {
            try th_flags.append("ENABLE_WS1=1");
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

    const dir: std.Build.Step.InstallArtifact.Options.Dir = .{
        .override = .{
            .custom = try target.result.zigTriple(b.allocator),
        },
    };

    const dl_output = b.addInstallArtifact(dl, .{
        .dest_dir = dir,
    });

    const th_output = b.addInstallArtifact(th, .{
        .dest_dir = dir,
    });

    b.getInstallStep().dependOn(&dl_output.step);
    b.getInstallStep().dependOn(&th_output.step);
}
