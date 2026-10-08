#include "ui/hud.h"

#include <algorithm>
#include <cfloat>
#include <format>
#include <string>
#include <string_view>
#include <vector>

#include <imgui.h>

#include "config.h"
#include "features/feature_summary.h"

namespace ui
{
namespace
{
// Sizes at UI scale 1.0.
constexpr float kWatermarkPaddingX = 8.0f;
constexpr float kWatermarkPaddingY = 5.0f;
constexpr float kWatermarkRounding = 8.0f;
constexpr float kWatermarkLogoInset = 3.0f; // gap between the logo and the watermark's edge
constexpr float kWatermarkGap = 6.0f;       // between the logo and the text
constexpr float kFeatureLineGap = 2.0f;     // between feature lines
constexpr float kFeatureDotRadius = 2.5f;   // the accent dot before each feature
constexpr float kFeatureDotGap = 7.0f;      // from the dot's centre to the feature name
constexpr float kWatermarkAlpha = 0.88f;    // the background, over the game
constexpr char kNoFeatures[] = "No features on";

ImVec2 text_size(ImFont* font, float size, const char* text)
{
    return font->CalcTextSizeA(size, FLT_MAX, 0.0f, text);
}

// [logo] External Cheat by BigH
//        • ESP
//        • Aimbot
void draw_watermark(const Fonts& fonts, ImTextureData* logo, ImDrawList* draw, const app::AppState& app)
{
    const float font_size = ImGui::GetFontSize();
    const std::string author = std::format(" {}", config::kAppAuthor);
    const ImVec2 name_size = text_size(fonts.bold, font_size, config::kAppName);
    const ImVec2 author_size = text_size(fonts.regular, font_size, author.c_str());
    const float title_height = std::max(name_size.y, author_size.y);

    // One line per active feature (std::string so each is null-terminated for ImGui), or one "none" line.
    std::vector<std::string> lines;
    for (const std::string_view name : features::active_feature_names(app.active))
    {
        lines.emplace_back(name);
    }
    const bool none = lines.empty();
    if (none)
    {
        lines.emplace_back(kNoFeatures);
    }
    const float bullet = none ? 0.0f : scaled(kFeatureDotGap) + scaled(kFeatureDotRadius);
    float lines_width = 0.0f;
    float line_height = 0.0f;
    for (const std::string& line : lines)
    {
        const ImVec2 size = text_size(fonts.regular, font_size, line.c_str());
        lines_width = std::max(lines_width, bullet + size.x);
        line_height = std::max(line_height, size.y);
    }
    const float line_gap = scaled(kFeatureLineGap);

    // The logo fills the title row's height; the text column starts to its right.
    const ImVec2 padding(scaled(kWatermarkPaddingX), scaled(kWatermarkPaddingY));
    const float inset = scaled(kWatermarkLogoInset);
    const float logo_size = logo != nullptr ? title_height + 2.0f * padding.y - 2.0f * inset : 0.0f;
    const float text_x = logo != nullptr ? inset + logo_size + scaled(kWatermarkGap) : padding.x;
    const float text_width = std::max(name_size.x + author_size.x, lines_width);
    const float height = 2.0f * padding.y + title_height +
                         static_cast<float>(lines.size()) * (line_gap + line_height);

    const Palette& p = palette();
    const ImVec2 min(scaled(config::kWatermarkMargin), scaled(config::kWatermarkMargin));
    const ImVec2 max(min.x + text_x + text_width + padding.x, min.y + height);
    draw->AddRectFilled(min, max, u32(with_alpha(p.chrome, kWatermarkAlpha)), scaled(kWatermarkRounding));
    draw->AddRect(min, max, u32(p.border), scaled(kWatermarkRounding));
    if (logo != nullptr)
    {
        const ImVec2 logo_min(min.x + inset, min.y + inset);
        draw->AddImageRounded(logo->GetTexRef(), logo_min, ImVec2(logo_min.x + logo_size, logo_min.y + logo_size),
                              ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), IM_COL32_WHITE,
                              scaled(kWatermarkRounding) - inset * 0.5f);
    }

    // Title row.
    const float x = min.x + text_x;
    float y = min.y + padding.y;
    draw->AddText(fonts.bold, font_size, ImVec2(x, y), u32(p.text), config::kAppName);
    draw->AddText(fonts.regular, font_size, ImVec2(x + name_size.x, y), u32(p.text_dim), author.c_str());
    y += title_height;

    // Feature lines.
    const ImU32 line_colour = u32(none ? p.text_faint : p.text);
    for (const std::string& line : lines)
    {
        y += line_gap;
        if (!none)
        {
            draw->AddCircleFilled(ImVec2(x + scaled(kFeatureDotRadius), y + line_height * 0.5f),
                                  scaled(kFeatureDotRadius), u32(p.accent));
        }
        draw->AddText(fonts.regular, font_size, ImVec2(x + bullet, y), line_colour, line.c_str());
        y += line_height;
    }
}

void draw_frame_outline(ImDrawList* draw)
{
    // Inset by half the thickness so the whole line is inside the overlay: if it covers the game exactly, the outline
    // sits right on the edges of the game's picture.
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const float half = config::kFrameOutlineThickness * 0.5f;
    draw->AddRect(ImVec2(half, half), ImVec2(display.x - half, display.y - half), u32(palette().accent),
                  0.0f, config::kFrameOutlineThickness);
}
} // namespace

void draw_hud(const Fonts& fonts, ImTextureData* logo, const app::AppState& app)
{
    ImDrawList* draw = ImGui::GetBackgroundDrawList();
    if (app.settings.overlay.frame_outline)
    {
        draw_frame_outline(draw);
    }
    if (app.settings.overlay.watermark)
    {
        draw_watermark(fonts, logo, draw, app);
    }
}
} // namespace ui
