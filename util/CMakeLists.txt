add_library(util CsvParse.cpp MarketTick.cpp)

target_include_directories(util PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)