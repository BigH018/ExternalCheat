#include "settings/presets.h"

namespace settings
{
namespace
{
// Which ESP elements a preset shows. Colours, thickness, team options and max distance are left alone.
struct EspLevel
{
    bool box;
    BoxStyle box_style;
    bool head_circle;
    bool skeleton;
    bool weapon;
    bool scoped_indicator;
    bool health_number;
    bool snaplines;
};

void set_esp(EspSettings& esp, const EspLevel& level)
{
    esp.enabled = true;
    esp.box = level.box;
    esp.box_style = level.box_style;
    esp.outline = true;
    esp.head_circle = level.head_circle;
    esp.skeleton = level.skeleton;
    esp.name = true;
    esp.health_bar = true;
    esp.health_number = level.health_number;
    esp.distance = true;
    esp.weapon = level.weapon;
    esp.scoped_indicator = level.scoped_indicator;
    esp.snaplines = level.snaplines;
}

void set_aimbot(AimbotSettings& aim, AimTarget target, float fov, float smoothing, bool visible_only)
{
    aim.enabled = true;
    aim.target = target;
    aim.priority = AimPriority::crosshair;
    aim.fov = fov;
    aim.smoothing = smoothing;
    aim.visible_only = visible_only;
}

void set_misc(Settings& settings, bool enabled)
{
    settings.radar.enabled = enabled;
    settings.bomb_timer.enabled = enabled;
    settings.spectators.enabled = enabled;
}

void apply_off(Settings& settings)
{
    settings.esp.enabled = false;
    settings.aimbot.enabled = false;
    settings.triggerbot.enabled = false;
    set_misc(settings, false);
}

void apply_chill(Settings& settings)
{
    set_esp(settings.esp, EspLevel{.box = true,
                                   .box_style = BoxStyle::corners,
                                   .head_circle = false,
                                   .skeleton = false,
                                   .weapon = false,
                                   .scoped_indicator = false,
                                   .health_number = false,
                                   .snaplines = false});
    set_aimbot(settings.aimbot, AimTarget::body, 3.0f, 12.0f, true);
    settings.triggerbot.enabled = false;
    set_misc(settings, true);
}

void apply_medium(Settings& settings)
{
    set_esp(settings.esp, EspLevel{.box = true,
                                   .box_style = BoxStyle::full,
                                   .head_circle = true,
                                   .skeleton = true,
                                   .weapon = true,
                                   .scoped_indicator = true,
                                   .health_number = false,
                                   .snaplines = false});
    set_aimbot(settings.aimbot, AimTarget::head, 6.0f, 6.0f, true);

    TriggerbotSettings& trigger = settings.triggerbot;
    trigger.enabled = true;
    trigger.activation = TriggerActivation::key;
    trigger.reaction_ms = 80;
    trigger.fire_mode = FireMode::single;
    trigger.visible_only = true;
    trigger.snipers_scoped_only = true;
    trigger.not_flashed = true;
    trigger.not_in_air = true;
    set_misc(settings, true);
}

void apply_rage(Settings& settings)
{
    set_esp(settings.esp, EspLevel{.box = true,
                                   .box_style = BoxStyle::full,
                                   .head_circle = true,
                                   .skeleton = true,
                                   .weapon = true,
                                   .scoped_indicator = true,
                                   .health_number = true,
                                   .snaplines = true});
    set_aimbot(settings.aimbot, AimTarget::head, 30.0f, 1.0f, false);

    TriggerbotSettings& trigger = settings.triggerbot;
    trigger.enabled = true;
    trigger.activation = TriggerActivation::always;
    trigger.reaction_ms = 0;
    trigger.fire_mode = FireMode::hold;
    trigger.visible_only = false;
    trigger.weapons = WeaponFilter{};
    trigger.snipers_scoped_only = false;
    trigger.not_flashed = false;
    trigger.not_in_air = false;
    trigger.head_only = false;
    set_misc(settings, true);
}
} // namespace

std::string_view preset_name(Preset preset) noexcept
{
    switch (preset)
    {
    case Preset::off: return "Off";
    case Preset::chill: return "Chill";
    case Preset::medium: return "Medium";
    case Preset::rage: return "Rage";
    }
    return "";
}

std::string_view preset_description(Preset preset) noexcept
{
    switch (preset)
    {
    case Preset::off: return "Every feature off.";
    case Preset::chill: return "Corner box ESP, a slow narrow body aimbot (visible bots only), no triggerbot.";
    case Preset::medium: return "Full ESP with skeletons, a faster head aimbot, triggerbot on the trigger key.";
    case Preset::rage: return "Everything: snap aimbot over 30 deg through walls, triggerbot always on, snaplines.";
    }
    return "";
}

void apply_preset(Settings& settings, Preset preset) noexcept
{
    switch (preset)
    {
    case Preset::off: apply_off(settings); return;
    case Preset::chill: apply_chill(settings); return;
    case Preset::medium: apply_medium(settings); return;
    case Preset::rage: apply_rage(settings); return;
    }
}
} // namespace settings
