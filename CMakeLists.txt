cmake_minimum_required(VERSION 3.16)
project(kwin-tinted VERSION 0.1.0)

set(QT_MIN_VERSION "6.8.0")
set(KF6_MIN_VERSION "6.10.0")
# match upstream Breeze, which still relies on Qt keywords and implicit QString casts
set(KDE_COMPILERSETTINGS_LEVEL "5.82")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED CONFIG COMPONENTS Core Gui Widgets DBus)
find_package(KF6 ${KF6_MIN_VERSION} REQUIRED COMPONENTS
    ColorScheme
    Config
    CoreAddons
    GuiAddons
    I18n
    IconThemes
    KCMUtils
    WidgetsAddons
)
find_package(KDecoration3 REQUIRED)

option(BUILD_EFFECT "Build the tintsampler KWin effect (content color sampling)" OFF)

add_subdirectory(common)
add_subdirectory(decoration)
if(BUILD_EFFECT)
    add_subdirectory(effect)
endif()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
