#include "settings/profile_json.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <exception>
#include <format>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <nlohmann/json.hpp>

#include "color.h"
#include "config.h"
#include "input/actions.h"
#include "input/keys.h"

namespace settings
{
namespace
{
// ordered_json keeps the fields in the order we write them, so a saved profile reads like the menu.
using Json = nlohmann::ordered_json;

constexpr char kSchemaKey[] = "schema_version";
constexpr int kJsonIndent = 2;
constexpr double kFloatDecimals = 10000.0; // floats are written rounded to 4 decimals (1.5, not 1.5000000596)

// Enum <-> text, in enum order.
constexpr std::array<std::string_view, 2> kTeamModes = {"teams", "free_for_all"};
constexpr std::array<std::string_view, 2> kBoxStyles = {"full", "corners"};
constexpr std::array<std::string_view, 3> kSnaplineOrigins = {"bottom", "centre", "top"};
constexpr std::array<std::string_view, 3> kAimTargets = {"head", "body", "nearest"};
constexpr std::array<std::string_view, 3> kAimPriorities = {"crosshair", "distance", "lowest_health"};
constexpr std::array<std::string_view, 2> kTriggerActivations = {"always", "key"};
constexpr std::array<std::string_view, 3> kFireModes = {"single", "burst", "hold"};
constexpr std::array<std::string_view, 4> kRadarCorners = {"top_left", "top_right", "bottom_left", "bottom_right"};
constexpr std::array<std::string_view, 2> kPanelSides = {"left", "right"};
constexpr std::array<std::string_view, 3> kBindModes = {"hold", "toggle", "press"}; // input::BindMode order
constexpr std::array<std::string_view, 5> kThemes = {"midnight", "black", "graphite", "violet", "ice"};

static_assert(static_cast<std::size_t>(BoxStyle::corners) == kBoxStyles.size() - 1);
static_assert(static_cast<std::size_t>(SnaplineOrigin::top) == kSnaplineOrigins.size() - 1);
static_assert(static_cast<std::size_t>(AimTarget::nearest) == kAimTargets.size() - 1);
static_assert(static_cast<std::size_t>(AimPriority::lowest_health) == kAimPriorities.size() - 1);
static_assert(static_cast<std::size_t>(FireMode::hold) == kFireModes.size() - 1);
static_assert(static_cast<std::size_t>(RadarCorner::bottom_right) == kRadarCorners.size() - 1);
static_assert(static_cast<std::size_t>(input::BindMode::press) == kBindModes.size() - 1);
static_assert(kMenuThemes.size() == kThemes.size() && static_cast<std::size_t>(MenuTheme::ice) == kThemes.size() - 1);

// --- colours ------------------------------------------------------------------------------------------------------

std::string colour_text(const Color& c)
{
    return std::format("#{:08X}", c.to_rgba());
}

// "#RRGGBBAA", or "#RRGGBB" (opaque). Either case.
std::optional<Color> parse_colour(const std::string& text)
{
    if ((text.size() != 7 && text.size() != 9) || text[0] != '#')
    {
        return std::nullopt;
    }
    std::uint32_t value = 0;
    for (std::size_t i = 1; i < text.size(); ++i)
    {
        const char c = text[i];
        std::uint32_t digit = 0;
        if (c >= '0' && c <= '9')
        {
            digit = static_cast<std::uint32_t>(c - '0');
        }
        else if (c >= 'a' && c <= 'f')
        {
            digit = static_cast<std::uint32_t>(c - 'a' + 10);
        }
        else if (c >= 'A' && c <= 'F')
        {
            digit = static_cast<std::uint32_t>(c - 'A' + 10);
        }
        else
        {
            return std::nullopt;
        }
        value = (value << 4) | digit;
    }
    if (text.size() == 7)
    {
        value = (value << 8) | 0xFF;
    }
    return Color::rgba(value);
}

// --- writer -------------------------------------------------------------------------------------------------------

class Writer
{
public:
    explicit Writer(Json& object) : object_(object) {}

    void boolean(const char* key, const bool& value) { object_[key] = value; }

    template <class T>
    void number(const char* key, const T& value, config::Range<T> /*range*/)
    {
        if constexpr (std::is_floating_point_v<T>)
        {
            object_[key] = std::round(static_cast<double>(value) * kFloatDecimals) / kFloatDecimals;
        }
        else
        {
            object_[key] = value;
        }
    }

    void colour(const char* key, const Color& value) { object_[key] = colour_text(value); }

    template <class E, std::size_t N>
    void choice(const char* key, const E& value, const std::array<std::string_view, N>& names)
    {
        object_[key] = std::string(names[static_cast<std::size_t>(value)]);
    }

    // A nested object ("colours", "weapons"), written by `visit(writer)`.
    template <class Fn>
    void section(const char* key, Fn&& visit)
    {
        Json child = Json::object();
        Writer writer(child);
        visit(writer);
        object_[key] = std::move(child);
    }

private:
    Json& object_;
};

// --- reader -------------------------------------------------------------------------------------------------------

// Reads one JSON object field by field into a settings struct, keeping the default for anything missing or wrong and
// recording every problem as "path.key: what happened".
class Reader
{
public:
    Reader(const Json* object, std::string path, std::vector<std::string>& warnings)
        : object_(object), path_(std::move(path)), warnings_(warnings)
    {
        if (object_ != nullptr && !object_->is_object())
        {
            warnings_.push_back(path_ + ": expected an object, using defaults");
            object_ = nullptr;
        }
    }

    void boolean(const char* key, bool& value)
    {
        const Json* field = find(key);
        if (field == nullptr)
        {
            return;
        }
        if (!field->is_boolean())
        {
            warn(key, "expected true/false, using default");
            return;
        }
        value = field->get<bool>();
    }

    template <class T>
    void number(const char* key, T& value, config::Range<T> range)
    {
        const Json* field = find(key);
        if (field == nullptr)
        {
            return;
        }
        if (!field->is_number() || !std::isfinite(field->get<double>()))
        {
            warn(key, "expected a number, using default");
            return;
        }
        const double raw = field->get<double>();
        // Clamp in double first, so a huge value can't overflow the conversion to an int.
        const double clamped = std::clamp(raw, static_cast<double>(range.min), static_cast<double>(range.max));
        T converted{};
        bool adjusted = clamped != raw; // out of range
        if constexpr (std::is_integral_v<T>)
        {
            converted = static_cast<T>(std::llround(clamped));
            adjusted = adjusted || static_cast<double>(converted) != clamped; // a fraction in a whole-number field
        }
        else
        {
            converted = static_cast<T>(clamped); // float precision alone is never worth a warning
        }
        if (adjusted)
        {
            warn(key, std::format("{} adjusted to {} (range/rounding)", field->dump(), converted));
        }
        value = converted;
    }

    void colour(const char* key, Color& value)
    {
        const Json* field = find(key);
        if (field == nullptr)
        {
            return;
        }
        const std::optional<Color> parsed =
            field->is_string() ? parse_colour(field->get<std::string>()) : std::optional<Color>{};
        if (!parsed)
        {
            warn(key, "expected #RRGGBB or #RRGGBBAA, using default");
            return;
        }
        value = *parsed;
    }

    template <class E, std::size_t N>
    void choice(const char* key, E& value, const std::array<std::string_view, N>& names)
    {
        const Json* field = find(key);
        if (field == nullptr)
        {
            return;
        }
        if (field->is_string())
        {
            const std::string text = field->get<std::string>();
            for (std::size_t i = 0; i < N; ++i)
            {
                if (names[i] == text)
                {
                    value = static_cast<E>(i);
                    return;
                }
            }
        }
        std::string allowed;
        for (const std::string_view name : names)
        {
            allowed += (allowed.empty() ? "" : ", ") + std::string(name);
        }
        warn(key, "expected one of " + allowed + ", using default");
    }

    // A nested object ("colours", "weapons"), read by `visit(reader)`. Missing = every field keeps its default.
    template <class Fn>
    void section(const char* key, Fn&& visit)
    {
        Reader reader(find(key), join(key), warnings_);
        visit(reader);
        reader.finish();
    }

    // A field read by hand (the keybinds); marks it as known.
    const Json* child(const char* key) { return find(key); }

    // Call after reading every known field: anything left over is unknown.
    void finish()
    {
        if (object_ == nullptr)
        {
            return;
        }
        for (auto it = object_->begin(); it != object_->end(); ++it)
        {
            if (seen_.count(it.key()) == 0)
            {
                warn(it.key().c_str(), "unknown setting, ignored");
            }
        }
    }

    void warn(const char* key, const std::string& what) { warnings_.push_back(join(key) + ": " + what); }

private:
    // "esp" + "thickness" -> "esp.thickness"; the top level has no path of its own.
    std::string join(const char* key) const { return path_.empty() ? std::string(key) : path_ + "." + key; }

    const Json* find(const char* key)
    {
        seen_.insert(key);
        if (object_ == nullptr)
        {
            return nullptr;
        }
        const auto it = object_->find(key);
        return it == object_->end() ? nullptr : &*it;
    }

    const Json* object_;
    std::string path_;
    std::vector<std::string>& warnings_;
    std::set<std::string> seen_;
};

// --- the field lists (one per section, used for both writing and reading) ------------------------------------------
// V is a Writer or a Reader; S is the section, const for writing. Keep each list in the order its page shows it.

template <class V, class S>
void visit_overlay(V& v, S& s)
{
    v.choice("theme", s.theme, kThemes);
    v.colour("accent", s.accent);
    v.boolean("watermark", s.watermark);
    v.boolean("frame_outline", s.frame_outline);
}

template <class V, class S>
void visit_general(V& v, S& s)
{
    v.choice("team_mode", s.team_mode, kTeamModes);
}

template <class V, class S>
void visit_esp_colours(V& v, S& s)
{
    v.colour("enemy_visible", s.enemy_visible);
    v.colour("enemy_hidden", s.enemy_hidden);
    v.colour("team_visible", s.team_visible);
    v.colour("team_hidden", s.team_hidden);
    v.colour("skeleton", s.skeleton);
    v.colour("text", s.text);
    v.colour("scoped", s.scoped);
}

template <class V, class S>
void visit_esp(V& v, S& s)
{
    v.boolean("enabled", s.enabled);
    v.boolean("show_teammates", s.show_teammates);
    v.number("max_distance", s.max_distance, config::kMaxDistance);
    v.boolean("box", s.box);
    v.choice("box_style", s.box_style, kBoxStyles);
    v.number("thickness", s.thickness, config::kEspThickness);
    v.boolean("outline", s.outline);
    v.boolean("head_circle", s.head_circle);
    v.boolean("skeleton", s.skeleton);
    v.boolean("name", s.name);
    v.boolean("health_bar", s.health_bar);
    v.boolean("health_number", s.health_number);
    v.boolean("distance", s.distance);
    v.boolean("weapon", s.weapon);
    v.boolean("scoped_indicator", s.scoped_indicator);
    v.boolean("snaplines", s.snaplines);
    v.choice("snapline_origin", s.snapline_origin, kSnaplineOrigins);
    v.boolean("visibility_colours", s.visibility_colours);
    v.section("colours", [&](auto& c) { visit_esp_colours(c, s.colours); });
}

template <class V, class S>
void visit_aimbot(V& v, S& s)
{
    v.boolean("enabled", s.enabled);
    v.choice("target", s.target, kAimTargets);
    v.choice("priority", s.priority, kAimPriorities);
    v.number("fov", s.fov, config::kAimFov);
    v.boolean("draw_fov", s.draw_fov);
    v.colour("fov_colour", s.fov_colour);
    v.number("smoothing", s.smoothing, config::kAimSmoothing);
    v.boolean("team_check", s.team_check);
    v.number("max_distance", s.max_distance, config::kMaxDistance);
    v.boolean("visible_only", s.visible_only);
}

template <class V, class S>
void visit_weapon_filter(V& v, S& s)
{
    v.boolean("pistol", s.pistol);
    v.boolean("smg", s.smg);
    v.boolean("rifle", s.rifle);
    v.boolean("sniper", s.sniper);
    v.boolean("shotgun", s.shotgun);
    v.boolean("heavy", s.heavy);
}

template <class V, class S>
void visit_triggerbot(V& v, S& s)
{
    v.boolean("enabled", s.enabled);
    v.choice("activation", s.activation, kTriggerActivations);
    v.number("reaction_ms", s.reaction_ms, config::kTriggerReaction);
    v.choice("fire_mode", s.fire_mode, kFireModes);
    v.number("burst_shots", s.burst_shots, config::kTriggerBurst);
    v.number("shot_delay_ms", s.shot_delay_ms, config::kTriggerShotDelay);
    v.boolean("team_check", s.team_check);
    v.boolean("visible_only", s.visible_only);
    v.number("max_distance", s.max_distance, config::kMaxDistance);
    v.section("weapons", [&](auto& w) { visit_weapon_filter(w, s.weapons); });
    v.boolean("snipers_scoped_only", s.snipers_scoped_only);
    v.boolean("not_flashed", s.not_flashed);
    v.boolean("not_in_air", s.not_in_air);
    v.boolean("head_only", s.head_only);
}

template <class V, class S>
void visit_radar_colours(V& v, S& s)
{
    v.colour("enemy_visible", s.enemy_visible);
    v.colour("enemy_hidden", s.enemy_hidden);
    v.colour("team", s.team);
    v.colour("you", s.you);
    v.colour("background", s.background);
}

template <class V, class S>
void visit_radar(V& v, S& s)
{
    v.boolean("enabled", s.enabled);
    v.choice("corner", s.corner, kRadarCorners);
    v.number("size", s.size, config::kRadarSize);
    v.number("range", s.range, config::kRadarRange);
    v.boolean("rotate", s.rotate);
    v.boolean("show_teammates", s.show_teammates);
    v.boolean("facing", s.facing);
    v.boolean("names", s.names);
    v.boolean("clamp_to_edge", s.clamp_to_edge);
    v.number("dot_size", s.dot_size, config::kRadarDotSize);
    v.boolean("visibility_colours", s.visibility_colours);
    v.section("colours", [&](auto& c) { visit_radar_colours(c, s.colours); });
}

template <class V, class S>
void visit_bomb_timer(V& v, S& s)
{
    v.boolean("enabled", s.enabled);
    v.number("top", s.top, config::kBombTimerTop);
    v.boolean("defuse_hint", s.defuse_hint);
    v.boolean("distance", s.distance);
}

template <class V, class S>
void visit_spectators(V& v, S& s)
{
    v.boolean("enabled", s.enabled);
    v.choice("side", s.side, kPanelSides);
    v.number("top", s.top, config::kSpectatorTop);
    v.boolean("show_mode", s.show_mode);
    v.boolean("hide_when_empty", s.hide_when_empty);
}

// Every section but the keybinds, by name: writes and reads go through the same list.
template <class V, class S>
void visit_sections(V& v, S& s)
{
    v.section("overlay", [&](auto& x) { visit_overlay(x, s.overlay); });
    v.section("general", [&](auto& x) { visit_general(x, s.general); });
    v.section("esp", [&](auto& x) { visit_esp(x, s.esp); });
    v.section("aimbot", [&](auto& x) { visit_aimbot(x, s.aimbot); });
    v.section("triggerbot", [&](auto& x) { visit_triggerbot(x, s.triggerbot); });
    v.section("radar", [&](auto& x) { visit_radar(x, s.radar); });
    v.section("bomb_timer", [&](auto& x) { visit_bomb_timer(x, s.bomb_timer); });
    v.section("spectators", [&](auto& x) { visit_spectators(x, s.spectators); });
}

// --- keybinds (keyed by action) ------------------------------------------------------------------------------------

Json write_keybinds(const KeybindSettings& keybinds)
{
    Json out = Json::object();
    for (const input::ActionDef& def : input::actions())
    {
        const input::Bind& bind = keybinds.bind(def.id);
        Json entry = Json::object();
        entry["key"] = bind.key == input::kUnbound ? Json(nullptr) : Json(input::key_name(bind.key));
        entry["mode"] = std::string(kBindModes[static_cast<std::size_t>(bind.mode)]);
        out[std::string(def.key)] = std::move(entry);
    }
    return out;
}

void read_bind(const Json& entry, const input::ActionDef& def, input::Bind& bind, Reader& section)
{
    const std::string key_name(def.key);
    if (!entry.is_object())
    {
        section.warn(key_name.c_str(), "expected an object, using default");
        return;
    }
    if (const auto key = entry.find("key"); key != entry.end())
    {
        std::optional<std::uint32_t> vk;
        if (key->is_null())
        {
            vk = input::kUnbound;
        }
        else if (key->is_string())
        {
            vk = input::vk_from_name(key->get<std::string>());
        }
        if (!vk)
        {
            section.warn(key_name.c_str(), "unknown key " + key->dump() + ", using default");
        }
        else if (!input::key_allowed(def.id, *vk))
        {
            section.warn(key_name.c_str(), "key " + key->dump() + " can't be used for this action, using default");
        }
        else
        {
            bind.key = *vk;
        }
    }
    if (const auto mode = entry.find("mode"); mode != entry.end())
    {
        std::optional<input::BindMode> parsed;
        for (std::size_t i = 0; i < kBindModes.size(); ++i)
        {
            if (mode->is_string() && mode->get<std::string>() == kBindModes[i])
            {
                parsed = static_cast<input::BindMode>(i);
            }
        }
        if (!parsed)
        {
            section.warn(key_name.c_str(), "bad mode " + mode->dump() + ", using default");
        }
        else if (!input::mode_allowed(def.id, *parsed))
        {
            section.warn(key_name.c_str(), "mode " + mode->dump() + " not allowed for this action, using default");
        }
        else
        {
            bind.mode = *parsed;
        }
    }
    for (auto it = entry.begin(); it != entry.end(); ++it)
    {
        if (it.key() != "key" && it.key() != "mode")
        {
            section.warn((key_name + "." + it.key()).c_str(), "unknown setting, ignored");
        }
    }
}

void read_keybinds(const Json* data, KeybindSettings& keybinds, std::vector<std::string>& warnings)
{
    Reader section(data, "keybinds", warnings);
    for (const input::ActionDef& def : input::actions())
    {
        const std::string key(def.key);
        if (const Json* entry = section.child(key.c_str()))
        {
            read_bind(*entry, def, keybinds.bind(def.id), section);
        }
    }
    section.finish();
}

// --- schema version -----------------------------------------------------------------------------------------------

// Upgrades from older schema versions go here (version N -> a function that rewrites the JSON to version N + 1),
// before the fields are read. None yet: schema 1 is the first.
void migrate(const Json& data, std::vector<std::string>& warnings)
{
    int version = 1;
    if (const auto field = data.find(kSchemaKey); field != data.end())
    {
        if (field->is_number_integer() && field->get<int>() >= 1)
        {
            version = field->get<int>();
        }
        else
        {
            warnings.push_back(std::string(kSchemaKey) + ": invalid (" + field->dump() + "), assuming 1");
        }
    }
    if (version > config::kProfileSchemaVersion)
    {
        warnings.push_back(std::format("profile is from a newer version ({}); loading what this version understands",
                                       version));
    }
}
} // namespace

std::string to_json_text(const Settings& settings)
{
    Json data = Json::object();
    data[kSchemaKey] = config::kProfileSchemaVersion;
    Writer writer(data);
    visit_sections(writer, settings);
    data["keybinds"] = write_keybinds(settings.keybinds);
    return data.dump(kJsonIndent) + "\n";
}

ParseResult from_json_text(std::string_view text)
{
    ParseResult result;
    try
    {
        const Json data = Json::parse(text.begin(), text.end(), nullptr, false); // no exceptions: discarded on error
        if (data.is_discarded())
        {
            result.error = "not valid JSON";
            return result;
        }
        result.ok = true;
        if (!data.is_object())
        {
            result.warnings.push_back("profile is not a JSON object, using defaults");
            return result;
        }
        migrate(data, result.warnings);
        Reader top(&data, "", result.warnings);
        top.child(kSchemaKey);
        visit_sections(top, result.settings);
        read_keybinds(top.child("keybinds"), result.settings.keybinds, result.warnings);
        top.finish();
    }
    catch (const std::exception& e)
    {
        result = ParseResult{};
        result.error = std::string("could not read the profile: ") + e.what();
    }
    return result;
}
} // namespace settings
