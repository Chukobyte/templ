#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "seika/defines.h"
#include "seika/file_system.h"
#include "seika/logger.h"

#if SKA_HTTP_SERVER
#include "seika/networking/http_server.h"
#endif

#include "engine.h"
#include "config.h"
#include "platform.h"

#define CONFIG_FILENAME "tconf.json"

static void print_usage(void) {
    printf(
        "Usage: templ <command> [options]\n"
        "\n"
        "Run commands from the directory containing " CONFIG_FILENAME " (or the file named by --config).\n"
        "\n"
        "Commands:\n"
        "  build [--config <file>] [--theme <name>]  Build the site\n"
        "  serve [--config <file>] [--port <n>] [--theme <name>]  Build then serve (default port: 8080)\n"
        "  deploy [--config <file>]  Build then force-push output to the gh-pages branch\n"
        "  new [<name>]              Write a starter " CONFIG_FILENAME " (in <name>/ or current dir)\n"
    );
}

static const char* get_flag(int argc, char** argv, const char* flag, const char* fallback) {
    for (int i = 1; i + 1 < argc; i++) {
        if (strcmp(argv[i], flag) == 0) return argv[i + 1];
    }
    return fallback;
}

static const char* default_config =
    "{\n"
    "  \"site\": {\n"
    "    \"title\": \"My Site\",\n"
    "    \"base_url\": \"\"\n"
    "  },\n"
    "  \"paths\": {\n"
    "    \"content\":   \"content\",\n"
    "    \"templates\": \"templates\",\n"
    "    \"assets\":    \"assets\",\n"
    "    \"output\":    \"dist\"\n"
    "  },\n"
    "  \"defaults\": {\n"
    "    \"layout\": \"base\"\n"
    "  },\n"
    "  \"collections\": {}\n"
    "}\n";

#ifdef _WIN32
#  define tpl_popen  _popen
#  define tpl_pclose _pclose
#else
#  define tpl_popen  popen
#  define tpl_pclose pclose
#endif

// Runs cmd and captures its trimmed stdout into out[out_size]. Returns false if
// the command exits non-zero or produces no output.
static bool run_capture(const char* cmd_str, char* out, usize out_size) {
    FILE* f = tpl_popen(cmd_str, "r");
    if (!f) return false;
    usize n = fread(out, 1, out_size - 1, f);
    out[n] = '\0';
    while (n > 0 && (out[n-1] == '\n' || out[n-1] == '\r' || out[n-1] == ' '))
        out[--n] = '\0';
    return tpl_pclose(f) == 0 && n > 0;
}

// Runs "git -C <dir> <subcmd>" and returns true on success.
static bool git_in(const char* dir, const char* subcmd) {
    char buf[TPL_PATH_MAX * 4];
    snprintf(buf, sizeof(buf), "git -C \"%s\" %s", dir, subcmd);
    return system(buf) == 0;
}

static bool cmd_deploy(const char* config_filename) {
    // 1. Build (no theme override here; deploy respects config)
    ska_logger_message("templ: building...");
    TplEngineOpts deploy_opts = { .config = config_filename };
    if (!tpl_engine_build(".", &deploy_opts)) return false;

    // 2. Read config for output path
    TplConfig cfg;
    if (!tpl_config_load(config_filename, &cfg)) return false;
    const char* out = cfg.path_output;

    // 3. Verify we have a git remote to push to
    char remote_url[TPL_PATH_MAX];
    if (!run_capture("git remote get-url origin", remote_url, sizeof(remote_url))) {
        fprintf(stderr, "templ deploy: could not read git remote 'origin'\n"
                        "              Run from inside a git repo that has a remote named 'origin'.\n");
        return false;
    }

    // 4. Drop .nojekyll so GitHub Pages serves the files as-is (no Jekyll)
    char nojekyll[TPL_PATH_MAX * 2];
    snprintf(nojekyll, sizeof(nojekyll), "%s/.nojekyll", out);
    ska_fs_write_to_file(nojekyll, "");

    // 5. Remove a stale .git left by a previous deploy run
    char dot_git[TPL_PATH_MAX * 2];
    snprintf(dot_git, sizeof(dot_git), "%s/.git", out);
    if (ska_fs_does_dir_exist(dot_git)) tpl_rmdir_recursive(dot_git);

    // 6. Bootstrap a throw-away repo, commit everything, force-push to gh-pages
    char remote_cmd[TPL_PATH_MAX * 2];
    snprintf(remote_cmd, sizeof(remote_cmd), "remote add origin \"%s\"", remote_url);

    bool ok =
        git_in(out, "init -q")                        &&
        git_in(out, "add -A")                         &&
        git_in(out, "commit -q -m \"Deploy\"")        &&
        git_in(out, remote_cmd)                       &&
        git_in(out, "push --force origin HEAD:gh-pages");

    // 7. Always clean up the throw-away .git
    if (ska_fs_does_dir_exist(dot_git)) tpl_rmdir_recursive(dot_git);

    if (!ok) {
        fprintf(stderr, "templ deploy: push failed\n");
        return false;
    }

    ska_logger_message("templ: deployed → gh-pages  (%s)", remote_url);
    return true;
}

static bool cmd_new(const char* name) {
    char cfg_path[TPL_PATH_MAX];

    if (name) {
        if (ska_fs_does_dir_exist(name)) {
            fprintf(stderr, "templ new: '%s' already exists\n", name);
            return false;
        }
        tpl_mkdir_p(name);
        snprintf(cfg_path, sizeof(cfg_path), "%s/" CONFIG_FILENAME, name);
    } else {
        snprintf(cfg_path, sizeof(cfg_path), CONFIG_FILENAME);
    }

    if (ska_fs_does_file_exist(cfg_path)) {
        fprintf(stderr, "templ new: '%s' already exists\n", cfg_path);
        return false;
    }

    if (!ska_fs_write_to_file(cfg_path, default_config)) {
        fprintf(stderr, "templ new: failed to write '%s'\n", cfg_path);
        return false;
    }

    printf("Created %s\n", cfg_path);
    printf("Edit it to point to your content, templates, assets, and output directories,\n");
    printf("then run 'templ build' from the same directory.\n");
    return true;
}

int main(int argc, char** argv) {
    ska_logger_set_level(SkaLogLevel_INFO);

    if (argc < 2) { print_usage(); return 0; }

    const char* cmd = argv[1];
    const char* config = get_flag(argc, argv, "--config", CONFIG_FILENAME);

    if (strcmp(cmd, "build") == 0) {
        if (!ska_fs_does_file_exist(config)) {
            fprintf(stderr, "templ: no '%s' in current directory\n", config);
            return 1;
        }
        TplEngineOpts opts = { .theme = get_flag(argc, argv, "--theme", NULL), .config = config };
        return tpl_engine_build(".", &opts) ? 0 : 1;
    }

    if (strcmp(cmd, "serve") == 0) {
        if (!ska_fs_does_file_exist(config)) {
            fprintf(stderr, "templ: no '%s' in current directory\n", config);
            return 1;
        }
        TplEngineOpts opts = { .theme = get_flag(argc, argv, "--theme", NULL), .config = config };
        if (!tpl_engine_build(".", &opts)) return 1;

#if SKA_HTTP_SERVER
        TplConfig cfg;
        tpl_config_load(config, &cfg);

        const char* port_str = get_flag(argc, argv, "--port", "8080");
        SkaHttpServerConfig http_cfg = {
            .root_dir = cfg.path_output,
            .port     = (i32)atoi(port_str),
        };
        ska_http_server_serve(http_cfg);
#else
        fprintf(stderr, "templ: HTTP server not compiled in (rebuild with SKA_HTTP_SERVER=1)\n");
        return 1;
#endif
        return 0;
    }

    if (strcmp(cmd, "deploy") == 0) {
        if (!ska_fs_does_file_exist(config)) {
            fprintf(stderr, "templ: no '%s' in current directory\n", config);
            return 1;
        }
        return cmd_deploy(config) ? 0 : 1;
    }

    if (strcmp(cmd, "new") == 0) {
        const char* name = (argc >= 3) ? argv[2] : NULL;
        return cmd_new(name) ? 0 : 1;
    }

    fprintf(stderr, "templ: unknown command '%s'\n", cmd);
    print_usage();
    return 1;
}
