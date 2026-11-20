//////////////////////////////////////////////////////////////////////////////
// breezetintedconfigurationui.h
// -------------------
//
// SPDX-FileCopyrightText: 2009 Hugo Pereira Da Costa <hugo.pereira@free.fr>
//
// SPDX-License-Identifier: MIT
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include "breezetinted.h"
#include "breezetintedexceptionlistwidget.h"
#include "breezetintedsettings.h"
#include "ui_breezetintedconfigurationui.h"

#include <KCModule>
#include <KSharedConfig>

#include <QSharedPointer>
#include <QWidget>

class QListWidget;
class QSpinBox;

namespace BreezeTinted
{
//_____________________________________________
class ConfigWidget : public KCModule
{
    Q_OBJECT

public:
    //* constructor
    explicit ConfigWidget(QObject *parent, const KPluginMetaData &data, const QVariantList &args);

    //* destructor
    virtual ~ConfigWidget() = default;

    //* default
    void defaults() override;

    //* load configuration
    void load() override;

    //* save configuration
    void save() override;

protected Q_SLOTS:

    //* update changed state
    virtual void updateChanged();

private:
    //* build the Tint tab, which is not part of the .ui file
    void setupTintTab();

    //* assign tint settings to the Tint tab
    void loadTint(const InternalSettings &settings);

    //* copy the Tint tab into settings
    void saveTint(InternalSettings &settings) const;

    //* tint sources, in list order, only the checked ones
    QStringList tintSources() const;

    //* ui
    Ui_BreezeTintedConfigurationUI m_ui;

    //* Tint tab widgets
    QSpinBox *m_cornerRadius = nullptr;
    QListWidget *m_tintSources = nullptr;
    QSpinBox *m_sampledMinSaturation = nullptr;
    QSpinBox *m_tintSaturation = nullptr;
    QSpinBox *m_tintMinLightness = nullptr;
    QSpinBox *m_tintMaxLightness = nullptr;
    QSpinBox *m_tintAccentMix = nullptr;
    QSpinBox *m_outlineIntensityActive = nullptr;
    QSpinBox *m_outlineIntensityInactive = nullptr;
    QSpinBox *m_outlineThickness = nullptr;
    QSpinBox *m_titleBarTintActive = nullptr;
    QSpinBox *m_titleBarTintInactive = nullptr;
    QSpinBox *m_tintTransitionDuration = nullptr;

    //* kconfiguration object
    KSharedConfig::Ptr m_configuration;

    //* internal exception
    InternalSettingsPtr m_internalSettings;

    //* changed state
    bool m_changed;
};

}
