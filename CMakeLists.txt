cmake_minimum_required(VERSION 2.4.7)
PROJECT(indi-focuserlink CXX C)

set (VERSION_MAJOR 0)
set (VERSION_MINOR 1)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
set(BIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin")
set(CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")


find_package(INDI REQUIRED)

include_directories( ${CMAKE_CURRENT_BINARY_DIR})
include_directories( ${CMAKE_CURRENT_SOURCE_DIR})
include_directories( ${INDI_INCLUDE_DIR})

################ AstroLink4 mini II ################

set(indi_astrolink4mini2_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/indi_astrolink4mini2.cpp
   )

add_executable(indi_astrolink4mini2 ${indi_astrolink4mini2_SRCS})
target_link_libraries(indi_astrolink4mini2 indidriver)
install(TARGETS indi_astrolink4mini2 RUNTIME DESTINATION bin )
install(FILES indi_astrolink4mini2.xml DESTINATION ${INDI_DATA_DIR})

