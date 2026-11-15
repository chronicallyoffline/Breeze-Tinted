add_definitions(-DTRANSLATION_DOMAIN="tinted_kwin_deco")

set(tinteddecoration_SRCS
    tintedbutton.cpp
    tinteddecoration.cpp
    tintedsettingsprovider.cpp
    tintedexceptionlist.cpp
    colorresolver.cpp
)

kconfig_add_kcfg_files(tinteddecoration_SRCS tintedsettings.kcfgc)
add_library(tinteddecoration MODULE ${tinteddecoration_SRCS})

set_target_properties(tinteddecoration PROPERTIES
    OUTPUT_NAME org.kde.tinted
)

target_link_libraries(tinteddecoration
    PRIVATE
        tintedcommon
        Qt6::DBus
        KF6::CoreAddons
        KF6::ConfigGui
        KF6::GuiAddons
        KF6::I18n
        KF6::IconThemes
        KDecoration3::KDecoration
        KF6::ColorScheme
)

install(TARGETS tinteddecoration DESTINATION ${KDE_INSTALL_PLUGINDIR}/${KDECORATION_PLUGIN_DIR})
install(FILES tintedsettingsdata.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR} RENAME tinted-decoration.kcfg)

add_subdirectory(config)
