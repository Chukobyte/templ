const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // templ has its own main(); seika_main.c's would clash on Linux/macOS.
    const seika_dep = b.dependency("seika", .{ .target = target, .optimize = optimize, .use_main = false });
    const seika_lib = seika_dep.artifact("seika");
    const cjson_lib = seika_dep.artifact("cjson");

    const templ_core_sources: []const []const u8 = &.{
        "src/platform.c",
        "src/config.c",
        "src/frontmatter.c",
        "src/markdown.c",
        "src/template.c",
        "src/engine.c",
        "src/theme.c",
        "src/themes/theme_default.c",
        "src/themes/theme_modern.c",
    };

    const lib_mod = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libc = true,
    });
    lib_mod.addCSourceFiles(.{
        .files = templ_core_sources,
        .flags = &.{"-std=c11", "-Wall", "-Wextra", "-Wno-unused-parameter", "-Wno-missing-field-initializers"},
    });
    lib_mod.addIncludePath(b.path("src"));
    lib_mod.addIncludePath(seika_dep.path("."));
    lib_mod.addIncludePath(seika_dep.path("thirdparty"));
    lib_mod.linkLibrary(seika_lib);
    lib_mod.linkLibrary(cjson_lib);
    // Mirrors CMakeLists.txt: strict -std=c11 hides POSIX bits (DT_DIR, etc.) on glibc/macOS.
    const posix_target = target.result.os.tag != .windows;
    if (posix_target) lib_mod.addCMacro("_DEFAULT_SOURCE", "1");
    const templ_lib = b.addLibrary(.{ .name = "templ_lib", .root_module = lib_mod, .linkage = .static });

    const exe_mod = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libc = true,
    });
    exe_mod.addCSourceFile(.{ .file = b.path("src/main.c"), .flags = &.{"-std=c11"} });
    exe_mod.addIncludePath(b.path("src"));
    exe_mod.addIncludePath(seika_dep.path("."));
    exe_mod.addIncludePath(seika_dep.path("thirdparty"));
    if (posix_target) exe_mod.addCMacro("_DEFAULT_SOURCE", "1");
    exe_mod.linkLibrary(templ_lib);
    if (target.result.os.tag == .linux) {
        // templ_lib -> seika_lib needs X11/GL, but they must be declared here at the final
        // executable module, not on templ_lib/seika_lib's own static-lib modules -- see
        // seika/build.zig's buildSeika Linux-branch comment (declaring a system *shared* library
        // on a module bound for a static-lib artifact makes zig/LLD try to fold the resolved .so
        // into the .a).
        exe_mod.linkSystemLibrary("X11", .{});
        exe_mod.linkSystemLibrary("GL", .{});
    }
    const exe = b.addExecutable(.{ .name = "templ", .root_module = exe_mod });
    b.installArtifact(exe);
}
