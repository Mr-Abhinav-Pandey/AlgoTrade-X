cmake_minimum_required(VERSION 3.10)
project(AlgoTradeX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Unicode support (matching Code::Blocks settings)
add_definitions(-DUNICODE -D_UNICODE)

# Source files
# Note: variantforge_engine.cpp is #included in main.cpp
# so it is NOT listed here as a separate compilation unit
add_executable(AlgoTradeX
    src/fetcher.cpp
    src/parser.cpp
    src/main.cpp
)

# Windows libraries
if(WIN32)
    target_link_libraries(AlgoTradeX
        wininet      # HTTPS requests (Binance API)
        ws2_32       # Winsock (networking base)
    )
endif()

# Compiler flags (matching Code::Blocks settings)
target_compile_options(AlgoTradeX PRIVATE
    -O2
    -Wall
    -fexceptions
)

# Output binary to bin/ folder
set_target_properties(AlgoTradeX PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)