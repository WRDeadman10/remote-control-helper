cmake_minimum_required(VERSION 3.15)
project(RemoteControlHelper VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /WX- /permissive- /EHsc)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -DWIN32_LEAN_AND_MEAN -DNOMINMAX)
endif()

include_directories(include)

set(SOURCES
    src/main.cpp
    src/InputInjector.cpp
    src/KeycodeMapper.cpp
    src/SafetyManager.cpp
    src/LoopbackServer.cpp
)

set(HEADERS
    include/InputInjector.h
    include/InputValidator.h
    include/KeycodeMapper.h
    include/SafetyManager.h
    include/LoopbackServer.h
)

add_executable(remote-control-helper ${SOURCES} ${HEADERS})

target_link_libraries(remote-control-helper PRIVATE
    ws2_32
    user32
)

# Test executable for unit testing input mapping and validation
add_executable(helper-tests
    tests/test_main.cpp
    src/KeycodeMapper.cpp
    src/SafetyManager.cpp
    src/InputInjector.cpp
)

target_link_libraries(helper-tests PRIVATE
    user32
)
