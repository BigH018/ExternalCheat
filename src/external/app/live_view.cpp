#include "app/live_view.h"

#include <chrono>
#include <cstdio>
#include <format>

#include "config.h"
#include "core/log.h"
#include "core/runtime.h"
#include "features/spectators.h"
#include "game/player.h"
#include "game/weapon.h"
#include "maths/vec.h"

namespace app
{
namespace
{
// Console escape sequences (virtual terminal mode): cursor home, clear to end of line, clear to end of screen.
constexpr char kCursorHome[] = "\x1b[H";
constexpr char kClearScreen[] = "\x1b[2J";
constexpr char kClearLineEnd[] = "\x1b[K";
constexpr char kClearScreenEnd[] = "\x1b[J";

std::string weapon_text(const game::PlayerSnapshot& player)
{
    if (!player.weapon_id)
    {
        return "-";
    }
    const game::WeaponInfo info = game::weapon_info(*player.weapon_id);
    return info.name.empty() ? std::format("#{}", *player.weapon_id) : std::string(info.name);
}

// A dead player's camera: "watching Kev (1st person)", "free camera", or "" (alive, death cam).
std::string observer_text(const game::PlayerSnapshot& player, const game::GameSnapshot& snapshot)
{
    if (player.alive || player.observer_mode == game::ObserverMode::none)
    {
        return "";
    }
    if (player.observer_mode == game::ObserverMode::roaming)
    {
        return "free camera ";
    }
    if (!game::watches_target(player.observer_mode))
    {
        return "";
    }
    std::string target = "?";
    for (const game::PlayerSnapshot& other : snapshot.players)
    {
        if (player.observer_target != 0 && other.pawn == player.observer_target)
        {
            target = other.is_local ? "YOU" : other.name;
        }
    }
    return std::format("watching {} ({}) ", target, features::observer_mode_name(player.observer_mode));
}

std::string player_row(const game::PlayerSnapshot& player, const game::GameSnapshot& snapshot)
{
    const game::PlayerSnapshot* local = snapshot.local();
    std::string state = "no pawn";
    if (player.pawn != 0)
    {
        state = player.alive ? "alive" : "dead";
    }
    std::string distance = "-";
    if (local != nullptr && local->pawn != 0 && player.pawn != 0 && !player.is_local)
    {
        distance = std::format("{:.1f} m", maths::units_to_metres(local->origin.distance_to(player.origin)));
    }
    std::string notes;
    if (player.is_local)
    {
        notes += "YOU ";
    }
    if (player.scoped)
    {
        notes += "scoped ";
    }
    if (player.dormant)
    {
        notes += "dormant ";
    }
    if (player.pawn != 0 && !player.on_ground())
    {
        notes += "in air ";
    }
    notes += observer_text(player, snapshot);
    return std::format("{:>3}  {:<4} {:<20.20} {:<7} {:>4} {:>4}  {:<14.14} {:>8.0f} {:>8.0f} {:>7.0f}  {:>8}  {}",
                       player.index, game::team_short_name(player.team), player.name, state, player.health,
                       player.armor, weapon_text(player), player.origin.x, player.origin.y, player.origin.z,
                       distance, notes);
}

bool enable_virtual_terminal() noexcept
{
    const HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    return out != INVALID_HANDLE_VALUE && GetConsoleMode(out, &mode) &&
           SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}
} // namespace

std::vector<std::string> format_live_view(const game::GameSnapshot& snapshot, double read_ms)
{
    std::vector<std::string> lines;
    lines.push_back(std::format("{} - live view (Ctrl+C to exit)", config::kAppName));
    if (!snapshot.in_match)
    {
        lines.push_back("Not in a match: join Practice with Bots (offline, -insecure).");
        return lines;
    }
    const game::GlobalVars& globals = snapshot.globals;
    lines.push_back(std::format("map {}   tick {}   curtime {:.2f}   interval {:.4f}   max clients {}",
                                globals.map_name.empty() ? "?" : globals.map_name, globals.tick_count,
                                globals.curtime, globals.interval_per_tick, globals.max_clients));
    if (snapshot.view)
    {
        const maths::ViewMatrix& m = *snapshot.view;
        lines.push_back(std::format("view matrix OK   w row [{:.3f} {:.3f} {:.3f} {:.1f}]", m.at(3, 0), m.at(3, 1),
                                    m.at(3, 2), m.at(3, 3)));
    }
    else
    {
        lines.push_back("view matrix not sane (all zero or non-finite): nothing would be projected");
    }

    int alive = 0;
    for (const game::PlayerSnapshot& player : snapshot.players)
    {
        alive += player.alive ? 1 : 0;
    }
    lines.push_back(
        std::format("{} players, {} alive   (read in {:.2f} ms)", snapshot.players.size(), alive, read_ms));
    lines.emplace_back();
    lines.push_back(std::format("{:>3}  {:<4} {:<20} {:<7} {:>4} {:>4}  {:<14} {:>8} {:>8} {:>7}  {:>8}  {}", "#",
                                "TEAM", "NAME", "STATE", "HP", "ARM", "WEAPON", "X", "Y", "Z", "DIST", "NOTES"));
    for (const game::PlayerSnapshot& player : snapshot.players)
    {
        lines.push_back(player_row(player, snapshot));
    }
    return lines;
}

int run_live_view(const core::Memory& memory, HANDLE process, const core::ModuleInfo& client)
{
    // In a console, redraw in place. Redirected to a file (no virtual terminal), print one frame after another.
    const bool in_place = enable_virtual_terminal();
    if (in_place)
    {
        std::fputs(kClearScreen, stdout);
    }
    while (!core::shutdown_requested.load())
    {
        if (!core::is_running(process))
        {
            if (in_place)
            {
                std::fputs(kClearScreen, stdout);
                std::fputs(kCursorHome, stdout);
            }
            logger::info("cs2.exe has closed: exiting");
            return 0;
        }
        const auto started = std::chrono::steady_clock::now();
        const game::GameSnapshot snapshot = game::read_game(memory, client.base);
        const std::chrono::duration<double, std::milli> took = std::chrono::steady_clock::now() - started;
        std::string frame = in_place ? kCursorHome : "";
        for (const std::string& line : format_live_view(snapshot, took.count()))
        {
            frame += line;
            frame += in_place ? kClearLineEnd : "";
            frame += '\n';
        }
        frame += in_place ? kClearScreenEnd : "\n";
        std::fputs(frame.c_str(), stdout);
        std::fflush(stdout);
        Sleep(config::kLiveViewIntervalMs);
    }
    std::fputs("\n", stdout);
    logger::info("Live view stopped");
    return 0;
}
} // namespace app
