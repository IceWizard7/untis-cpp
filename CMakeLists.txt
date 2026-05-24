cmake_minimum_required(VERSION 4.1)

project(Untis VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Dependencies
add_subdirectory(external/IXWebSocket)

set(CPR_CURL_USE_LIBPSL OFF CACHE BOOL "" FORCE)
add_subdirectory(external/cpr)
add_subdirectory(external/json)

# Library
add_library(Untis STATIC
        src/config.cpp
        src/logging.cpp
        src/objects.cpp
        src/render.cpp
)

# Public headers for consumers
target_include_directories(Untis PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link dependencies
target_link_libraries(Untis PUBLIC
        ixwebsocket
        cpr::cpr
        nlohmann_json::nlohmann_json
)

install(TARGETS Untis DESTINATION lib)
install(DIRECTORY src/ DESTINATION include
        FILES_MATCHING PATTERN "*.hpp")
