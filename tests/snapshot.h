#ifndef SNAPSHOT_H
#define SNAPSHOT_H 1

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "doctest/doctest.h"

namespace snapshot {

namespace fs = std::filesystem;

inline fs::path snapshots_dir() { return fs::path(DOCTEST_SOURCE_DIR) / "tests" / "snapshots"; }

inline bool should_update() {
    const char *env = std::getenv("UPDATE_SNAPSHOTS");
    return env != nullptr && std::string(env) == "1";
}

inline std::string read_file(const fs::path &path) {
    std::ifstream file(path);
    std::ostringstream buf;
    buf << file.rdbuf();
    return buf.str();
}

inline void write_file(const fs::path &path, const std::string &content) {
    fs::create_directories(path.parent_path());
    std::ofstream file(path);
    file << content;
}

template <typename T> void check(const T &obj, const std::string &snap_name) {
    std::ostringstream oss;
    oss << obj;
    std::string actual = oss.str();

    fs::path snap_path = snapshots_dir() / (snap_name + ".snap");

    if (should_update() || !fs::exists(snap_path)) {
        write_file(snap_path, actual);
        if (!should_update()) {
            MESSAGE("Created snapshot: ", snap_name);
        }
        return;
    }

    std::string expected = read_file(snap_path);
    REQUIRE(actual == expected);
}

inline void check_string(const std::string &actual, const std::string &snap_name) {
    fs::path snap_path = snapshots_dir() / (snap_name + ".snap");

    if (should_update() || !fs::exists(snap_path)) {
        write_file(snap_path, actual);
        if (!should_update()) {
            MESSAGE("Created snapshot: ", snap_name);
        }
        return;
    }

    std::string expected = read_file(snap_path);
    REQUIRE(actual == expected);
}

} // namespace snapshot

#endif
