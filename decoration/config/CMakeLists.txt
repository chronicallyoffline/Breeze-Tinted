set(tinteddecoration_config_SRCS
    ../tintedexceptionlist.cpp
    tintedconfigwidget.cpp
    tinteddetectwidget.cpp
    tintedexceptiondialog.cpp
    tintedexceptionlistwidget.cpp
    tintedexceptionmodel.cpp
    tinteditemmodel.cpp
)
ki18n_wrap_ui(tinteddecoration_config_SRCS
   ui/tintedconfigurationui.ui
   ui/tintedexceptiondialog.ui
   ui/tintedexceptionlistwidget.ui
)

kcoreaddons_add_plugin(kcm_tinteddecoration SOURCES kcm_tinteddecoration.cpp ${tinteddecoration_config_SRCS} INSTALL_NAMESPACE "${KDECORATION_KCM_PLUGIN_DIR}")
kconfig_add_kcfg_files(kcm_tinteddecoration ../tintedsettings.kcfgc)
target_include_directories(kcm_tinteddecoration PRIVATE ${CMAKE_SOURCE_DIR}/decoration ${CMAKE_BINARY_DIR}/decoration/)
target_link_libraries(kcm_tinteddecoration
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
kcmutils_generate_desktop_file(kcm_tinteddecoration)

# kcmshell6 < 6.27.0 doesn't know the --smoke-test param, so skip this test for
# now.
# TODO: Drop this when KF6_MIN_VERSION >= 6.27.0
if(KF6KCMUtils_VERSION VERSION_GREATER_EQUAL "6.27.0")
    add_test(NAME kcm_tinteddecoration_smoketest COMMAND kcmshell6 --smoke-test ${KDECORATION_KCM_PLUGIN_DIR}/kcm_tinteddecoration)
    set_tests_properties(kcm_tinteddecoration_smoketest PROPERTIES
        ENVIRONMENT_MODIFICATION QT_PLUGIN_PATH=path_list_prepend:${CMAKE_BINARY_DIR}/bin
    )
endif()
