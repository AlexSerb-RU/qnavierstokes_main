add_library(gauss STATIC
    gauss.cpp
    gauss.hpp
)

target_include_directories(
    gauss INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)
