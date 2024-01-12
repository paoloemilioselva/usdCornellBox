cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(TARGET "usdCornellBox")
add_definitions(-DNOMINMAX)
add_definitions(-DTBB_USE_DEBUG)

project(
    ${TARGET}
    DESCRIPTION "Procedural CornellBox for your models"
    LANGUAGES CXX C
)

set(CMAKE_POSITION_INDEPENDENT_CODE ON) # fPIC

# Add OpenGL
find_package(OpenGL REQUIRED)

# Add jsoncpp
#set(JSONCPP_WITH_CMAKE_PACKAGE 1)
#set(JSONCPP_WITH_TESTS OFF CACHE BOOL "" FORCE)
#set(JSONCPP_WITH_EXAMPLE OFF CACHE BOOL "" FORCE)
set(JSONCPP_WITH_TESTS OFF CACHE BOOL "Enable jsoncpp tests" FORCE)
add_subdirectory( "../jsoncpp" "../jsoncpp" )
include_directories( "../jsoncpp/include" )

# Add GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory( "../glfw" "../glfw" )

# Add OpenUSD
set(USD_VERSION 23.11)
set(USD_ROOT "C:\\dev\\usd-${USD_VERSION}" )
set(USD_ROOT_FOR_BATCH "%CURRENT_FOLDER%usd-${USD_VERSION}")
include(cmake/FindPixarUsd.cmake)

# Add TBB
include(cmake/FindTBB.cmake)

add_executable( 
    ${TARGET}
    main.cpp
)

target_compile_options(
        ${TARGET}
        PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /wd4273 /Zi /experimental:external /external:W0 /D> 
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:RelWithDebInfo>>:/Ob0 /Od>
        )

target_link_options(
        ${TARGET}
        PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/ignore:4217 /ignore:4049> 
        )

target_link_directories(
    ${TARGET}
    PRIVATE
    ${USD_LIBRARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/../jsoncpp/src/lib_json
)

target_link_libraries(
    ${TARGET}
    PUBLIC
    OpenGL::GL
    glfw
    jsoncpp
    ${USD_LIBS}
)

install(DIRECTORY ${USD_ROOT}/bin DESTINATION bin/usd-${USD_VERSION})
install(DIRECTORY ${USD_ROOT}/include DESTINATION bin/usd-${USD_VERSION})
install(DIRECTORY ${USD_ROOT}/lib DESTINATION bin/usd-${USD_VERSION})
install(DIRECTORY ${USD_ROOT}/plugin DESTINATION bin/usd-${USD_VERSION})
install(DIRECTORY ${USD_ROOT}/share DESTINATION bin/usd-${USD_VERSION})

install(TARGETS ${TARGET} RUNTIME DESTINATION bin)
install(FILES ${CMAKE_SOURCE_DIR}/plugInfo.json DESTINATION bin/usd-extra/plugin/usd)

configure_file( run_simpleusdview.bat run_simpleusdview.bat )
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/run_simpleusdview.bat DESTINATION bin)
