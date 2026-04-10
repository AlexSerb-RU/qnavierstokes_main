add_library(los STATIC
    los.cpp
    los.hpp
)

target_link_libraries(los PRIVATE
    csr_tools
    gauss
)

target_include_directories(
    los INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)
