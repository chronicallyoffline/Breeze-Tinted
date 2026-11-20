add_library(breezetintedcommon OBJECT breezetintedboxshadowrenderer.cpp)

set_target_properties(breezetintedcommon PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_include_directories(breezetintedcommon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(breezetintedcommon
    PUBLIC
        Qt6::Core
        Qt6::Gui)
