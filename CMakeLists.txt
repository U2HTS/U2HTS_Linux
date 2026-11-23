cmake_minimum_required(VERSION 3.20)

project(U2HTS_Linux C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(USBGX REQUIRED IMPORTED_TARGET libusbgx)
find_package(Git QUIET)

if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --dirty --always
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        OUTPUT_VARIABLE U2HTS_LINUX_GIT_SHA
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE GIT_RET
    )
endif()

if(NOT GIT_FOUND OR NOT GIT_RET EQUAL 0 OR U2HTS_LINUX_GIT_SHA STREQUAL "")
    set(U2HTS_LINUX_GIT_SHA "UNKNOWN")
endif()

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/u2hts_core)

set(TOUCH_CONTROLLERS
    ${CMAKE_CURRENT_LIST_DIR}/u2hts_touch_controllers/linux-multitouch.c
)

add_executable(U2HTS_Linux 
    ${CMAKE_CURRENT_LIST_DIR}/main.c
    ${CMAKE_CURRENT_LIST_DIR}/src/u2hts_linux.c
    ${TOUCH_CONTROLLERS}
)

target_compile_definitions(U2HTS_Linux PUBLIC
    U2HTS_LINUX_GIT_SHA="${U2HTS_LINUX_GIT_SHA}"
)

target_link_libraries(U2HTS_Linux PUBLIC
    u2hts_core
    PkgConfig::USBGX
)

target_include_directories(U2HTS_Linux PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)   