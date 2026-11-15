/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "tintedsettings.h"

#include <QColor>
#include <QIcon>
#include <QPalette>
#include <QString>

namespace Tinted
{
//* turns per-window information into a single tint color
namespace ColorResolver
{
//* dominant color of an icon, cached per window class. Invalid if the icon is (near) monochrome
QColor iconColor(const QString &windowClass, const QIcon &icon);

//* drop the cached icon color for a window class
void invalidateIconColor(const QString &windowClass);

//* color of the window's own color scheme, or invalid if it uses the global scheme
QColor paletteColor(const QPalette &windowPalette);

//* global accent (selection background of the global color scheme)
QColor accentColor();

//* re-read kdeglobals after a global color scheme or accent change
void reloadGlobalColors();

//* apply saturation, lightness clamp and accent mix from the settings
QColor shape(const QColor &color, const InternalSettings &settings);
}
}
