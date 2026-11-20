/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "colorresolver.h"

#include <KColorScheme>
#include <KColorUtils>
#include <KSharedConfig>

#include <QHash>
#include <QImage>

#include <array>
#include <cmath>

namespace BreezeTinted
{
namespace
{
//* icon size used for analysis, in device independent pixels
constexpr int IconSampleSize = 32;

//* number of hue buckets for the icon histogram
constexpr int HueBins = 36;

//* minimum accumulated weight before an icon counts as "colored"
constexpr qreal MinIconWeight = 6.0;

//* minimum RGB distance (0-255 per channel) for a palette color to count as different from the global one
constexpr qreal MinPaletteDistance = 12.0;

struct GlobalColors {
    bool loaded = false;
    QColor window;
    QColor highlight;
};

GlobalColors &globalColors()
{
    static GlobalColors colors;
    if (!colors.loaded) {
        const auto config = KSharedConfig::openConfig(QStringLiteral("kdeglobals"));
        colors.window = KColorScheme(QPalette::Active, KColorScheme::Window, config).background().color();
        colors.highlight = KColorScheme(QPalette::Active, KColorScheme::Selection, config).background().color();
        colors.loaded = true;
    }
    return colors;
}

QHash<QString, QColor> &iconCache()
{
    static QHash<QString, QColor> cache;
    return cache;
}

qreal distance(const QColor &a, const QColor &b)
{
    const qreal dr = a.red() - b.red();
    const qreal dg = a.green() - b.green();
    const qreal db = a.blue() - b.blue();
    return std::sqrt(dr * dr + dg * dg + db * db);
}

QColor analyzeIcon(const QIcon &icon)
{
    if (icon.isNull()) {
        return QColor();
    }

    const QImage image = icon.pixmap(QSize(IconSampleSize, IconSampleSize)).toImage().convertToFormat(QImage::Format_ARGB32);
    if (image.isNull()) {
        return QColor();
    }

    // saturation and value weighted hue histogram, with the summed rgb per bucket
    struct Bin {
        qreal weight = 0;
        qreal r = 0;
        qreal g = 0;
        qreal b = 0;
    };
    std::array<Bin, HueBins> bins{};

    for (int y = 0; y < image.height(); ++y) {
        const auto *line = reinterpret_cast<const QRgb *>(image.constScanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            const QRgb pixel = line[x];
            const int alpha = qAlpha(pixel);
            if (alpha < 128) {
                continue;
            }

            const QColor color = QColor::fromRgb(pixel);
            const qreal saturation = color.hsvSaturationF();
            const qreal value = color.valueF();
            if (saturation < 0.2 || value < 0.2) {
                continue;
            }

            const qreal weight = saturation * value * (alpha / 255.0);
            const int hue = std::max(0, color.hsvHue());
            Bin &bin = bins[(hue * HueBins / 360) % HueBins];
            bin.weight += weight;
            bin.r += color.redF() * weight;
            bin.g += color.greenF() * weight;
            bin.b += color.blueF() * weight;
        }
    }

    // pick the strongest bucket together with its neighbours, so a hue on a bucket edge is not split
    int best = -1;
    qreal bestWeight = 0;
    for (int i = 0; i < HueBins; ++i) {
        const qreal weight = bins[(i + HueBins - 1) % HueBins].weight + bins[i].weight + bins[(i + 1) % HueBins].weight;
        if (weight > bestWeight) {
            bestWeight = weight;
            best = i;
        }
    }

    if (best < 0 || bestWeight < MinIconWeight) {
        return QColor();
    }

    qreal r = 0;
    qreal g = 0;
    qreal b = 0;
    for (int offset = -1; offset <= 1; ++offset) {
        const Bin &bin = bins[(best + offset + HueBins) % HueBins];
        r += bin.r;
        g += bin.g;
        b += bin.b;
    }

    return QColor::fromRgbF(r / bestWeight, g / bestWeight, b / bestWeight);
}
}

namespace ColorResolver
{
//________________________________________________________________
QColor iconColor(const QString &windowClass, const QIcon &icon)
{
    auto &cache = iconCache();
    const auto it = cache.constFind(windowClass);
    if (it != cache.constEnd()) {
        return *it;
    }

    const QColor color = analyzeIcon(icon);
    if (!windowClass.isEmpty()) {
        cache.insert(windowClass, color);
    }
    return color;
}

//________________________________________________________________
void invalidateIconColor(const QString &windowClass)
{
    iconCache().remove(windowClass);
}

//________________________________________________________________
QColor paletteColor(const QPalette &windowPalette)
{
    const auto &global = globalColors();

    // an app-specific highlight is the most characteristic color a scheme has
    const QColor highlight = windowPalette.color(QPalette::Active, QPalette::Highlight);
    if (highlight.isValid() && distance(highlight, global.highlight) > MinPaletteDistance) {
        return highlight;
    }

    // otherwise fall back to the window background, when that differs
    const QColor window = windowPalette.color(QPalette::Active, QPalette::Window);
    if (window.isValid() && distance(window, global.window) > MinPaletteDistance) {
        return window;
    }

    return QColor();
}

//________________________________________________________________
QColor accentColor()
{
    return globalColors().highlight;
}

//________________________________________________________________
void reloadGlobalColors()
{
    KSharedConfig::openConfig(QStringLiteral("kdeglobals"))->reparseConfiguration();
    globalColors().loaded = false;
}

//________________________________________________________________
QColor shape(const QColor &color, const InternalSettings &settings)
{
    if (!color.isValid()) {
        return color;
    }

    const qreal minLightness = std::min(settings.tintMinLightness(), settings.tintMaxLightness()) / 100.0;
    const qreal maxLightness = std::max(settings.tintMinLightness(), settings.tintMaxLightness()) / 100.0;

    const QColor hsl = color.toHsl();
    const qreal hue = std::max<qreal>(0, hsl.hslHueF());
    const qreal saturation = std::clamp(hsl.hslSaturationF() * settings.tintSaturation() / 100.0, 0.0, 1.0);
    const qreal lightness = std::clamp<qreal>(hsl.lightnessF(), minLightness, maxLightness);

    QColor shaped = QColor::fromHslF(hue, saturation, lightness).toRgb();
    if (settings.tintAccentMix() > 0) {
        shaped = KColorUtils::mix(shaped, accentColor(), settings.tintAccentMix() / 100.0);
    }
    return shaped;
}
}
}
