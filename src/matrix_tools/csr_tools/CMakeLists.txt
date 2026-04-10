add_library(csr_tools STATIC
    csr_tools.cpp
    csr_tools.hpp
)

target_include_directories(
    csr_tools INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)
