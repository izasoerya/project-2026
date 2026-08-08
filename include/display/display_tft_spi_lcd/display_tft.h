#if !defined(DISPLAY_TFT_H)
#define DISPLAY_TFT_H

#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include "consts/logo_bitmap.h"
#include "consts/icon_bitmap.h"

// ============================================================
// Grafana-inspired dark palette (RGB565)
// ============================================================
namespace DisplayColor
{
    constexpr uint16_t BG = 0x1082;       // #111217
    constexpr uint16_t PANEL = 0x18E4;    // #1B1D23
    constexpr uint16_t BORDER = 0x2986;   // #2C3235
    constexpr uint16_t TEXT = 0xDEDB;     // #D8D9DA
    constexpr uint16_t TEXT_DIM = 0x8C92; // #8E9297
    constexpr uint16_t ORANGE = 0xFBC1;   // #FF780A
    constexpr uint16_t GREEN = 0x75ED;    // #73BF69
    constexpr uint16_t BLUE = 0x54BE;     // #5794F2
    constexpr uint16_t RED = 0xF24B;      // #F2495C
    constexpr uint16_t YELLOW = 0xFEA5;   // #FADE2A
    constexpr uint16_t TEAL = 0x2EB7;     // #2DD4BF
}

enum class IconType : uint8_t
{
    NONE = 0,
    THERMO,
    DROPLET,
    WIND,
    COMPASS,
    RAIN,
    COMPANY // uses the large 40x40 logo instead of the 20x20 icon box
};

struct IconAsset
{
    const uint8_t *bitmap;
    uint8_t w;
    uint8_t h;
};

struct Container
{
    uint8_t row = 0;     // 0-2 (top to bottom)
    uint8_t column = 0;  // 0-1 (left to right)
    char label[24] = ""; // Max 24 character
    char value[24] = ""; // Max 24 character
    uint16_t color = DisplayColor::TEXT;
    IconType icon = IconType::NONE;
    bool populated = false;
};

class DisplayTFT320X240P
{
private:
    TFT_eSPI &tft;

    static constexpr uint16_t SCREEN_W = 320;
    static constexpr uint16_t SCREEN_H = 240;

    static constexpr uint16_t HEADER_H = 28;
    static constexpr uint16_t FOOTER_H = 26;
    static constexpr uint16_t COL_GAP = 6;
    static constexpr uint16_t ROW_GAP = 6;
    static constexpr uint16_t MARGIN_X = 8;

    static constexpr uint16_t COL1_X = MARGIN_X;
    static constexpr uint16_t PANEL_W = (SCREEN_W - (MARGIN_X * 2) - COL_GAP) / 2; // 148
    static constexpr uint16_t COL2_X = COL1_X + PANEL_W + COL_GAP;

    static constexpr uint16_t ROW1_Y = HEADER_H + 6;
    static constexpr uint16_t PANEL_H = (SCREEN_H - HEADER_H - FOOTER_H - 6 - (ROW_GAP * 2)) / 3; // 56
    static constexpr uint16_t ROW2_Y = ROW1_Y + PANEL_H + ROW_GAP;
    static constexpr uint16_t ROW3_Y = ROW2_Y + PANEL_H + ROW_GAP;

    static constexpr uint8_t ACCENT_W = 4;
    static constexpr uint8_t RADIUS = 6;
    static constexpr uint8_t ICON_BOX = ICON_BITMAP_DIM_W; // 20 - small icon bitmaps
    static constexpr uint8_t ICON_BOX_LG = 40;             // company logo bitmap

    static constexpr uint8_t NUM_CONTAINERS = 6;
    Container containers[NUM_CONTAINERS];

    char headerTitle[24] = "";
    char footerText[24] = "";
    char headerClock[9] = "";
    char signalStrength[12] = ""; // Either x dBm or No Internet

    int containerX(uint8_t column) const
    {
        switch (column)
        {
        case 0:
            return COL1_X;
        default:
            return COL2_X;
        }
    }

    int containerY(uint8_t row) const
    {
        switch (row)
        {
        case 0:
            return ROW1_Y;
        case 1:
            return ROW2_Y;
        default:
            return ROW3_Y;
        }
    }

    // Text starts further right when a panel uses the larger company logo
    int textOffsetX(const Container &c) const
    {
        return (c.icon == IconType::COMPANY) ? 52 : 36;
    }

    IconAsset iconAsset(IconType icon) const
    {
        switch (icon)
        {
        case IconType::THERMO:
            return {icon_thermo_bits, ICON_BITMAP_DIM_W, ICON_BITMAP_DIM_H};
        case IconType::DROPLET:
            return {icon_droplet_bits, ICON_BITMAP_DIM_W, ICON_BITMAP_DIM_H};
        case IconType::WIND:
            return {icon_wind_bits, ICON_BITMAP_DIM_W, ICON_BITMAP_DIM_H};
        case IconType::COMPASS:
            return {icon_compass_bits, ICON_BITMAP_DIM_W, ICON_BITMAP_DIM_H};
        case IconType::RAIN:
            return {icon_rain_bits, ICON_BITMAP_DIM_W, ICON_BITMAP_DIM_H};
        case IconType::COMPANY:
            return {icon_logo_t4t_40px, ICON_BOX_LG, ICON_BOX_LG};
        default:
            return {nullptr, 0, 0};
        }
    }

    void drawIcon(const Container &c, int x, int y) const
    {
        IconAsset asset = iconAsset(c.icon);
        if (asset.bitmap == nullptr)
            return;
        tft.drawBitmap(x, y, asset.bitmap, asset.w, asset.h, c.color);
    }

    void drawContainerShell(const Container &c) const
    {
        int x = containerX(c.column);
        int y = containerY(c.row);

        if (!c.populated)
        {
            tft.drawRoundRect(x, y, PANEL_W, PANEL_H, RADIUS, DisplayColor::BORDER);
            tft.setTextFont(2);
            tft.setTextSize(1);
            tft.setTextDatum(MC_DATUM);
            tft.setTextColor(DisplayColor::BORDER, DisplayColor::BG);
            tft.drawString("-- empty --", x + PANEL_W / 2, y + PANEL_H / 2);
            tft.setTextDatum(TL_DATUM);
            return;
        }

        tft.fillRoundRect(x, y, PANEL_W, PANEL_H, RADIUS, DisplayColor::PANEL);
        tft.drawRoundRect(x, y, PANEL_W, PANEL_H, RADIUS, DisplayColor::BORDER);
        tft.fillRect(x + 2, y + 4, ACCENT_W, PANEL_H - 8, c.color);

        if (c.icon == IconType::COMPANY)
            drawIcon(c, x + 6, y + (PANEL_H - ICON_BOX_LG) / 2);
        else if (c.icon != IconType::NONE)
            drawIcon(c, x + 10, y + (PANEL_H - ICON_BOX) / 2);

        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(DisplayColor::TEXT_DIM, DisplayColor::PANEL);
        tft.drawString(c.label, x + textOffsetX(c), y + 7);
    }

    void drawContainerValue(const Container &c) const
    {
        int x = containerX(c.column);
        int y = containerY(c.row);
        int tx = textOffsetX(c);

        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(DisplayColor::TEXT, DisplayColor::PANEL);
        tft.setTextPadding(PANEL_W - tx - 6); // flicker-free: pads over the old value
        tft.drawString(c.value, x + tx, y + 26);
    }

    void drawHeader() const
    {
        tft.fillRect(0, 0, SCREEN_W, HEADER_H, DisplayColor::PANEL);
        tft.drawFastHLine(0, HEADER_H, SCREEN_W, DisplayColor::BORDER);
        tft.drawBitmap(6, 6, icon_logo_zts_16px, 16, 16, DisplayColor::TEAL);

        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(DisplayColor::TEXT, DisplayColor::PANEL);
        tft.drawString(headerTitle, 30, 9);
    }

    void drawHeaderClock() const
    {
        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextDatum(TR_DATUM);
        tft.setTextColor(DisplayColor::TEXT_DIM, DisplayColor::PANEL);
        tft.setTextPadding(70);
        tft.drawString(headerClock, 310, 9);
    }

    void drawFooter() const
    {
        int y = SCREEN_H - FOOTER_H;
        tft.fillRect(0, y, SCREEN_W, FOOTER_H, DisplayColor::PANEL);
        tft.drawFastHLine(0, y, SCREEN_W, DisplayColor::BORDER);

        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(DisplayColor::TEXT_DIM, DisplayColor::PANEL);
        tft.drawString(footerText, 10, y + 6);
    }

    void drawFooterSignalStrength() const
    {
        int y = SCREEN_H - FOOTER_H;
        int textX = 230;
        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(DisplayColor::TEXT_DIM, DisplayColor::PANEL);
        tft.drawString(signalStrength, textX, y + 6);

        int textW = tft.textWidth(signalStrength); // measure actual width first
        int iconX = textX + textW + 12;            // circle placed just past the text
        int iconY = y + 6 + 8;
        tft.fillCircle(iconX, iconY, 4, DisplayColor::GREEN);
    }

    void setContainerFields(uint8_t index1based, const char *label, const char *value, uint16_t color, IconType icon)
    {
        if (index1based < 1 || index1based > NUM_CONTAINERS)
            return;
        Container &c = containers[index1based - 1];
        strncpy(c.label, label, sizeof(c.label) - 1);
        c.label[sizeof(c.label) - 1] = '\0';
        strncpy(c.value, value, sizeof(c.value) - 1);
        c.value[sizeof(c.value) - 1] = '\0';
        c.color = color;
        c.icon = icon;
        c.populated = true;
    }

public:
    explicit DisplayTFT320X240P(TFT_eSPI &tftRef) : tft(tftRef)
    {
        // Slot 1-3 = left column top-to-bottom, 4-6 = right column top-to-bottom
        for (uint8_t i = 0; i < NUM_CONTAINERS; i++)
        {
            containers[i].row = i % 3;
            containers[i].column = i / 3;
        }
    }

    void begin()
    {
        tft.init();
        tft.setRotation(1);
    }

    void setHeaderTitle(const char *title)
    {
        strncpy(headerTitle, title, sizeof(headerTitle) - 1);
        headerTitle[sizeof(headerTitle) - 1] = '\0';
    }

    void setFooterText(const char *text)
    {
        strncpy(footerText, text, sizeof(footerText) - 1);
        footerText[sizeof(footerText) - 1] = '\0';
    }

    void setClock(uint8_t h, uint8_t m, uint8_t s)
    {
        snprintf(headerClock, sizeof(headerClock), "%02u:%02u:%02u", h, m, s);
    }

    void setSignalStrength(int8_t signal)
    {
        snprintf(signalStrength, sizeof(signalStrength), "%d dBm", signal);
    }

    // ---------------------------------------------------
    // Container configuration. Slot numbering matches the reference
    // layout: 1-3 = left column (top-to-bottom), 4-6 = right column.
    //   display.setContainer1("TEMPERATURE", "24.5 C", DisplayColor::ORANGE, IconType::THERMO);
    // ---------------------------------------------------
    void setContainer1(const char *label, const char *value, uint16_t color, IconType icon) { setContainerFields(1, label, value, color, icon); }
    void setContainer2(const char *label, const char *value, uint16_t color, IconType icon) { setContainerFields(2, label, value, color, icon); }
    void setContainer3(const char *label, const char *value, uint16_t color, IconType icon) { setContainerFields(3, label, value, color, icon); }
    void setContainer4(const char *label, const char *value, uint16_t color, IconType icon) { setContainerFields(4, label, value, color, icon); }
    void setContainer5(const char *label, const char *value, uint16_t color, IconType icon) { setContainerFields(5, label, value, color, icon); }
    void setContainer6(const char *label, const char *value, uint16_t color, IconType icon) { setContainerFields(6, label, value, color, icon); }

    // Updates just the value text of a slot and repaints only that region
    // (flicker-free, same technique as the reference sketch's setTextPadding trick).
    void updateContainerValue(uint8_t index1based, const char *value)
    {
        if (index1based < 1 || index1based > NUM_CONTAINERS)
            return;
        Container &c = containers[index1based - 1];
        strncpy(c.value, value, sizeof(c.value) - 1);
        c.value[sizeof(c.value) - 1] = '\0';
        if (c.populated)
            drawContainerValue(c);
    }

    void clearContainer(uint8_t index1based)
    {
        if (index1based < 1 || index1based > NUM_CONTAINERS)
            return;
        Container &c = containers[index1based - 1];
        c.populated = false;
        c.label[0] = '\0';
        c.value[0] = '\0';
        c.icon = IconType::NONE;
    }

    // ---------------------------------------------------
    // Rendering
    // ---------------------------------------------------
    // Full repaint: background, header, all 6 shells (border/icon/label), footer.
    // Call once after configuring containers, or whenever a shell-level
    // property (label/icon/color/populated) changes.
    void drawLayout() const
    {
        tft.fillScreen(DisplayColor::BG);
        drawHeader();
        for (uint8_t i = 0; i < NUM_CONTAINERS; i++)
            drawContainerShell(containers[i]);
        drawFooter();
    }

    // Cheap periodic refresh: only repaints value text + clock, leaves
    // shells/borders alone. Safe to call every loop() tick.
    void refresh() const
    {
        for (uint8_t i = 0; i < NUM_CONTAINERS; i++)
            if (containers[i].populated)
                drawContainerValue(containers[i]);
        drawHeaderClock();
        drawFooterSignalStrength();
    }
};

class DisplayTFT480X320P
{
private:
    TFT_eSPI &tft;

    static constexpr uint16_t SCREEN_W = 480;
    static constexpr uint16_t SCREEN_H = 320;

    // Header/footer/margins scaled up from the 320x240 layout, then
    // re-derived so ROW3 + PANEL_H + FOOTER_H lands exactly on SCREEN_H.
    static constexpr uint16_t HEADER_H = 32;
    static constexpr uint16_t FOOTER_H = 32;
    static constexpr uint16_t COL_GAP = 8;
    static constexpr uint16_t ROW_GAP = 8;
    static constexpr uint16_t MARGIN_X = 10;
    static constexpr uint16_t TOP_GAP = 8; // gap between header and row 1

    static constexpr uint16_t COL1_X = MARGIN_X;
    static constexpr uint16_t PANEL_W = (SCREEN_W - (MARGIN_X * 2) - COL_GAP) / 2; // 226
    static constexpr uint16_t COL2_X = COL1_X + PANEL_W + COL_GAP;

    static constexpr uint16_t ROW1_Y = HEADER_H + TOP_GAP;
    static constexpr uint16_t PANEL_H = (SCREEN_H - HEADER_H - FOOTER_H - TOP_GAP - (ROW_GAP * 2)) / 3; // 77
    static constexpr uint16_t ROW2_Y = ROW1_Y + PANEL_H + ROW_GAP;
    static constexpr uint16_t ROW3_Y = ROW2_Y + PANEL_H + ROW_GAP;

    static constexpr uint8_t ACCENT_W = 5;
    static constexpr uint8_t RADIUS = 8;
    static constexpr uint8_t ICON_BOX = ICON_BITMAP_DIM_W; // 20 - small icon bitmaps (asset size is fixed, not screen-scaled)
    static constexpr uint8_t ICON_BOX_LG = 40;             // company logo bitmap

    static constexpr uint8_t NUM_CONTAINERS = 6;
    Container containers[NUM_CONTAINERS];

    char headerTitle[24] = "";
    char footerText[24] = "";
    char headerClock[9] = "";
    char signalStrength[12] = ""; // Either x dBm or No Internet

    int containerX(uint8_t column) const
    {
        switch (column)
        {
        case 0:
            return COL1_X;
        default:
            return COL2_X;
        }
    }

    int containerY(uint8_t row) const
    {
        switch (row)
        {
        case 0:
            return ROW1_Y;
        case 1:
            return ROW2_Y;
        default:
            return ROW3_Y;
        }
    }

    // Text starts further right when a panel uses the larger company logo.
    // Panels are wider here than on the 320x240 variant, so the icon gets
    // more breathing room before the label starts.
    int textOffsetX(const Container &c) const
    {
        return (c.icon == IconType::COMPANY) ? 60 : 42;
    }

    IconAsset iconAsset(IconType icon) const
    {
        switch (icon)
        {
        case IconType::THERMO:
            return {icon_thermo_bits, ICON_BITMAP_DIM_W, ICON_BITMAP_DIM_H};
        case IconType::DROPLET:
            return {icon_droplet_bits, ICON_BITMAP_DIM_W, ICON_BITMAP_DIM_H};
        case IconType::WIND:
            return {icon_wind_bits, ICON_BITMAP_DIM_W, ICON_BITMAP_DIM_H};
        case IconType::COMPASS:
            return {icon_compass_bits, ICON_BITMAP_DIM_W, ICON_BITMAP_DIM_H};
        case IconType::RAIN:
            return {icon_rain_bits, ICON_BITMAP_DIM_W, ICON_BITMAP_DIM_H};
        case IconType::COMPANY:
            return {icon_logo_t4t_40px, ICON_BOX_LG, ICON_BOX_LG};
        default:
            return {nullptr, 0, 0};
        }
    }

    void drawIcon(const Container &c, int x, int y) const
    {
        IconAsset asset = iconAsset(c.icon);
        if (asset.bitmap == nullptr)
            return;
        tft.drawBitmap(x, y, asset.bitmap, asset.w, asset.h, c.color);
    }

    void drawContainerShell(const Container &c) const
    {
        int x = containerX(c.column);
        int y = containerY(c.row);

        if (!c.populated)
        {
            tft.drawRoundRect(x, y, PANEL_W, PANEL_H, RADIUS, DisplayColor::BORDER);
            tft.setTextFont(2);
            tft.setTextSize(1);
            tft.setTextDatum(MC_DATUM);
            tft.setTextColor(DisplayColor::BORDER, DisplayColor::BG);
            tft.drawString("-- empty --", x + PANEL_W / 2, y + PANEL_H / 2);
            tft.setTextDatum(TL_DATUM);
            return;
        }

        tft.fillRoundRect(x, y, PANEL_W, PANEL_H, RADIUS, DisplayColor::PANEL);
        tft.drawRoundRect(x, y, PANEL_W, PANEL_H, RADIUS, DisplayColor::BORDER);
        tft.fillRect(x + 3, y + 5, ACCENT_W, PANEL_H - 10, c.color);

        if (c.icon == IconType::COMPANY)
            drawIcon(c, x + 10, y + (PANEL_H - ICON_BOX_LG) / 2);
        else if (c.icon != IconType::NONE)
            drawIcon(c, x + 14, y + (PANEL_H - ICON_BOX) / 2);

        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(DisplayColor::TEXT_DIM, DisplayColor::PANEL);
        tft.drawString(c.label, x + textOffsetX(c), y + 9);
    }

    void drawContainerValue(const Container &c) const
    {
        int x = containerX(c.column);
        int y = containerY(c.row);
        int tx = textOffsetX(c);

        tft.setTextFont(4); // one size step up from the 320x240 variant - larger panels can afford it
        tft.setTextSize(1);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(DisplayColor::TEXT, DisplayColor::PANEL);
        tft.setTextPadding(PANEL_W - tx - 8); // flicker-free: pads over the old value
        tft.drawString(c.value, x + tx, y + 32);
    }

    void drawHeader() const
    {
        tft.fillRect(0, 0, SCREEN_W, HEADER_H, DisplayColor::PANEL);
        tft.drawFastHLine(0, HEADER_H, SCREEN_W, DisplayColor::BORDER);
        tft.drawBitmap(8, 8, icon_logo_zts_16px, 16, 16, DisplayColor::TEAL);

        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(DisplayColor::TEXT, DisplayColor::PANEL);
        tft.drawString(headerTitle, 34, 11);
    }

    void drawHeaderClock() const
    {
        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextDatum(TR_DATUM);
        tft.setTextColor(DisplayColor::TEXT_DIM, DisplayColor::PANEL);
        tft.setTextPadding(80);
        tft.drawString(headerClock, 468, 11);
    }

    void drawFooter() const
    {
        int y = SCREEN_H - FOOTER_H;
        tft.fillRect(0, y, SCREEN_W, FOOTER_H, DisplayColor::PANEL);
        tft.drawFastHLine(0, y, SCREEN_W, DisplayColor::BORDER);

        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(DisplayColor::TEXT_DIM, DisplayColor::PANEL);
        tft.drawString(footerText, 12, y + 8);
    }

    void drawFooterSignalStrength() const
    {
        int y = SCREEN_H - FOOTER_H;
        int textX = 380;
        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(DisplayColor::TEXT_DIM, DisplayColor::PANEL);
        tft.drawString(signalStrength, textX, y + 8);

        int textW = tft.textWidth(signalStrength); // measure actual width first
        int iconX = textX + textW + 14;            // circle placed just past the text
        int iconY = y + 8 + 8;
        tft.fillCircle(iconX, iconY, 5, DisplayColor::GREEN);
    }

    void setContainerFields(uint8_t index1based, const char *label, const char *value, uint16_t color, IconType icon)
    {
        if (index1based < 1 || index1based > NUM_CONTAINERS)
            return;
        Container &c = containers[index1based - 1];
        strncpy(c.label, label, sizeof(c.label) - 1);
        c.label[sizeof(c.label) - 1] = '\0';
        strncpy(c.value, value, sizeof(c.value) - 1);
        c.value[sizeof(c.value) - 1] = '\0';
        c.color = color;
        c.icon = icon;
        c.populated = true;
    }

public:
    explicit DisplayTFT480X320P(TFT_eSPI &tftRef) : tft(tftRef)
    {
        // Slot 1-3 = left column top-to-bottom, 4-6 = right column top-to-bottom
        for (uint8_t i = 0; i < NUM_CONTAINERS; i++)
        {
            containers[i].row = i % 3;
            containers[i].column = i / 3;
        }
    }

    void begin()
    {
        tft.init();
        tft.setRotation(0);
    }

    void setHeaderTitle(const char *title)
    {
        strncpy(headerTitle, title, sizeof(headerTitle) - 1);
        headerTitle[sizeof(headerTitle) - 1] = '\0';
    }

    void setFooterText(const char *text)
    {
        strncpy(footerText, text, sizeof(footerText) - 1);
        footerText[sizeof(footerText) - 1] = '\0';
    }

    void setClock(uint8_t h, uint8_t m, uint8_t s)
    {
        snprintf(headerClock, sizeof(headerClock), "%02u:%02u:%02u", h, m, s);
    }

    void setSignalStrength(int8_t signal)
    {
        snprintf(signalStrength, sizeof(signalStrength), "%d dBm", signal);
    }

    // ---------------------------------------------------
    // Container configuration. Slot numbering matches the reference
    // layout: 1-3 = left column (top-to-bottom), 4-6 = right column.
    //   display.setContainer1("TEMPERATURE", "24.5 C", DisplayColor::ORANGE, IconType::THERMO);
    // ---------------------------------------------------
    void setContainer1(const char *label, const char *value, uint16_t color, IconType icon) { setContainerFields(1, label, value, color, icon); }
    void setContainer2(const char *label, const char *value, uint16_t color, IconType icon) { setContainerFields(2, label, value, color, icon); }
    void setContainer3(const char *label, const char *value, uint16_t color, IconType icon) { setContainerFields(3, label, value, color, icon); }
    void setContainer4(const char *label, const char *value, uint16_t color, IconType icon) { setContainerFields(4, label, value, color, icon); }
    void setContainer5(const char *label, const char *value, uint16_t color, IconType icon) { setContainerFields(5, label, value, color, icon); }
    void setContainer6(const char *label, const char *value, uint16_t color, IconType icon) { setContainerFields(6, label, value, color, icon); }

    // Updates just the value text of a slot and repaints only that region
    // (flicker-free, same technique as the reference sketch's setTextPadding trick).
    void updateContainerValue(uint8_t index1based, const char *value)
    {
        if (index1based < 1 || index1based > NUM_CONTAINERS)
            return;
        Container &c = containers[index1based - 1];
        strncpy(c.value, value, sizeof(c.value) - 1);
        c.value[sizeof(c.value) - 1] = '\0';
        if (c.populated)
            drawContainerValue(c);
    }

    void clearContainer(uint8_t index1based)
    {
        if (index1based < 1 || index1based > NUM_CONTAINERS)
            return;
        Container &c = containers[index1based - 1];
        c.populated = false;
        c.label[0] = '\0';
        c.value[0] = '\0';
        c.icon = IconType::NONE;
    }

    // ---------------------------------------------------
    // Rendering
    // ---------------------------------------------------
    // Full repaint: background, header, all 6 shells (border/icon/label), footer.
    // Call once after configuring containers, or whenever a shell-level
    // property (label/icon/color/populated) changes.
    void drawLayout() const
    {
        tft.fillScreen(DisplayColor::BG);
        drawHeader();
        for (uint8_t i = 0; i < NUM_CONTAINERS; i++)
            drawContainerShell(containers[i]);
        drawFooter();
    }

    // Cheap periodic refresh: only repaints value text + clock, leaves
    // shells/borders alone. Safe to call every loop() tick.
    void refresh() const
    {
        for (uint8_t i = 0; i < NUM_CONTAINERS; i++)
            if (containers[i].populated)
                drawContainerValue(containers[i]);
        drawHeaderClock();
        drawFooterSignalStrength();
    }
};

#include <TFT_eSPI.h>

class DisplayTFT320X480PV
{
private:
    TFT_eSPI &tft;

    static constexpr uint16_t SCREEN_W = 320;
    static constexpr uint16_t SCREEN_H = 480;

    static constexpr uint16_t HEADER_H = 32;
    static constexpr uint16_t FOOTER_H = 32;
    static constexpr uint16_t COL_GAP = 8;
    static constexpr uint16_t ROW_GAP = 8;
    static constexpr uint16_t MARGIN_X = 10;
    static constexpr uint16_t TOP_GAP = 8;

    static constexpr uint16_t COL1_X = MARGIN_X;
    static constexpr uint16_t PANEL_W = (SCREEN_W - (MARGIN_X * 2) - COL_GAP) / 2; // 146
    static constexpr uint16_t COL2_X = COL1_X + PANEL_W + COL_GAP;

    static constexpr uint16_t ROW1_Y = HEADER_H + TOP_GAP;
    static constexpr uint16_t PANEL_H = (SCREEN_H - HEADER_H - FOOTER_H - TOP_GAP - (ROW_GAP * 2)) / 3; // 130
    static constexpr uint16_t ROW2_Y = ROW1_Y + PANEL_H + ROW_GAP;
    static constexpr uint16_t ROW3_Y = ROW2_Y + PANEL_H + ROW_GAP;

    static constexpr uint8_t ACCENT_W = 5;
    static constexpr uint8_t RADIUS = 8;
    static constexpr uint8_t ICON_BOX = ICON_BITMAP_DIM_W; // 20
    static constexpr uint8_t ICON_BOX_LG = 40;             // 40

    static constexpr uint8_t NUM_CONTAINERS = 6;
    Container containers[NUM_CONTAINERS];

    char headerTitle[24] = "";
    char footerText[24] = "";
    char headerClock[9] = "";
    char signalStrength[12] = "";

    int containerX(uint8_t column) const
    {
        return (column == 0) ? COL1_X : COL2_X;
    }

    int containerY(uint8_t row) const
    {
        switch (row)
        {
        case 0:
            return ROW1_Y;
        case 1:
            return ROW2_Y;
        default:
            return ROW3_Y;
        }
    }

    IconAsset iconAsset(IconType icon) const
    {
        switch (icon)
        {
        case IconType::THERMO:
            return {icon_thermo_bits, ICON_BITMAP_DIM_W, ICON_BITMAP_DIM_H};
        case IconType::DROPLET:
            return {icon_droplet_bits, ICON_BITMAP_DIM_W, ICON_BITMAP_DIM_H};
        case IconType::WIND:
            return {icon_wind_bits, ICON_BITMAP_DIM_W, ICON_BITMAP_DIM_H};
        case IconType::COMPASS:
            return {icon_compass_bits, ICON_BITMAP_DIM_W, ICON_BITMAP_DIM_H};
        case IconType::RAIN:
            return {icon_rain_bits, ICON_BITMAP_DIM_W, ICON_BITMAP_DIM_H};
        default:
            return {nullptr, 0, 0};
        }
    }

    void drawContainerShell(const Container &c) const
    {
        int x = containerX(c.column);
        int y = containerY(c.row);

        if (!c.populated)
        {
            tft.drawRoundRect(x, y, PANEL_W, PANEL_H, RADIUS, DisplayColor::BORDER);
            tft.setTextFont(2);
            tft.setTextSize(1);
            tft.setTextDatum(MC_DATUM);
            tft.setTextColor(DisplayColor::BORDER, DisplayColor::BG);
            tft.drawString("-- empty --", x + PANEL_W / 2, y + PANEL_H / 2);
            tft.setTextDatum(TL_DATUM);
            return;
        }

        // Base Panel Background & Border
        tft.fillRoundRect(x, y, PANEL_W, PANEL_H, RADIUS, DisplayColor::PANEL);
        tft.drawRoundRect(x, y, PANEL_W, PANEL_H, RADIUS, DisplayColor::BORDER);
        tft.fillRect(x + 3, y + 5, ACCENT_W, PANEL_H - 10, c.color);

        // --- COMPANY PANEL LAYOUT (2 Rows: Tree4Trees & ZTS) ---
        if (c.icon == IconType::COMPANY)
        {
            tft.setTextFont(2);
            tft.setTextSize(1);
            tft.setTextDatum(TL_DATUM);
            tft.setTextColor(DisplayColor::TEXT_DIM, DisplayColor::PANEL);

            // Row 1: Tree4Trees
            tft.drawBitmap(x + 14, y + 14, icon_logo_t4t_40px, 40, 40, c.color);
            tft.drawString("Tree4Trees", x + 60, y + 26);

            // Row 2: ZTS (Using 16px icon, visually centered in the bottom half)
            tft.drawBitmap(x + 26, y + 84, icon_logo_zts_16px, 16, 16, DisplayColor::TEAL);
            tft.drawString("ZTS", x + 60, y + 84);
            return;
        }

        // --- STANDARD PANEL LAYOUT (Row 1: Header Label & Icon) ---
        if (c.icon != IconType::NONE)
        {
            IconAsset asset = iconAsset(c.icon);
            if (asset.bitmap != nullptr)
            {
                tft.drawBitmap(x + 14, y + 14, asset.bitmap, asset.w, asset.h, c.color);
            }
        }

        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(DisplayColor::TEXT_DIM, DisplayColor::PANEL);
        tft.drawString(c.label, x + 42, y + 16);
    }

    void drawContainerValue(const Container &c) const
    {
        // Company layout is static, no values to update
        if (c.icon == IconType::COMPANY)
            return;

        int x = containerX(c.column);
        int y = containerY(c.row);

        // Automatically split value and unit based on space character
        char valBuf[32] = {0};
        char unitBuf[32] = {0};

        const char *space = strchr(c.value, ' ');
        if (space != nullptr)
        {
            int len = space - c.value;
            if (len >= sizeof(valBuf))
                len = sizeof(valBuf) - 1;
            strncpy(valBuf, c.value, len);
            strncpy(unitBuf, space + 1, sizeof(unitBuf) - 1);
        }
        else
        {
            strncpy(valBuf, c.value, sizeof(valBuf) - 1);
        }

        // Row 2: Value
        tft.setTextFont(2);
        tft.setTextSize(2);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(DisplayColor::TEXT, DisplayColor::PANEL);
        tft.setTextPadding(PANEL_W - 42 - 10);
        tft.drawString(valBuf, x + 42, y + 55);

        // Row 3: Unit (Right-Aligned, Font 2)
        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextDatum(TR_DATUM);
        tft.setTextColor(DisplayColor::TEXT_DIM, DisplayColor::PANEL);
        tft.setTextPadding(50); // Pad out old unit text just in case
        tft.drawString(unitBuf, x + PANEL_W - 14, y + 100);
    }

    void drawHeader() const
    {
        tft.fillRect(0, 0, SCREEN_W, HEADER_H, DisplayColor::PANEL);
        tft.drawFastHLine(0, HEADER_H, SCREEN_W, DisplayColor::BORDER);
        tft.drawBitmap(8, 8, icon_logo_zts_16px, 16, 16, DisplayColor::TEAL);

        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(DisplayColor::TEXT, DisplayColor::PANEL);
        tft.drawString(headerTitle, 34, 11);
    }

    void drawHeaderClock() const
    {
        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextDatum(TR_DATUM);
        tft.setTextColor(DisplayColor::TEXT_DIM, DisplayColor::PANEL);
        tft.setTextPadding(80);
        tft.drawString(headerClock, SCREEN_W - 12, 11);
    }

    void drawFooter() const
    {
        int y = SCREEN_H - FOOTER_H;
        tft.fillRect(0, y, SCREEN_W, FOOTER_H, DisplayColor::PANEL);
        tft.drawFastHLine(0, y, SCREEN_W, DisplayColor::BORDER);

        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(DisplayColor::TEXT_DIM, DisplayColor::PANEL);
        tft.drawString(footerText, 12, y + 8);
    }

    void drawFooterSignalStrength() const
    {
        int y = SCREEN_H - FOOTER_H;
        int textX = 220;
        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(DisplayColor::TEXT_DIM, DisplayColor::PANEL);
        tft.drawString(signalStrength, textX, y + 8);

        int textW = tft.textWidth(signalStrength);
        int iconX = textX + textW + 14;
        int iconY = y + 8 + 8;
        tft.fillCircle(iconX, iconY, 5, DisplayColor::GREEN);
    }

    void setContainerFields(uint8_t index1based, const char *label, const char *value, uint16_t color, IconType icon)
    {
        if (index1based < 1 || index1based > NUM_CONTAINERS)
            return;
        Container &c = containers[index1based - 1];
        strncpy(c.label, label, sizeof(c.label) - 1);
        c.label[sizeof(c.label) - 1] = '\0';
        strncpy(c.value, value, sizeof(c.value) - 1);
        c.value[sizeof(c.value) - 1] = '\0';
        c.color = color;
        c.icon = icon;
        c.populated = true;
    }

public:
    explicit DisplayTFT320X480PV(TFT_eSPI &tftRef) : tft(tftRef)
    {
        for (uint8_t i = 0; i < NUM_CONTAINERS; i++)
        {
            containers[i].row = i % 3;
            containers[i].column = i / 3;
        }
    }

    void begin()
    {
        tft.init();
        tft.setRotation(2);
    }

    void setHeaderTitle(const char *title)
    {
        strncpy(headerTitle, title, sizeof(headerTitle) - 1);
        headerTitle[sizeof(headerTitle) - 1] = '\0';
    }

    void setFooterText(const char *text)
    {
        strncpy(footerText, text, sizeof(footerText) - 1);
        footerText[sizeof(footerText) - 1] = '\0';
    }

    void setClock(uint8_t h, uint8_t m, uint8_t s)
    {
        snprintf(headerClock, sizeof(headerClock), "%02u:%02u:%02u", h, m, s);
    }

    void setSignalStrength(int8_t signal)
    {
        snprintf(signalStrength, sizeof(signalStrength), "%d dBm", signal);
    }

    void setContainer1(const char *label, const char *value, uint16_t color, IconType icon) { setContainerFields(1, label, value, color, icon); }
    void setContainer2(const char *label, const char *value, uint16_t color, IconType icon) { setContainerFields(2, label, value, color, icon); }
    void setContainer3(const char *label, const char *value, uint16_t color, IconType icon) { setContainerFields(3, label, value, color, icon); }
    void setContainer4(const char *label, const char *value, uint16_t color, IconType icon) { setContainerFields(4, label, value, color, icon); }
    void setContainer5(const char *label, const char *value, uint16_t color, IconType icon) { setContainerFields(5, label, value, color, icon); }
    void setContainer6(const char *label, const char *value, uint16_t color, IconType icon) { setContainerFields(6, label, value, color, icon); }

    void updateContainerValue(uint8_t index1based, const char *value)
    {
        if (index1based < 1 || index1based > NUM_CONTAINERS)
            return;
        Container &c = containers[index1based - 1];
        strncpy(c.value, value, sizeof(c.value) - 1);
        c.value[sizeof(c.value) - 1] = '\0';
        if (c.populated)
            drawContainerValue(c);
    }

    void clearContainer(uint8_t index1based)
    {
        if (index1based < 1 || index1based > NUM_CONTAINERS)
            return;
        Container &c = containers[index1based - 1];
        c.populated = false;
        c.label[0] = '\0';
        c.value[0] = '\0';
        c.icon = IconType::NONE;
    }

    void drawLayout() const
    {
        tft.fillScreen(DisplayColor::BG);
        drawHeader();
        for (uint8_t i = 0; i < NUM_CONTAINERS; i++)
            drawContainerShell(containers[i]);
        drawFooter();
    }

    void refresh() const
    {
        for (uint8_t i = 0; i < NUM_CONTAINERS; i++)
            if (containers[i].populated)
                drawContainerValue(containers[i]);
        drawHeaderClock();
        drawFooterSignalStrength();
    }
};

#endif // DISPLAY_TFT_H