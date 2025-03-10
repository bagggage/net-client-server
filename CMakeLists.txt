cmake_minimum_required(VERSION 3.5.0)
project(net-labs VERSION 0.1.0 LANGUAGES C CXX)

include_directories(src)

aux_source_directory(src/core CORE_SOURCES)
aux_source_directory(src/client CLIENT_SOURCES)
aux_source_directory(src/server SERVER_SOURCES)

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_executable(client
    ${CLIENT_SOURCES}
    ${CORE_SOURCES}
)

add_executable(server
    ${SERVER_SOURCES}
    ${CORE_SOURCES}
)