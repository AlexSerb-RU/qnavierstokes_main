add_library(settings STATIC
    settings.hpp
    settings_manager.cpp
    settings_manager.hpp
    settings_window.cpp
    settings_window.hpp
    settings_window.ui
)

target_link_libraries(settings PRIVATE
    Qt6::Core
    Qt6::Widgets
)

target_include_directories(
    settings INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)
