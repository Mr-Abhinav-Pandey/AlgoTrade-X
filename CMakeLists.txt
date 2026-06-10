cmake_minimum_required(VERSION 3.10)
project(AlgoTradeX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Optionally build with Boost.Beast websocket support
option(BUILD_WITH_BOOST_BEAST "Build with Boost.Beast websocket client" OFF)


add_executable(AlgoTradeX
    src/fetcher.cpp
    src/parser.cpp
    src/main.cpp
    src/websocket_feed.cpp
    src/realtime/binance_stream_parser.cpp
    src/realtime/websocket_client.cpp
)

target_include_directories(AlgoTradeX PRIVATE ${CMAKE_SOURCE_DIR}/src)

if(BUILD_WITH_BOOST_BEAST)
    find_package(Boost 1.66 REQUIRED COMPONENTS system thread)
    find_package(OpenSSL REQUIRED)
    find_package(nlohmann_json REQUIRED)
    target_compile_definitions(AlgoTradeX PRIVATE BUILD_WITH_BOOST_BEAST=1)
    target_sources(AlgoTradeX PRIVATE src/realtime/websocket_beast_client.cpp src/realtime/websocket_beast_client.h)
    target_link_libraries(AlgoTradeX PRIVATE ${Boost_LIBRARIES} OpenSSL::SSL OpenSSL::Crypto nlohmann_json::nlohmann_json)
    target_include_directories(AlgoTradeX PRIVATE ${Boost_INCLUDE_DIRS})
endif()

if(WIN32)
    target_compile_definitions(AlgoTradeX PRIVATE
        _WIN32_WINNT=0x0601
        UNICODE
        _UNICODE
    )
    target_link_libraries(AlgoTradeX PRIVATE wininet ws2_32)
endif()

target_compile_options(AlgoTradeX PRIVATE -O2 -Wall)

set_target_properties(AlgoTradeX PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)