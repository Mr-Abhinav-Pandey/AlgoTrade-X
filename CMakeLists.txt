cmake_minimum_required(VERSION 3.10)
project(AlgoTradeX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(AlgoTradeX
    src/fetcher.cpp
    src/parser.cpp
    src/main.cpp
    src/websocket_feed.cpp
)

target_include_directories(AlgoTradeX PRIVATE ${CMAKE_SOURCE_DIR}/src)

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