set(breezetinteddecoration_config_SRCS
    ../breezetintedexceptionlist.cpp
    breezetintedconfigwidget.cpp
    breezetinteddetectwidget.cpp
    breezetintedexceptiondialog.cpp
    breezetintedexceptionlistwidget.cpp
    breezetintedexceptionmodel.cpp
    breezetinteditemmodel.cpp
)
ki18n_wrap_ui(breezetinteddecoration_config_SRCS
   ui/breezetintedconfigurationui.ui
   ui/breezetintedexceptiondialog.ui
   ui/breezetintedexceptionlistwidget.ui
)

kcoreaddons_add_plugin(kcm_breezetinteddecoration SOURCES kcm_breezetinteddecoration.cpp ${breezetinteddecoration_config_SRCS} INSTALL_NAMESPACE "${KDECORATION_KCM_PLUGIN_DIR}")
kconfig_add_kcfg_files(kcm_breezetinteddecoration ../breezetintedsettings.kcfgc)
target_include_directories(kcm_breezetinteddecoration PRIVATE ${CMAKE_SOURCE_DIR}/decoration ${CMAKE_BINARY_DIR}/decoration/)
target_link_libraries(kcm_breezetinteddecoration
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::DBus
    PRIVATE
        KF6::ConfigCore
        KF6::CoreAddons
        KF6::GuiAddons
        KF6::I18n
        KF6::IconThemes
        KF6::KCMUtils
        KF6::WidgetsAddons
)
kcmutils_generate_desktop_file(kcm_breezetinteddecoration)

# kcmshell6 < 6.27.0 doesn't know the --smoke-test param, so skip this test for
# now.
# TODO: Drop this when KF6_MIN_VERSION >= 6.27.0
if(KF6KCMUtils_VERSION VERSION_GREATER_EQUAL "6.27.0")
    add_test(NAME kcm_breezetinteddecoration_smoketest COMMAND kcmshell6 --smoke-test ${KDECORATION_KCM_PLUGIN_DIR}/kcm_breezetinteddecoration)
    set_tests_properties(kcm_breezetinteddecoration_smoketest PROPERTIES
        ENVIRONMENT_MODIFICATION QT_PLUGIN_PATH=path_list_prepend:${CMAKE_BINARY_DIR}/bin
    )
endif()
