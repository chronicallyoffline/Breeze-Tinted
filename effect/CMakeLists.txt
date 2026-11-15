find_package(KWin REQUIRED)

kcoreaddons_add_plugin(tintsampler
    SOURCES main.cpp tintsampler.cpp
    INSTALL_NAMESPACE "kwin/effects/plugins"
)

target_link_libraries(tintsampler
    PRIVATE
        KWin::kwin
        KDecoration3::KDecoration
        KF6::ConfigCore
)
