/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "tintsampler.h"

#include <core/region.h>
#include <core/rendertarget.h>
#include <core/renderviewport.h>
#include <effect/effecthandler.h>
#include <opengl/glutils.h>

#include <KConfigGroup>
#include <KDecoration3/Decoration>
#include <KSharedConfig>

#include <QColor>
#include <QLoggingCategory>
#include <QMetaMethod>

Q_LOGGING_CATEGORY(KWIN_TINTSAMPLER, "kwin_effect_tintsampler", QtWarningMsg)

namespace KWin
{
namespace
{
//* longest side of the offscreen sample, in pixels
constexpr int SampleSize = 32;

//* upper bound of windows sampled in a single frame, to avoid frame time spikes
constexpr int MaxSamplesPerFrame = 4;

//* pixels with less alpha than this don't contribute
constexpr int MinAlpha = 10;

bool hasSampledColorSlot(const QObject *object)
{
    static const QByteArray signature = QMetaObject::normalizedSignature("setSampledColor(QColor)");
    return object && object->metaObject()->indexOfMethod(signature.constData()) >= 0;
}
}

//________________________________________________________________
TintSamplerEffect::TintSamplerEffect()
{
    reconfigure(ReconfigureAll);

    m_deferredTimer.setSingleShot(true);
    connect(&m_deferredTimer, &QTimer::timeout, this, [this]() {
        // a repaint brings us back into paintScreen(), where sampling happens
        for (auto it = m_windows.cbegin(); it != m_windows.cend(); ++it) {
            if (it->dirty && isSampleable(it.key())) {
                it.key()->addRepaintFull();
            }
        }
    });

    connect(effects, &EffectsHandler::windowAdded, this, &TintSamplerEffect::slotWindowAdded);
    connect(effects, &EffectsHandler::windowDeleted, this, &TintSamplerEffect::slotWindowDeleted);

    const auto windows = effects->stackingOrder();
    for (EffectWindow *w : windows) {
        slotWindowAdded(w);
    }
    effects->addRepaintFull();
}

//________________________________________________________________
TintSamplerEffect::~TintSamplerEffect()
{
    // hand the decorations back to their other color sources
    for (auto it = m_windows.cbegin(); it != m_windows.cend(); ++it) {
        publish(it.key(), QColor());
    }

    if (m_texture || m_framebuffer) {
        effects->makeOpenGLContextCurrent();
        m_framebuffer.reset();
        m_texture.reset();
    }
}

//________________________________________________________________
bool TintSamplerEffect::supported()
{
    return effects->isOpenGLCompositing();
}

//________________________________________________________________
void TintSamplerEffect::reconfigure(ReconfigureFlags)
{
    const KConfigGroup group(KSharedConfig::openConfig(QStringLiteral("kwinrc")), QStringLiteral("Effect-tintsampler"));

    m_interval = std::max(100, group.readEntry("Interval", 1000));
    m_mode = group.readEntry("Mode", QStringLiteral("Edges")).compare(QLatin1String("Average"), Qt::CaseInsensitive) == 0 ? Mode::Average : Mode::Edges;

    m_blocklist.clear();
    const QStringList patterns = group.readEntry("Blocklist", QStringList());
    for (const QString &pattern : patterns) {
        QRegularExpression expression(pattern.trimmed(), QRegularExpression::CaseInsensitiveOption);
        if (!pattern.trimmed().isEmpty() && expression.isValid()) {
            m_blocklist.append(expression);
        }
    }

    for (auto it = m_windows.begin(); it != m_windows.end(); ++it) {
        it->dirty = true;
        if (!isSampleable(it.key())) {
            publish(it.key(), QColor());
        }
    }
}

//________________________________________________________________
void TintSamplerEffect::slotWindowAdded(EffectWindow *w)
{
    m_windows.insert(w, WindowState());
    connect(w, &EffectWindow::windowDamaged, this, &TintSamplerEffect::slotWindowDamaged);
}

//________________________________________________________________
void TintSamplerEffect::slotWindowDamaged(EffectWindow *w)
{
    const auto it = m_windows.find(w);
    if (it != m_windows.end()) {
        it->dirty = true;
    }
}

//________________________________________________________________
void TintSamplerEffect::slotWindowDeleted(EffectWindow *w)
{
    m_windows.remove(w);
}

//________________________________________________________________
bool TintSamplerEffect::isSampleable(EffectWindow *w) const
{
    if (w->isDeleted() || w->isMinimized() || !w->isOnCurrentDesktop() || !w->hasDecoration()) {
        return false;
    }
    if (!w->isNormalWindow() && !w->isDialog()) {
        return false;
    }

    const QString windowClass = w->windowClass();
    for (const QRegularExpression &expression : m_blocklist) {
        if (expression.match(windowClass).hasMatch()) {
            return false;
        }
    }

    return hasSampledColorSlot(w->decoration());
}

//________________________________________________________________
bool TintSamplerEffect::paintScreen(const RenderTarget &renderTarget, const RenderViewport &viewport, int mask, const Region &deviceRegion, LogicalOutput *screen)
{
    const bool result = effects->paintScreen(renderTarget, viewport, mask, deviceRegion, screen);

    int samples = 0;
    qint64 nextDue = -1;
    for (auto it = m_windows.begin(); it != m_windows.end(); ++it) {
        WindowState &state = it.value();
        if (!state.dirty || !isSampleable(it.key())) {
            continue;
        }

        const qint64 remaining = state.lastSample.isValid() ? m_interval - state.lastSample.elapsed() : 0;
        if (remaining > 0 || samples >= MaxSamplesPerFrame) {
            const qint64 delay = std::max<qint64>(remaining, 0);
            nextDue = nextDue < 0 ? delay : std::min(nextDue, delay);
            continue;
        }

        const QColor color = sample(it.key());
        qCDebug(KWIN_TINTSAMPLER) << "sampled" << it.key()->windowClass() << color;
        state.dirty = false;
        state.lastSample.start();
        ++samples;
        if (color.isValid()) {
            publish(it.key(), color);
        }
    }

    if (nextDue >= 0) {
        scheduleDeferred(nextDue);
    }

    return result;
}

//________________________________________________________________
void TintSamplerEffect::scheduleDeferred(qint64 delay)
{
    if (m_deferredTimer.isActive() && m_deferredTimer.remainingTime() <= delay) {
        return;
    }
    m_deferredTimer.start(std::max<qint64>(delay, 16));
}

//________________________________________________________________
QColor TintSamplerEffect::sample(EffectWindow *w)
{
    const RectF client = w->clientGeometry();
    if (client.isEmpty()) {
        return QColor();
    }

    const qreal scale = SampleSize / std::max(client.width(), client.height());
    const QSize size(std::max(1, int(std::ceil(client.width() * scale))), std::max(1, int(std::ceil(client.height() * scale))));

    if (!m_texture || m_texture->size() != size) {
        m_framebuffer.reset();
        m_texture = GLTexture::allocate(GL_RGBA8, size);
        if (!m_texture) {
            return QColor();
        }
        m_framebuffer = std::make_unique<GLFramebuffer>(m_texture.get());
        if (!m_framebuffer->valid()) {
            m_framebuffer.reset();
            m_texture.reset();
            return QColor();
        }
    }

    QList<uchar> pixels(size.width() * size.height() * 4);

    GLFramebuffer::pushFramebuffer(m_framebuffer.get());
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    const RenderTarget target(m_framebuffer.get());
    const RenderViewport sampleViewport(client, scale, target, QPoint());
    WindowPaintData data;
    const bool drawn = effects->drawWindow(target, sampleViewport, w, PAINT_WINDOW_TRANSFORMED | PAINT_WINDOW_TRANSLUCENT, Region::infinite(), data);

    if (drawn) {
        glReadPixels(0, 0, size.width(), size.height(), GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    }
    GLFramebuffer::popFramebuffer();

    if (!drawn) {
        return QColor();
    }

    // thickness of the edge strip, in sample pixels
    const int edge = std::min(size.width(), size.height()) >= 8 ? 2 : 1;

    // the output is premultiplied, so summing the channels and dividing by the summed alpha un-premultiplies
    qint64 r = 0;
    qint64 g = 0;
    qint64 b = 0;
    qint64 a = 0;
    for (int y = 0; y < size.height(); ++y) {
        for (int x = 0; x < size.width(); ++x) {
            if (m_mode == Mode::Edges && x >= edge && y >= edge && x < size.width() - edge && y < size.height() - edge) {
                continue;
            }

            const uchar *pixel = pixels.constData() + (y * size.width() + x) * 4;
            if (pixel[3] < MinAlpha) {
                continue;
            }
            r += pixel[0];
            g += pixel[1];
            b += pixel[2];
            a += pixel[3];
        }
    }

    if (a == 0) {
        return QColor();
    }

    return QColor(std::min<qint64>(255, r * 255 / a), std::min<qint64>(255, g * 255 / a), std::min<qint64>(255, b * 255 / a));
}

//________________________________________________________________
void TintSamplerEffect::publish(EffectWindow *w, const QColor &color)
{
    QObject *decoration = w->decoration();
    if (!hasSampledColorSlot(decoration)) {
        return;
    }
    QMetaObject::invokeMethod(decoration, "setSampledColor", Qt::DirectConnection, Q_ARG(QColor, color));
}

}

#include "moc_tintsampler.cpp"
