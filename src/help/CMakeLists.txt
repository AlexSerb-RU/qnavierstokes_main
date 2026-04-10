add_library(reference STATIC
    help_browser.hpp
    help_browser.cpp
    help_engine.cpp
    help_engine.hpp
    help_window.cpp
    help_window.hpp
    help_window.ui
)

target_link_libraries(reference PRIVATE
    Qt6::Widgets
    Qt6::Help
)

target_include_directories(reference PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set(QHELPGENERATOR_EXECUTABLE "C:/Qt6/6.8.3/msvc2022_64/bin/qhelpgenerator.exe")

add_custom_target(reference_content
    ALL
    COMMAND "${QHELPGENERATOR_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/qnavierstokes.qhcp"
            -o
            "${CMAKE_CURRENT_BINARY_DIR}/qnavierstokes.qch"
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/qnavierstokes.qhcp
        ${CMAKE_CURRENT_SOURCE_DIR}/qnavierstokes.qhp
)