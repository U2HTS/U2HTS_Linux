cmake_minimum_required(VERSION 3.20)

project(U2HTS_Linux C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(USBGX REQUIRED IMPORTED_TARGET libusbgx)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/u2hts_core)

set(TOUCH_CONTROLLERS
    ${CMAKE_CURRENT_LIST_DIR}/u2hts_touch_controllers/dummy_test.c
    ${CMAKE_CURRENT_LIST_DIR}/u2hts_touch_controllers/linux-multitouch.c
)

add_executable(U2HTS_Linux 
    ${CMAKE_CURRENT_LIST_DIR}/main.c
    ${CMAKE_CURRENT_LIST_DIR}/src/u2hts_linux.c
    ${TOUCH_CONTROLLERS}
)

target_link_libraries(U2HTS_Linux PUBLIC
    u2hts_core
    PkgConfig::USBGX
)

target_include_directories(U2HTS_Linux PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)   