add_library(tintedcommon OBJECT tintedboxshadowrenderer.cpp)

set_target_properties(tintedcommon PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_include_directories(tintedcommon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(tintedcommon
    PUBLIC
        Qt6::Core
        Qt6::Gui)
