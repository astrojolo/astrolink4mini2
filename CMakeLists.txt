cmake_minimum_required(VERSION 3.10)

project(indi-astrolink4mini2 VERSION 1.1 LANGUAGES CXX C)

# Make sure custom cmake modules are discoverable
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules" ${CMAKE_MODULE_PATH})

# Use portable install dir variables
include(GNUInstallDirs)

# Use a modern, CMake-managed way to request C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find INDI (unchanged)
find_package(INDI REQUIRED)

# Include common project CMake snippets if present
include(CMakeCommon)

################ AstroLink4 mini II ################

set(indi_astrolink4mini2_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/indi_astrolink4mini2.cpp
)

add_executable(indi_astrolink4mini2 ${indi_astrolink4mini2_SRCS})

# Prefer target-based include and link handling
target_include_directories(indi_astrolink4mini2
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${INDI_INCLUDE_DIR}
)

target_link_libraries(indi_astrolink4mini2
  PRIVATE
    indidriver
)

# Install rules using GNUInstallDirs variables for portability
install(TARGETS indi_astrolink4mini2
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES indi_astrolink4mini2.xml DESTINATION ${INDI_DATA_DIR})