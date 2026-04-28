cmake_minimum_required(VERSION 4.1)
project(Untis)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(external/IXWebSocket)

set(CPR_CURL_USE_LIBPSL OFF CACHE BOOL "" FORCE)
add_subdirectory(external/cpr)
add_subdirectory(external/json)

add_executable(Untis main.cpp
        config.cpp
        logging.cpp
        objects.cpp
        config.hpp
        exceptions.hpp
        logging.hpp
        objects.hpp
        render.cpp
        render.hpp
)

target_include_directories(Untis PRIVATE
        external/boost/libs/uuid/include
        external/boost/libs/config/include
        external/boost/libs/assert/include
        external/boost/libs/core/include
        external/boost/libs/static_assert/include
        external/boost/libs/throw_exception/include
        external/boost/libs/type_traits/include
)

target_link_libraries(Untis PRIVATE
        ixwebsocket
        cpr::cpr
        nlohmann_json::nlohmann_json
)
