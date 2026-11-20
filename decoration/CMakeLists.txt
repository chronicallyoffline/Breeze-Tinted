add_definitions(-DTRANSLATION_DOMAIN="breezetinted_kwin_deco")

set(breezetinteddecoration_SRCS
    breezetintedbutton.cpp
    breezetinteddecoration.cpp
    breezetintedsettingsprovider.cpp
    breezetintedexceptionlist.cpp
    colorresolver.cpp
)

kconfig_add_kcfg_files(breezetinteddecoration_SRCS breezetintedsettings.kcfgc)
add_library(breezetinteddecoration MODULE ${breezetinteddecoration_SRCS})

set_target_properties(breezetinteddecoration PROPERTIES
    OUTPUT_NAME org.kde.breezetinted
)

target_link_libraries(breezetinteddecoration
    PRIVATE
        breezetintedcommon
        Qt6::DBus
        KF6::CoreAddons
        KF6::ConfigGui
        KF6::GuiAddons
        KF6::I18n
        KF6::IconThemes
        KDecoration3::KDecoration
        KF6::ColorScheme
)

install(TARGETS breezetinteddecoration DESTINATION ${KDE_INSTALL_PLUGINDIR}/${KDECORATION_PLUGIN_DIR})
install(FILES breezetintedsettingsdata.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR} RENAME breezetinted-decoration.kcfg)

add_subdirectory(config)
