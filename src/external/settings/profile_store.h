#pragma once

// Profiles on disk: <folder>/<name>.json, plus <folder>/.last_profile (same design as the AC project's ProfileStore).
//
// - "default" is built in: the code defaults (settings::Settings{}). It always exists, can't be saved over, renamed or
//   deleted ("Save as" a new name instead). profiles/default.json in the repo mirrors it (a test checks they're equal).
// - Saving is atomic: write <name>.json.tmp, then replace <name>.json with it, so a crash can't leave half a file.
// - Loading never fails on bad content (see profile_json.h); only a missing/unreadable file or invalid JSON is an
//   error.
// No Windows API (std::filesystem only), so the tests run it against a temporary folder. app/frame keeps the folder
// next to cs2_external.exe and calls this on the main thread (small files, rare operations).

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "settings/settings.h"

namespace settings
{
// The result of a profile operation: ok, or a message for the menu's notice line.
struct ProfileStatus
{
    bool ok = true;
    std::string message;

    static ProfileStatus success(std::string text = {}) { return ProfileStatus{true, std::move(text)}; }
    static ProfileStatus failure(std::string text) { return ProfileStatus{false, std::move(text)}; }
};

struct LoadedProfile
{
    std::string name;
    Settings settings;
    std::vector<std::string> warnings; // content problems (see profile_json.h)
};

class ProfileStore
{
public:
    explicit ProfileStore(std::filesystem::path folder) : folder_(std::move(folder)) {}

    // The cleaned name (trimmed), or nullopt if it isn't a safe file name: letters, digits, space, _ or -, starting
    // with a letter or digit, at most config::kProfileNameMaxLength, and not a Windows device name (CON, NUL, COM1...).
    static std::optional<std::string> clean_name(std::string_view name);
    static bool is_read_only(std::string_view name); // "default" (any case)

    // "default" first, then every saved profile alphabetically (case-insensitive).
    std::vector<std::string> list() const;
    bool exists(std::string_view name) const;

    // Load a profile ("default" = the code defaults). Fails on a bad name, a missing/unreadable file or invalid JSON.
    ProfileStatus load(std::string_view name, LoadedProfile& out) const;

    // Save over (or create) a profile. Refuses "default" and bad names.
    ProfileStatus save(std::string_view name, const Settings& settings) const;
    ProfileStatus rename(std::string_view from, std::string_view to) const;
    ProfileStatus remove(std::string_view name) const;

    std::optional<std::string> last_profile() const;
    void set_last_profile(std::string_view name) const; // best effort (a failure only means "default" next time)

    // What to start with: the last used profile, else "default". Never fails: a broken profile is skipped (its problem
    // is the first warning).
    LoadedProfile load_startup() const;

    const std::filesystem::path& folder() const { return folder_; }

private:
    std::filesystem::path path_for(const std::string& clean) const;

    std::filesystem::path folder_;
};
} // namespace settings
