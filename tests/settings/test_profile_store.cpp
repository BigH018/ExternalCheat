// settings/profile_store against a temporary folder.

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <doctest.h>

#include "settings/profile_store.h"

namespace
{
namespace fs = std::filesystem;

// A fresh, empty folder under %TEMP%, removed again at the end of the test.
class TempFolder
{
public:
    TempFolder()
    {
        static int counter = 0;
        path_ = fs::temp_directory_path() / ("cs2_external_profiles_test_" + std::to_string(++counter));
        fs::remove_all(path_);
        fs::create_directories(path_);
    }
    ~TempFolder()
    {
        std::error_code ignored;
        fs::remove_all(path_, ignored);
    }
    TempFolder(const TempFolder&) = delete;
    TempFolder& operator=(const TempFolder&) = delete;

    [[nodiscard]] const fs::path& path() const { return path_; }

private:
    fs::path path_;
};

void write(const fs::path& path, const std::string& text)
{
    std::ofstream(path, std::ios::binary) << text;
}

settings::Settings custom()
{
    settings::Settings s;
    s.aimbot.enabled = true;
    s.radar.corner = settings::RadarCorner::bottom_left;
    return s;
}
} // namespace

TEST_CASE("profile store: names are safe file names, trimmed")
{
    using settings::ProfileStore;
    CHECK(ProfileStore::clean_name("rage") == "rage");
    CHECK(ProfileStore::clean_name("  my setup-2_b ") == "my setup-2_b");
    CHECK_FALSE(ProfileStore::clean_name("").has_value());
    CHECK_FALSE(ProfileStore::clean_name("   ").has_value());
    CHECK_FALSE(ProfileStore::clean_name("../evil").has_value());
    CHECK_FALSE(ProfileStore::clean_name("a/b").has_value());
    CHECK_FALSE(ProfileStore::clean_name("a\\b").has_value());
    CHECK_FALSE(ProfileStore::clean_name("C:x").has_value());
    CHECK_FALSE(ProfileStore::clean_name("-dash first").has_value());
    CHECK_FALSE(ProfileStore::clean_name("CON").has_value());
    CHECK_FALSE(ProfileStore::clean_name("lpt1").has_value());
    CHECK_FALSE(ProfileStore::clean_name(std::string(41, 'a')).has_value());
    CHECK(ProfileStore::clean_name(std::string(40, 'a')).has_value());
    CHECK(ProfileStore::is_read_only("default"));
    CHECK(ProfileStore::is_read_only(" Default "));
    CHECK_FALSE(ProfileStore::is_read_only("defaults"));
}

TEST_CASE("profile store: save, list and load")
{
    TempFolder folder;
    const settings::ProfileStore store(folder.path());
    CHECK(store.list() == std::vector<std::string>{"default"});
    REQUIRE(store.save("Rage", custom()).ok);
    REQUIRE(store.save("chill", settings::Settings{}).ok);
    write(folder.path() / "notes.txt", "not a profile");
    write(folder.path() / "bad name!.json", "{}");
    CHECK(store.list() == std::vector<std::string>{"default", "chill", "Rage"});
    CHECK(store.exists("Rage"));
    CHECK(store.exists("default"));
    CHECK_FALSE(store.exists("missing"));

    settings::LoadedProfile loaded;
    REQUIRE(store.load("Rage", loaded).ok);
    CHECK(loaded.name == "Rage");
    CHECK(loaded.settings == custom());
    CHECK(loaded.warnings.empty());
    CHECK_FALSE(fs::exists(folder.path() / "Rage.json.tmp")); // the atomic save cleaned up
}

TEST_CASE("profile store: saving creates the folder and replaces an existing profile")
{
    TempFolder folder;
    const settings::ProfileStore store(folder.path() / "profiles");
    REQUIRE(store.save("mine", settings::Settings{}).ok);
    REQUIRE(store.save("mine", custom()).ok);
    settings::LoadedProfile loaded;
    REQUIRE(store.load("mine", loaded).ok);
    CHECK(loaded.settings == custom());
}

TEST_CASE("profile store: default is built in and read-only")
{
    TempFolder folder;
    const settings::ProfileStore store(folder.path());
    settings::LoadedProfile loaded;
    REQUIRE(store.load("default", loaded).ok);
    CHECK(loaded.settings == settings::Settings{});
    CHECK_FALSE(store.save("default", custom()).ok);
    CHECK_FALSE(store.remove("default").ok);
    REQUIRE(store.save("x", custom()).ok);
    CHECK_FALSE(store.rename("x", "default").ok);
    CHECK_FALSE(store.rename("default", "y").ok);
    write(folder.path() / "default.json", R"({"aimbot": {"enabled": true}})"); // a hand-made file doesn't count
    REQUIRE(store.load("default", loaded).ok);
    CHECK(loaded.settings == settings::Settings{});
    CHECK(store.list() == std::vector<std::string>{"default", "x"});
}

TEST_CASE("profile store: loading: missing, invalid JSON, and content warnings")
{
    TempFolder folder;
    const settings::ProfileStore store(folder.path());
    settings::LoadedProfile loaded;
    CHECK_FALSE(store.load("missing", loaded).ok);
    CHECK_FALSE(store.load("../x", loaded).ok);
    write(folder.path() / "broken.json", "{ nope");
    CHECK_FALSE(store.load("broken", loaded).ok);
    write(folder.path() / "odd.json", R"({"aimbot": {"fov": 999}})");
    REQUIRE(store.load("odd", loaded).ok);
    CHECK(loaded.warnings.size() == 1);
}

TEST_CASE("profile store: rename and delete")
{
    TempFolder folder;
    const settings::ProfileStore store(folder.path());
    REQUIRE(store.save("a", custom()).ok);
    REQUIRE(store.save("b", settings::Settings{}).ok);
    store.set_last_profile("a");
    CHECK_FALSE(store.rename("a", "b").ok); // the target exists
    CHECK_FALSE(store.rename("missing", "c").ok);
    REQUIRE(store.rename("a", "c").ok);
    CHECK_FALSE(store.exists("a"));
    CHECK(store.exists("c"));
    CHECK(store.last_profile() == "c"); // the last-used marker follows the rename
    REQUIRE(store.rename("c", "C").ok); // a case-only rename is allowed
    REQUIRE(store.remove("b").ok);
    CHECK_FALSE(store.exists("b"));
    CHECK_FALSE(store.remove("b").ok);
}

TEST_CASE("profile store: last profile and startup")
{
    TempFolder folder;
    const settings::ProfileStore store(folder.path());
    CHECK_FALSE(store.last_profile().has_value());
    settings::LoadedProfile start = store.load_startup();
    CHECK(start.name == "default");
    CHECK(start.warnings.empty());

    REQUIRE(store.save("mine", custom()).ok);
    store.set_last_profile("mine");
    start = store.load_startup();
    CHECK(start.name == "mine");
    CHECK(start.settings == custom());

    write(folder.path() / ".last_profile", "mine\r\n"); // hand-edited, with a line break
    CHECK(store.last_profile() == "mine");

    store.set_last_profile("gone");
    start = store.load_startup();
    CHECK(start.name == "default");
    CHECK(start.settings == settings::Settings{});
    REQUIRE(start.warnings.size() == 1);
    CHECK(start.warnings[0].find("does not exist") != std::string::npos);

    write(folder.path() / "broken.json", "{ nope");
    store.set_last_profile("broken");
    start = store.load_startup();
    CHECK(start.name == "default");
    REQUIRE(start.warnings.size() == 1);
    CHECK(start.warnings[0].find("not valid JSON") != std::string::npos);
}
