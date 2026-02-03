/*
 * changes done to nob.h:
 * - removed emotes from nob_cancer_log_handler (RIP Windows Terminal)
 * - add silent version of nob_delete_file
 * - add nob_delete_directory
 */

#define NOB_IMPLEMENTATION
#include "nob.h"

#define LLVM_VERSION (version)
#define BUILD_MODE (build_mode)
#define LINK_MODE (link_mode)
#define PROJECTS (projects)
#define SOURCE_DIR (temp_sprintf("./llvm-%s", LLVM_VERSION))
#define BUILD_DIR (temp_sprintf("%s/build", SOURCE_DIR))
#define INSTALL_DIR_NAME                                                       \
    (temp_sprintf("%s-%s-%s-%s", pathify(PROJECTS), LLVM_VERSION, BUILD_MODE,  \
                  LINK_MODE))
#define INSTALL_DIR (temp_sprintf("./%s", INSTALL_DIR_NAME))
#define ARCHIVE_NAME (temp_sprintf("%s.7z", INSTALL_DIR_NAME))

#define CHK_DIR (temp_sprintf("./chk-%s", INSTALL_DIR_NAME))
#define CHK_CONF (temp_sprintf("%s/chk_conf", CHK_DIR))
#define CHK_LLVM (temp_sprintf("%s/chk_llvm", CHK_DIR))
#define CHK_7ZIP (temp_sprintf("%s/chk_7zip", CHK_DIR))

typedef enum Link_Mode {
    LINK_STATIC = 0,
    LINK_DYNAMIC,
} Link_Mode;

typedef struct Config {
    const char *version;
    const char *build_mode;
    Link_Mode link_mode;
    const char *projects;
    const char *targets;
} Config;
#define CONF(v, b, l, p, t)                                                    \
    {.version = (v),                                                           \
     .build_mode = (b),                                                        \
     .link_mode = (l),                                                         \
     .projects = (p),                                                          \
     .targets = (t)}

// Comment out this line to start the process.
#define DRY_RUN

// Choose the configurations that you want to build here.
const Config CONFIGS[] = {
    CONF("22.1.0-rc2", "Release", LINK_STATIC, "mlir;lldb;clang;lld", "host"),
    CONF("22.1.0-rc2", "Release", LINK_DYNAMIC, "mlir;lldb;clang;lld", "host"),
    CONF("21.1.8", "Release", LINK_STATIC, "mlir;lldb;clang;lld", "host"),
    CONF("21.1.8", "Release", LINK_DYNAMIC, "mlir;lldb;clang;lld", "host"),
    CONF("20.1.7", "Release", LINK_STATIC, "lldb;clang;lld", "host"),
    CONF("20.1.7", "Release", LINK_DYNAMIC, "lldb;clang;lld", "host"),
};

const char *pathify(const char *projects) {
    size_t len = strlen(projects);
    if (len == 0) {
        return "llvm";
    }
    char *_path = temp_alloc(len + 1);
    assert(_path != NULL);
    _path[len] = '\0';
    memcpy(_path, projects, len);
    for (size_t i = 0; i < len; i++) {
        if (_path[i] == ';')
            _path[i] = '-';
    }
    const char *path = temp_sprintf("llvm-%s", _path);
    return path;
}

Cmd cmd = {0};

bool _cmd_run(Cmd *cmd) {
#ifdef DRY_RUN
    String_Builder sb = {0};
    cmd_render(*cmd, &sb);
    nob_log(NOB_INFO, "%.*s", (int)sb.count, sb.items);
    cmd->count = 0;
    return true;
#else
    return cmd_run(cmd);
#endif
}

const char *get_link_mode(Link_Mode mode) {
    switch (mode) {
    case LINK_STATIC:
        return "Static";
    case LINK_DYNAMIC:
        return "Dynamic";
    }
    UNREACHABLE("get_link_mode");
}

void touch_file(const char *path) {
#ifdef DRY_RUN
    (void)path;
#else
    if (file_exists(path))
        return;
    write_entire_file(path, NULL, 0);
#endif
}

bool clone_repo(Config config) {
    const char *version = config.version;
    if (file_exists(SOURCE_DIR)) {
        nob_log(NOB_INFO, "Directory %s exists, skipping git clone.",
                SOURCE_DIR);
        return true;
    }
#ifndef DRY_RUN
    if (!mkdir_if_not_exists(SOURCE_DIR))
        return false;
#endif
    const char *branch = temp_sprintf("llvmorg-%s", LLVM_VERSION);
    cmd_append(&cmd, "git", "clone", "--single-branch", "--branch", branch,
               "--depth", "1", "https://github.com/llvm/llvm-project.git",
               SOURCE_DIR);
    return _cmd_run(&cmd);
}

bool config_project(Config config) {
    const char *version = config.version;
    const char *build_mode = config.build_mode;
    Link_Mode mode = config.link_mode;
    const char *link_mode = get_link_mode(mode);
    const char *projects = config.projects;
    if (file_exists(CHK_CONF)) {
        nob_log(NOB_INFO, "Check file %s exists, skipping configuration step.",
                CHK_CONF);
        return true;
    }
#ifndef DRY_RUN
    if (file_exists(BUILD_DIR)) {
        nob_log(NOB_WARNING, "Previous build directory %s exists. Deleting it.",
                BUILD_DIR);
        nob_log(NOB_INFO, "This may be because you're building the same LLVM "
                          "version multiple times.");
        if (!delete_directory(BUILD_DIR))
            return false;
    }
#endif
    cmd_append(&cmd, "cmake", "-S", temp_sprintf("%s/llvm", SOURCE_DIR), "-B",
               BUILD_DIR, "-G", "Ninja", "-DLLVM_USE_LINKER=lld",
               temp_sprintf("-DCMAKE_BUILD_TYPE=%s", BUILD_MODE),
               temp_sprintf("-DCMAKE_INSTALL_PREFIX=%s", INSTALL_DIR),
               "-DLLVM_CCACHE_BUILD=ON");
    if (strlen(config.projects) > 0) {
        cmd_append(&cmd,
                   temp_sprintf("-DLLVM_ENABLE_PROJECTS=%s", config.projects));
    }
    if (strlen(config.targets) > 0) {
        cmd_append(&cmd,
                   temp_sprintf("-DLLVM_TARGETS_TO_BUILD=%s", config.targets));
    }
#ifdef _WIN32
    cmd_append(&cmd, "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded");
    cmd_append(
        &cmd,
        "-DCMAKE_CXX_FLAGS=-D_SILENCE_NONFLOATING_COMPLEX_DEPRECATION_WARNING");
#endif
    if (mode == LINK_DYNAMIC) {
        cmd_append(&cmd, "-DLLVM_BUILD_LLVM_DYLIB=ON");
        cmd_append(&cmd, "-DLLVM_LINK_LLVM_DYLIB=ON");
    }
    bool succ = _cmd_run(&cmd);
    if (succ)
        touch_file(CHK_CONF);
    return succ;
}

bool build_project(Config config) {
    const char *version = config.version;
    const char *build_mode = config.build_mode;
    Link_Mode mode = config.link_mode;
    const char *link_mode = get_link_mode(mode);
    const char *projects = config.projects;
    if (file_exists(CHK_LLVM)) {
        nob_log(NOB_INFO, "Check file %s exists, skipping build step.",
                CHK_LLVM);
        return true;
    }
    cmd_append(&cmd, "cmake", "--build", BUILD_DIR, "--target", "install",
               "--config", BUILD_MODE, "-j", "12");
    bool succ = _cmd_run(&cmd);
    if (succ)
        touch_file(CHK_LLVM);
    return succ;
}

bool archive_project(Config config) {
    const char *version = config.version;
    const char *build_mode = config.build_mode;
    Link_Mode mode = config.link_mode;
    const char *link_mode = get_link_mode(mode);
    const char *projects = config.projects;
    if (file_exists(CHK_7ZIP)) {
        nob_log(NOB_INFO, "Check file %s exists, skipping archiving step.",
                CHK_7ZIP);
        return true;
    }
    cmd_append(&cmd, "7z", "a", "-mx9", ARCHIVE_NAME,
               temp_sprintf("%s/*", INSTALL_DIR));
    bool succ = _cmd_run(&cmd);
    if (succ)
        touch_file(CHK_7ZIP);
    return succ;
}

bool build_llvm_toolchain(Config config) {
#ifndef DRY_RUN
    const char *version = config.version;
    const char *build_mode = config.build_mode;
    Link_Mode mode = config.link_mode;
    const char *link_mode = get_link_mode(config.link_mode);
    const char *projects = config.projects;
    if (!mkdir_if_not_exists(CHK_DIR))
        return false;
#endif
    if (!clone_repo(config))
        return false;
    if (!config_project(config))
        return false;
    if (!build_project(config))
        return false;
    if (!archive_project(config))
        return false;
    return true;
}

int main(int argc, char **argv) {
    GO_REBUILD_URSELF(argc, argv);
    set_log_handler(cancer_log_handler);
    for (size_t i = 0; i < ARRAY_LEN(CONFIGS); i++) {
        if (!build_llvm_toolchain(CONFIGS[i]))
            return false;
    }
#ifdef DRY_RUN
    nob_log(NOB_WARNING, "This was a dry run. No command has been executed.");
    nob_log(NOB_WARNING,
            "Comment out `#define DRY_RUN` in ./nob.h to start the process.");
    nob_log(
        NOB_WARNING,
        "The process may take a while. It will run all commands shown above.");
    nob_log(NOB_INFO, "Intermediate steps will be cached.");
    nob_log(NOB_INFO,
            "With your current settings, you will build %zu versions of LLVM:",
            ARRAY_LEN(CONFIGS));
    for (size_t i = 0; i < ARRAY_LEN(CONFIGS); i++) {
        Config config = CONFIGS[i];
        const char *version = config.version;
        const char *build_mode = config.build_mode;
        Link_Mode mode = config.link_mode;
        const char *link_mode = get_link_mode(config.link_mode);
        const char *projects = config.projects;
        nob_log(NOB_INFO, "> %s", INSTALL_DIR_NAME);
    }
#endif
    return 0;
}
