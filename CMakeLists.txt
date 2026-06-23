cmake_minimum_required(VERSION 3.10)
project(AccessControlSystem LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Server executable
add_executable(server src/server.c)

# Client executable
add_executable(client src/client.c)

# Query tool executable
add_executable(query src/query.c)

# Platform-specific linking
if(WIN32)
    target_link_libraries(server ws2_32)
    target_link_libraries(client ws2_32)
else()
    target_link_libraries(server pthread)
    find_package(Threads REQUIRED)
    target_link_libraries(server Threads::Threads)
endif()

# Output directory
set_target_properties(server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
set_target_properties(client PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
set_target_properties(query PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
