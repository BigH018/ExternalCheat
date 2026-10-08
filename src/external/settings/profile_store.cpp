#include "settings/profile_store.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <iterator>
#include <sstream>
#include <system_error>

#include "config.h"
#include "settings/profile_json.h"

namespace settings
{
namespace
{
namespace fs = std::filesystem;

// Windows device names can't be file names, even with an extension.
constexpr std::array<std::string_view, 22> kReservedNames = {
    "con",  "prn",  "aux",  "nul",  "com1", "com2", "com3", "com4", "com5", "com6", "com7",
    "com8", "com9", "lpt1", "lpt2", "lpt3", "lpt4", "lpt5", "lpt6", "lpt7", "lpt8", "lpt9",
};

std::string lower(std::string_view text)
{
    std::string out(text);
    std::transform(out.begin(), out.end(), out.begin(),
                   [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });
    return out;
}

bool name_char(char c, bool first)
{
    const auto u = static_cast<unsigned char>(c);
    if (std::isalnum(u) != 0 && u < 0x80)
    {
        return true;
    }
    return !first && (c == ' ' || c == '_' || c == '-');
}

std::string bad_name_message(std::string_view name)
{
    return "Invalid profile name \"" + std::string(name) + "\": use letters, digits, space, _ or - (max " +
           std::to_string(config::kProfileNameMaxLength) + ")";
}

std::optional<std::string> read_file(const fs::path& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
    {
        return std::nullopt;
    }
    std::ostringstream text;
    text << file.rdbuf();
    if (file.bad())
    {
        return std::nullopt;
    }
    return text.str();
}
} // namespace

std::optional<std::string> ProfileStore::clean_name(std::string_view name)
{
    const auto first = name.find_first_not_of(' ');
    if (first == std::string_view::npos)
    {
        return std::nullopt;
    }
    const auto last = name.find_last_not_of(' ');
    const std::string_view trimmed = name.substr(first, last - first + 1);
    if (trimmed.size() > config::kProfileNameMaxLength)
    {
        return std::nullopt;
    }
    for (std::size_t i = 0; i < trimmed.size(); ++i)
    {
        if (!name_char(trimmed[i], i == 0))
        {
            return std::nullopt;
        }
    }
    const std::string lowered = lower(trimmed);
    if (std::find(kReservedNames.begin(), kReservedNames.end(), lowered) != kReservedNames.end())
    {
        return std::nullopt;
    }
    return std::string(trimmed);
}

bool ProfileStore::is_read_only(std::string_view name)
{
    const std::optional<std::string> clean = clean_name(name);
    return clean && lower(*clean) == config::kDefaultProfile;
}

fs::path ProfileStore::path_for(const std::string& clean) const
{
    return folder_ / (clean + config::kProfileExtension);
}

std::vector<std::string> ProfileStore::list() const
{
    std::vector<std::string> names;
    std::error_code error;
    for (fs::directory_iterator it(folder_, error), end; !error && it != end; it.increment(error))
    {
        const fs::path& path = it->path();
        if (!it->is_regular_file(error) || path.extension() != config::kProfileExtension)
        {
            continue;
        }
        const std::string stem = path.stem().string();
        if (clean_name(stem) == stem && !is_read_only(stem))
        {
            names.push_back(stem);
        }
    }
    std::sort(names.begin(), names.end(),
              [](const std::string& a, const std::string& b) { return lower(a) < lower(b); });
    names.insert(names.begin(), config::kDefaultProfile);
    return names;
}

bool ProfileStore::exists(std::string_view name) const
{
    if (is_read_only(name))
    {
        return true;
    }
    const std::optional<std::string> clean = clean_name(name);
    std::error_code error;
    return clean && fs::is_regular_file(path_for(*clean), error);
}

ProfileStatus ProfileStore::load(std::string_view name, LoadedProfile& out) const
{
    const std::optional<std::string> clean = clean_name(name);
    if (!clean)
    {
        return ProfileStatus::failure(bad_name_message(name));
    }
    if (is_read_only(*clean))
    {
        out = LoadedProfile{config::kDefaultProfile, Settings{}, {}};
        return ProfileStatus::success();
    }
    const fs::path path = path_for(*clean);
    std::error_code error;
    if (!fs::is_regular_file(path, error))
    {
        return ProfileStatus::failure("Profile \"" + *clean + "\" does not exist");
    }
    const std::optional<std::string> text = read_file(path);
    if (!text)
    {
        return ProfileStatus::failure("Could not read profile \"" + *clean + "\"");
    }
    ParseResult parsed = from_json_text(*text);
    if (!parsed.ok)
    {
        return ProfileStatus::failure("Could not read profile \"" + *clean + "\": " + parsed.error);
    }
    out = LoadedProfile{*clean, std::move(parsed.settings), std::move(parsed.warnings)};
    return ProfileStatus::success();
}

ProfileStatus ProfileStore::save(std::string_view name, const Settings& settings) const
{
    const std::optional<std::string> clean = clean_name(name);
    if (!clean)
    {
        return ProfileStatus::failure(bad_name_message(name));
    }
    if (is_read_only(*clean))
    {
        return ProfileStatus::failure("\"default\" is read-only: use Save as");
    }
    const fs::path path = path_for(*clean);
    fs::path temporary = path;
    temporary += ".tmp";
    std::error_code error;
    fs::create_directories(folder_, error);
    {
        std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
        const std::string text = to_json_text(settings);
        file.write(text.data(), static_cast<std::streamsize>(text.size()));
        if (!file)
        {
            fs::remove(temporary, error);
            return ProfileStatus::failure("Could not write profile \"" + *clean + "\"");
        }
    } // closed (and flushed) before the replace
    fs::rename(temporary, path, error); // replaces an existing file (MoveFileEx with REPLACE_EXISTING underneath)
    if (error)
    {
        fs::remove(temporary, error);
        return ProfileStatus::failure("Could not save profile \"" + *clean + "\"");
    }
    return ProfileStatus::success();
}

ProfileStatus ProfileStore::rename(std::string_view from, std::string_view to) const
{
    const std::optional<std::string> old_name = clean_name(from);
    const std::optional<std::string> new_name = clean_name(to);
    if (!old_name)
    {
        return ProfileStatus::failure(bad_name_message(from));
    }
    if (!new_name)
    {
        return ProfileStatus::failure(bad_name_message(to));
    }
    if (is_read_only(*old_name) || is_read_only(*new_name))
    {
        return ProfileStatus::failure("\"default\" can't be renamed or replaced");
    }
    std::error_code error;
    if (!fs::is_regular_file(path_for(*old_name), error))
    {
        return ProfileStatus::failure("Profile \"" + *old_name + "\" does not exist");
    }
    // Case-only renames ("rage" -> "Rage") point at the same file on Windows: allow them.
    if (lower(*old_name) != lower(*new_name) && fs::exists(path_for(*new_name), error))
    {
        return ProfileStatus::failure("Profile \"" + *new_name + "\" already exists");
    }
    fs::rename(path_for(*old_name), path_for(*new_name), error);
    if (error)
    {
        return ProfileStatus::failure("Could not rename \"" + *old_name + "\"");
    }
    if (const std::optional<std::string> last = last_profile(); last && lower(*last) == lower(*old_name))
    {
        set_last_profile(*new_name);
    }
    return ProfileStatus::success();
}

ProfileStatus ProfileStore::remove(std::string_view name) const
{
    const std::optional<std::string> clean = clean_name(name);
    if (!clean)
    {
        return ProfileStatus::failure(bad_name_message(name));
    }
    if (is_read_only(*clean))
    {
        return ProfileStatus::failure("\"default\" can't be deleted");
    }
    std::error_code error;
    if (!fs::remove(path_for(*clean), error) || error)
    {
        return ProfileStatus::failure("Could not delete \"" + *clean + "\" (does it exist?)");
    }
    return ProfileStatus::success();
}

std::optional<std::string> ProfileStore::last_profile() const
{
    std::optional<std::string> text = read_file(folder_ / config::kLastProfileFile);
    if (!text)
    {
        return std::nullopt;
    }
    // A hand-edited file may end with a newline: drop line breaks and tabs, clean_name trims the spaces.
    std::erase_if(*text, [](char c) { return c == '\r' || c == '\n' || c == '\t'; });
    return clean_name(*text);
}

void ProfileStore::set_last_profile(std::string_view name) const
{
    const std::optional<std::string> clean = clean_name(name);
    if (!clean)
    {
        return;
    }
    std::error_code error;
    fs::create_directories(folder_, error);
    std::ofstream file(folder_ / config::kLastProfileFile, std::ios::binary | std::ios::trunc);
    file << *clean;
}

LoadedProfile ProfileStore::load_startup() const
{
    std::vector<std::string> problems;
    if (const std::optional<std::string> last = last_profile())
    {
        LoadedProfile profile;
        const ProfileStatus status = load(*last, profile);
        if (status.ok)
        {
            return profile;
        }
        problems.push_back(status.message + ": starting with \"default\"");
    }
    LoadedProfile profile{config::kDefaultProfile, Settings{}, std::move(problems)};
    return profile;
}
} // namespace settings
