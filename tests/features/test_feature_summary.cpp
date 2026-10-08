#include <string_view>
#include <vector>

#include <doctest.h>

#include "features/feature_summary.h"

using Names = std::vector<std::string_view>;

TEST_CASE("active_feature_names: nothing on gives no names")
{
    CHECK(features::active_feature_names({}).empty());
    CHECK(features::feature_summary({}).empty());
}

TEST_CASE("active_feature_names: a single feature is just its name")
{
    features::ActiveFeatures active;
    active.esp = true;
    CHECK(features::active_feature_names(active) == Names{"ESP"});
    CHECK(features::feature_summary(active) == "ESP");

    active = {};
    active.spectator_list = true;
    CHECK(features::active_feature_names(active) == Names{"Spectators"});
}

TEST_CASE("active_feature_names: several features keep the fixed order, whatever was switched on first")
{
    features::ActiveFeatures active;
    active.triggerbot = true;
    active.esp = true;
    CHECK(features::active_feature_names(active) == Names{"ESP", "Triggerbot"});
    CHECK(features::feature_summary(active) == "ESP · Triggerbot");
}

TEST_CASE("active_feature_names: everything on")
{
    const features::ActiveFeatures all{true, true, true, true, true, true};
    CHECK(features::active_feature_names(all) ==
          Names{"ESP", "Aimbot", "Triggerbot", "Radar", "Bomb timer", "Spectators"});
    CHECK(features::feature_summary(all) == "ESP · Aimbot · Triggerbot · Radar · Bomb timer · Spectators");
}
