//////////////////////////////////////////////////////////////////////////////
// breezetintedconfigurationui.cpp
// -------------------
//
// SPDX-FileCopyrightText: 2009 Hugo Pereira Da Costa <hugo.pereira@free.fr>
//
// SPDX-License-Identifier: MIT
//////////////////////////////////////////////////////////////////////////////

#include "breezetintedconfigwidget.h"
#include "breezetintedexceptionlist.h"

#include <KLocalizedString>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QFontDatabase>
#include <QFormLayout>
#include <QLabel>
#include <QListWidget>
#include <QSpinBox>

namespace BreezeTinted
{
namespace
{
struct TintSource {
    QString key;
    QString label;
};

QList<TintSource> allTintSources()
{
    return {
        {QStringLiteral("sampled"), i18n("Window contents (requires the Tint Sampler effect)")},
        {QStringLiteral("palette"), i18n("Application color scheme")},
        {QStringLiteral("icon"), i18n("Application icon")},
    };
}

QSpinBox *createSpinBox(int min, int max, const QString &suffix, QWidget *parent)
{
    auto spinBox = new QSpinBox(parent);
    spinBox->setRange(min, max);
    spinBox->setSuffix(suffix);
    return spinBox;
}
}

//_________________________________________________________
ConfigWidget::ConfigWidget(QObject *parent, const KPluginMetaData &data, const QVariantList & /*args*/)
    : KCModule(parent, data)
    , m_configuration(KSharedConfig::openConfig(QStringLiteral("breezetintedrc")))
    , m_changed(false)
{
    // configuration
    m_ui.setupUi(widget());

    m_ui.tabWidget->tabBar()->setExpanding(true);

    // track ui changes
    connect(m_ui.titleAlignment, SIGNAL(currentIndexChanged(int)), SLOT(updateChanged()));
    connect(m_ui.buttonSize, SIGNAL(currentIndexChanged(int)), SLOT(updateChanged()));
    connect(m_ui.outlineCloseButton, &QAbstractButton::clicked, this, &ConfigWidget::updateChanged);
    connect(m_ui.drawBorderOnMaximizedWindows, &QAbstractButton::clicked, this, &ConfigWidget::updateChanged);
    connect(m_ui.drawBackgroundGradient, &QAbstractButton::clicked, this, &ConfigWidget::updateChanged);
    connect(m_ui.roundedCorners, &QAbstractButton::clicked, this, &ConfigWidget::updateChanged);

    // track shadows changes
    connect(m_ui.shadowSize, SIGNAL(currentIndexChanged(int)), SLOT(updateChanged()));
    connect(m_ui.shadowStrength, SIGNAL(valueChanged(int)), SLOT(updateChanged()));
    connect(m_ui.shadowColor, &KColorButton::changed, this, &ConfigWidget::updateChanged);
    connect(m_ui.drawOutline, &QAbstractButton::clicked, this, &ConfigWidget::updateChanged);

    setupTintTab();

    // track exception changes
    connect(m_ui.exceptions, &ExceptionListWidget::changed, this, &ConfigWidget::updateChanged);

    // set formatting
    m_ui.drawBorderOnMaximizedWindowsHelpLabel->setFont(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));
}

//_________________________________________________________
void ConfigWidget::load()
{
    // create internal settings and load from rc files
    m_internalSettings = InternalSettingsPtr(new InternalSettings());
    m_internalSettings->load();

    // assign to ui
    m_ui.titleAlignment->setCurrentIndex(m_internalSettings->titleAlignment());
    m_ui.buttonSize->setCurrentIndex(m_internalSettings->buttonSize());
    m_ui.drawBorderOnMaximizedWindows->setChecked(m_internalSettings->drawBorderOnMaximizedWindows());
    m_ui.outlineCloseButton->setChecked(m_internalSettings->outlineCloseButton());
    m_ui.drawBackgroundGradient->setChecked(m_internalSettings->drawBackgroundGradient());
    m_ui.roundedCorners->setChecked(m_internalSettings->roundedCorners());

    // load shadows
    if (m_internalSettings->shadowSize() <= InternalSettings::ShadowVeryLarge) {
        m_ui.shadowSize->setCurrentIndex(m_internalSettings->shadowSize());
    } else {
        m_ui.shadowSize->setCurrentIndex(InternalSettings::ShadowLarge);
    }

    m_ui.drawOutline->setChecked(m_internalSettings->outlineEnabled());

    m_ui.shadowStrength->setValue(qRound(qreal(m_internalSettings->shadowStrength() * 100) / 255));
    m_ui.shadowColor->setColor(m_internalSettings->shadowColor());

    loadTint(*m_internalSettings);

    // load exceptions
    ExceptionList exceptions;
    exceptions.readConfig(m_configuration);
    m_ui.exceptions->setExceptions(exceptions.get());
    setNeedsSave(false);
}

//_________________________________________________________
void ConfigWidget::save()
{
    // create internal settings and load from rc files
    m_internalSettings = InternalSettingsPtr(new InternalSettings());
    m_internalSettings->load();

    // apply modifications from ui
    m_internalSettings->setTitleAlignment(m_ui.titleAlignment->currentIndex());
    m_internalSettings->setButtonSize(m_ui.buttonSize->currentIndex());
    m_internalSettings->setOutlineCloseButton(m_ui.outlineCloseButton->isChecked());
    m_internalSettings->setDrawBorderOnMaximizedWindows(m_ui.drawBorderOnMaximizedWindows->isChecked());
    m_internalSettings->setDrawBackgroundGradient(m_ui.drawBackgroundGradient->isChecked());
    m_internalSettings->setRoundedCorners(m_ui.roundedCorners->isChecked());

    m_internalSettings->setShadowSize(m_ui.shadowSize->currentIndex());
    m_internalSettings->setShadowStrength(qRound(qreal(m_ui.shadowStrength->value() * 255) / 100));
    m_internalSettings->setShadowColor(m_ui.shadowColor->color());

    m_internalSettings->setOutlineEnabled(m_ui.drawOutline->isChecked());

    saveTint(*m_internalSettings);

    // save configuration
    m_internalSettings->save();

    // get list of exceptions and write
    InternalSettingsList exceptions(m_ui.exceptions->exceptions());
    ExceptionList(exceptions).writeConfig(m_configuration);

    // sync configuration
    m_configuration->sync();
    setNeedsSave(false);

    // needed to tell kwin to reload when running from external kcmshell
    {
        QDBusMessage message = QDBusMessage::createSignal("/KWin", "org.kde.KWin", "reloadConfig");
        QDBusConnection::sessionBus().send(message);
    }
}

//_________________________________________________________
void ConfigWidget::defaults()
{
    // create internal settings and load from rc files
    m_internalSettings = InternalSettingsPtr(new InternalSettings());
    m_internalSettings->setDefaults();

    // assign to ui
    m_ui.titleAlignment->setCurrentIndex(m_internalSettings->titleAlignment());
    m_ui.buttonSize->setCurrentIndex(m_internalSettings->buttonSize());
    m_ui.outlineCloseButton->setChecked(m_internalSettings->outlineCloseButton());
    m_ui.drawBorderOnMaximizedWindows->setChecked(m_internalSettings->drawBorderOnMaximizedWindows());
    m_ui.drawBackgroundGradient->setChecked(m_internalSettings->drawBackgroundGradient());
    m_ui.roundedCorners->setChecked(m_internalSettings->roundedCorners());

    m_ui.shadowSize->setCurrentIndex(m_internalSettings->shadowSize());
    m_ui.shadowStrength->setValue(qRound(qreal(m_internalSettings->shadowStrength() * 100) / 255));
    m_ui.shadowColor->setColor(m_internalSettings->shadowColor());
    m_ui.drawOutline->setChecked(m_internalSettings->outlineEnabled());

    loadTint(*m_internalSettings);
}

//_______________________________________________
void ConfigWidget::updateChanged()
{
    // check configuration
    if (!m_internalSettings) {
        return;
    }

    // track modifications
    bool modified(false);

    if (m_ui.titleAlignment->currentIndex() != m_internalSettings->titleAlignment()) {
        modified = true;
    } else if (m_ui.buttonSize->currentIndex() != m_internalSettings->buttonSize()) {
        modified = true;
    } else if (m_ui.outlineCloseButton->isChecked() != m_internalSettings->outlineCloseButton()) {
        modified = true;
    } else if (m_ui.drawBorderOnMaximizedWindows->isChecked() != m_internalSettings->drawBorderOnMaximizedWindows()) {
        modified = true;
    } else if (m_ui.drawBackgroundGradient->isChecked() != m_internalSettings->drawBackgroundGradient()) {
        modified = true;
    } else if (m_ui.roundedCorners->isChecked() != m_internalSettings->roundedCorners()) {
        modified = true;

        // shadows
    } else if (m_ui.shadowSize->currentIndex() != m_internalSettings->shadowSize()) {
        modified = true;
    } else if (qRound(qreal(m_ui.shadowStrength->value() * 255) / 100) != m_internalSettings->shadowStrength()) {
        modified = true;
    } else if (m_ui.shadowColor->color() != m_internalSettings->shadowColor()) {
        modified = true;
    } else if (m_ui.drawOutline->isChecked() != m_internalSettings->outlineEnabled()) {
        modified = true;
        // tint
    } else if (tintSources() != m_internalSettings->tintSources()) {
        modified = true;
    } else if (m_sampledMinSaturation->value() != m_internalSettings->sampledMinSaturation()
               || m_tintSaturation->value() != m_internalSettings->tintSaturation()
               || m_tintMinLightness->value() != m_internalSettings->tintMinLightness()
               || m_tintMaxLightness->value() != m_internalSettings->tintMaxLightness()
               || m_tintAccentMix->value() != m_internalSettings->tintAccentMix()
               || m_outlineIntensityActive->value() != m_internalSettings->outlineIntensityActive()
               || m_outlineIntensityInactive->value() != m_internalSettings->outlineIntensityInactive()
               || m_outlineThickness->value() != m_internalSettings->outlineThickness()
               || m_titleBarTintActive->value() != m_internalSettings->titleBarTintActive()
               || m_titleBarTintInactive->value() != m_internalSettings->titleBarTintInactive()
               || m_tintTransitionDuration->value() != m_internalSettings->tintTransitionDuration()) {
        modified = true;
        // exceptions
    } else if (m_ui.exceptions->isChanged()) {
        modified = true;
    }

    setNeedsSave(modified);
}

//_________________________________________________________
void ConfigWidget::setupTintTab()
{
    auto tab = new QWidget;
    auto layout = new QFormLayout(tab);
    layout->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);

    m_tintSources = new QListWidget(tab);
    m_tintSources->setDragDropMode(QAbstractItemView::InternalMove);
    m_tintSources->setDefaultDropAction(Qt::MoveAction);
    m_tintSources->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    layout->addRow(i18n("Color sources:"), m_tintSources);

    auto sourcesHelp = new QLabel(i18n("Drag to reorder. The first source that yields a color wins; windows with no match use the accent color."), tab);
    sourcesHelp->setWordWrap(true);
    sourcesHelp->setFont(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));
    layout->addRow(QString(), sourcesHelp);

    m_sampledMinSaturation = createSpinBox(0, 100, i18n("%"), tab);
    m_sampledMinSaturation->setToolTip(i18n("Window contents less saturated than this are ignored, so the next source is used instead."));
    layout->addRow(i18n("Minimum saturation of window contents:"), m_sampledMinSaturation);

    m_tintSaturation = createSpinBox(0, 300, i18n("%"), tab);
    layout->addRow(i18n("Saturation:"), m_tintSaturation);

    m_tintMinLightness = createSpinBox(0, 100, i18n("%"), tab);
    layout->addRow(i18n("Minimum lightness:"), m_tintMinLightness);

    m_tintMaxLightness = createSpinBox(0, 100, i18n("%"), tab);
    layout->addRow(i18n("Maximum lightness:"), m_tintMaxLightness);

    m_tintAccentMix = createSpinBox(0, 100, i18n("%"), tab);
    layout->addRow(i18n("Blend with accent color:"), m_tintAccentMix);

    m_outlineIntensityActive = createSpinBox(0, 100, i18n("%"), tab);
    layout->addRow(i18n("Outline intensity (active):"), m_outlineIntensityActive);

    m_outlineIntensityInactive = createSpinBox(0, 100, i18n("%"), tab);
    layout->addRow(i18n("Outline intensity (inactive):"), m_outlineIntensityInactive);

    m_outlineThickness = createSpinBox(1, 8, i18n(" px"), tab);
    layout->addRow(i18n("Outline thickness:"), m_outlineThickness);

    m_titleBarTintActive = createSpinBox(0, 100, i18n("%"), tab);
    layout->addRow(i18n("Title bar tint (active):"), m_titleBarTintActive);

    m_titleBarTintInactive = createSpinBox(0, 100, i18n("%"), tab);
    layout->addRow(i18n("Title bar tint (inactive):"), m_titleBarTintInactive);

    m_tintTransitionDuration = createSpinBox(0, 2000, i18n(" ms"), tab);
    m_tintTransitionDuration->setSingleStep(50);
    layout->addRow(i18n("Color transition:"), m_tintTransitionDuration);

    m_ui.tabWidget->insertTab(1, tab, i18n("Tint"));

    connect(m_tintSources, &QListWidget::itemChanged, this, &ConfigWidget::updateChanged);
    connect(m_tintSources->model(), &QAbstractItemModel::rowsMoved, this, &ConfigWidget::updateChanged);
    const QList<QSpinBox *> spinBoxes = {m_sampledMinSaturation,
                                         m_tintSaturation,
                                         m_tintMinLightness,
                                         m_tintMaxLightness,
                                         m_tintAccentMix,
                                         m_outlineIntensityActive,
                                         m_outlineIntensityInactive,
                                         m_outlineThickness,
                                         m_titleBarTintActive,
                                         m_titleBarTintInactive,
                                         m_tintTransitionDuration};
    for (QSpinBox *spinBox : spinBoxes) {
        connect(spinBox, &QSpinBox::valueChanged, this, &ConfigWidget::updateChanged);
    }
}

//_________________________________________________________
void ConfigWidget::loadTint(const InternalSettings &settings)
{
    // enabled sources first, in their configured order, then the disabled ones
    const QStringList enabled = settings.tintSources();
    QList<TintSource> sources = allTintSources();
    std::stable_sort(sources.begin(), sources.end(), [&enabled](const TintSource &a, const TintSource &b) {
        const auto indexOf = [&enabled](const TintSource &source) {
            const int index = enabled.indexOf(source.key);
            return index < 0 ? enabled.size() : index;
        };
        return indexOf(a) < indexOf(b);
    });

    const QSignalBlocker blocker(m_tintSources);
    m_tintSources->clear();
    for (const TintSource &source : std::as_const(sources)) {
        auto item = new QListWidgetItem(source.label, m_tintSources);
        item->setData(Qt::UserRole, source.key);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsDragEnabled);
        item->setCheckState(enabled.contains(source.key) ? Qt::Checked : Qt::Unchecked);
    }

    m_sampledMinSaturation->setValue(settings.sampledMinSaturation());
    m_tintSaturation->setValue(settings.tintSaturation());
    m_tintMinLightness->setValue(settings.tintMinLightness());
    m_tintMaxLightness->setValue(settings.tintMaxLightness());
    m_tintAccentMix->setValue(settings.tintAccentMix());
    m_outlineIntensityActive->setValue(settings.outlineIntensityActive());
    m_outlineIntensityInactive->setValue(settings.outlineIntensityInactive());
    m_outlineThickness->setValue(settings.outlineThickness());
    m_titleBarTintActive->setValue(settings.titleBarTintActive());
    m_titleBarTintInactive->setValue(settings.titleBarTintInactive());
    m_tintTransitionDuration->setValue(settings.tintTransitionDuration());
}

//_________________________________________________________
void ConfigWidget::saveTint(InternalSettings &settings) const
{
    settings.setTintSources(tintSources());
    settings.setSampledMinSaturation(m_sampledMinSaturation->value());
    settings.setTintSaturation(m_tintSaturation->value());
    settings.setTintMinLightness(m_tintMinLightness->value());
    settings.setTintMaxLightness(m_tintMaxLightness->value());
    settings.setTintAccentMix(m_tintAccentMix->value());
    settings.setOutlineIntensityActive(m_outlineIntensityActive->value());
    settings.setOutlineIntensityInactive(m_outlineIntensityInactive->value());
    settings.setOutlineThickness(m_outlineThickness->value());
    settings.setTitleBarTintActive(m_titleBarTintActive->value());
    settings.setTitleBarTintInactive(m_titleBarTintInactive->value());
    settings.setTintTransitionDuration(m_tintTransitionDuration->value());
}

//_________________________________________________________
QStringList ConfigWidget::tintSources() const
{
    QStringList sources;
    for (int i = 0; i < m_tintSources->count(); ++i) {
        const QListWidgetItem *item = m_tintSources->item(i);
        if (item->checkState() == Qt::Checked) {
            sources.append(item->data(Qt::UserRole).toString());
        }
    }
    return sources;
}

}
