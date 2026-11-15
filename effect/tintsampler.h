/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <effect/effect.h>

#include <QElapsedTimer>
#include <QHash>
#include <QRegularExpression>
#include <QTimer>

namespace KWin
{
class GLFramebuffer;
class GLTexture;

//* samples each window's contents and hands the color to its decoration
class TintSamplerEffect : public Effect
{
    Q_OBJECT

public:
    enum class Mode {
        //* average of the whole client area
        Average,
        //* average of a thin strip along the client area's edges
        Edges,
    };

    TintSamplerEffect();
    ~TintSamplerEffect() override;

    static bool supported();

    void reconfigure(ReconfigureFlags flags) override;
    bool paintScreen(const RenderTarget &renderTarget, const RenderViewport &viewport, int mask, const Region &deviceRegion, LogicalOutput *screen) override;

private:
    struct WindowState {
        //* contents changed since the last sample
        bool dirty = true;
        //* time of the last sample
        QElapsedTimer lastSample;
    };

    void slotWindowAdded(EffectWindow *w);
    void slotWindowDamaged(EffectWindow *w);
    void slotWindowDeleted(EffectWindow *w);

    //* whether the window is a candidate for sampling at all
    bool isSampleable(EffectWindow *w) const;

    //* render the window's client area into a small texture and average it. Invalid on failure
    QColor sample(EffectWindow *w);

    //* forward a color to the window's decoration, if it understands it
    void publish(EffectWindow *w, const QColor &color);

    //* make sure a repaint happens once the earliest deferred window is due
    void scheduleDeferred(qint64 delay);

    QHash<EffectWindow *, WindowState> m_windows;
    QTimer m_deferredTimer;

    std::unique_ptr<GLTexture> m_texture;
    std::unique_ptr<GLFramebuffer> m_framebuffer;

    int m_interval = 1000;
    Mode m_mode = Mode::Edges;
    QList<QRegularExpression> m_blocklist;
};

}
