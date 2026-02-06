const std = @import("std");

const targets: []const std.Target.Query = &.{
    .{.cpu_arch = .x86_64,  .os_tag = .macos,   .os_version_min = .{.semver = .{.major = 10, .minor = 7, .patch = 0}}},
    .{.cpu_arch = .aarch64, .os_tag = .macos,   .os_version_min = .{.semver = .{.major = 11, .minor = 0, .patch = 0}}},
    .{.cpu_arch = .x86_64,  .os_tag = .linux,   .abi = .musl},
    .{.cpu_arch = .aarch64, .os_tag = .linux,   .abi = .musl},
    .{.cpu_arch = .x86_64,  .os_tag = .windows, .abi = .gnu},
};

pub fn build(b: *std.Build) !void {
    const optimize = b.standardOptimizeOption(.{});

    for (targets) |t| {
        const target = b.resolveTargetQuery(t);

        const lzcrypt = b.addLibrary(.{
            .name = "lzcrypt",
            .linkage = .static,
            .root_module = b.createModule(.{
                .root_source_file = b.path("src/lzcrypt/lzcrypt.zig"),
                .target = target,
                .optimize = optimize
            })
        });

        const keygen = b.addExecutable(.{
            .name = "subkg",
            .root_module = b.createModule(.{
                .target = target,
                .optimize = optimize,
                .link_libc = true,
            })
        });
        keygen.linkLibrary(lzcrypt);
        keygen.root_module.addCSourceFiles(.{
            .root = b.path("src"),
            .files = &[_][]const u8{
                "main.c",
                "patch.c",
                "keygen.c",
                "skip/skip.c",
                "xskip/xskip.c",
            }
        });

        const target_output = b.addInstallArtifact(keygen, .{
            .dest_dir = .{
                .override = .{
                    .custom = try t.zigTriple(b.allocator),
                },
            },
        });

        b.getInstallStep().dependOn(&target_output.step);
    }
}
