// settings/themes: every theme stays readable (text contrast), the defaults use the first theme's accent, and text on
// an accent fill picks the colour that stands out.

#include <doctest.h>

#include "color.h"
#include "settings/settings.h"
#include "settings/themes.h"

using settings::MenuTheme;

TEST_CASE("themes: contrast ratio matches the WCAG reference values")
{
    CHECK(settings::contrast_ratio(Color::rgb(0x000000), Color::rgb(0xFFFFFF)) == doctest::Approx(21.0f).epsilon(0.01));
    CHECK(settings::contrast_ratio(Color::rgb(0xFFFFFF), Color::rgb(0x000000)) == doctest::Approx(21.0f).epsilon(0.01));
    CHECK(settings::contrast_ratio(Color::rgb(0x777777), Color::rgb(0x777777)) == doctest::Approx(1.0f));
    // #767676 on white is the classic "just passes 4.5:1" grey.
    CHECK(settings::contrast_ratio(Color::rgb(0x767676), Color::rgb(0xFFFFFF)) == doctest::Approx(4.54f).epsilon(0.01));
}

TEST_CASE("themes: every theme is dark and its text is readable")
{
    for (const MenuTheme theme : settings::kMenuThemes)
    {
        CAPTURE(settings::theme_name(theme));
        const settings::ThemeColours& t = settings::theme_colours(theme);
        // Dark: the page and the panels are near black.
        CHECK(settings::contrast_ratio(t.window, Color::rgb(0x000000)) < 1.5f);
        CHECK(settings::contrast_ratio(t.panel, Color::rgb(0x000000)) < 1.6f);
        // Text: main text comfortably readable (but not glaring white), dimmed text still AA, faint text visible.
        CHECK(settings::contrast_ratio(t.text, t.panel) >= 10.0f);
        CHECK(settings::contrast_ratio(t.text, t.panel) < 18.0f);
        CHECK(settings::contrast_ratio(t.text_dim, t.panel) >= 4.5f);
        CHECK(settings::contrast_ratio(t.text_faint, t.panel) >= 2.5f);
        CHECK(settings::contrast_ratio(t.text, t.control) >= 9.0f);
        // The accent and the status colours stand out from a panel.
        CHECK(settings::contrast_ratio(t.accent, t.panel) >= 4.5f);
        CHECK(settings::contrast_ratio(t.ok, t.panel) >= 4.5f);
        CHECK(settings::contrast_ratio(t.warn, t.panel) >= 4.5f);
        CHECK(settings::contrast_ratio(t.danger, t.panel) >= 4.5f);
        // Text on an accent fill is readable.
        CHECK(settings::contrast_ratio(settings::text_on(t.accent, t), t.accent) >= 4.5f);
        // Opaque, byte-exact (a theme's accent becomes a saved setting).
        CHECK(t.accent.a == 1.0f);
        CHECK(Color::rgba(t.accent.to_rgba()) == t.accent);
    }
}

TEST_CASE("themes: names, and the default look")
{
    CHECK(settings::theme_name(MenuTheme::midnight) == "Midnight");
    CHECK(settings::theme_name(MenuTheme::ice) == "Ice");
    const settings::OverlaySettings defaults;
    CHECK(defaults.theme == MenuTheme::midnight);
    CHECK(defaults.accent == settings::theme_colours(MenuTheme::midnight).accent);
    // Out of range falls back to the first theme instead of reading past the table.
    CHECK(&settings::theme_colours(static_cast<MenuTheme>(200)) == &settings::theme_colours(MenuTheme::midnight));
}

TEST_CASE("themes: text on a fill picks the colour that stands out")
{
    const settings::ThemeColours& t = settings::theme_colours(MenuTheme::midnight);
    CHECK(settings::text_on(Color::rgb(0xF0F0F0), t) == t.window); // light accent: dark text
    CHECK(settings::text_on(Color::rgb(0x202060), t) == t.text);   // dark accent: light text
}
