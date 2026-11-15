/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "tintsampler.h"

namespace KWin
{
KWIN_EFFECT_FACTORY_SUPPORTED(TintSamplerEffect, "metadata.json", return TintSamplerEffect::supported();)
}

#include "main.moc"
