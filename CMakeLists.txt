cmake_minimum_required(VERSION 3.16)

project(unitree_lidar_cli C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Sources
set(SOURCES
    src/main.c
    src/cli.c
    src/parser.c
    src/ring_buffer.c
    src/serial_port.c
)

# Headers
include_directories(include)

add_executable(unitree_lidar_cli ${SOURCES})

# Platform specific settings
if(WIN32)

    target_compile_definitions(unitree_lidar_cli PRIVATE PLATFORM_WINDOWS)

else()

    target_compile_definitions(unitree_lidar_cli PRIVATE PLATFORM_LINUX)

    find_package(Threads REQUIRED)
    target_link_libraries(unitree_lidar_cli PRIVATE Threads::Threads)

endif()
