add_library(util CsvParse.cpp CommonUtils.cpp MarketTick.cpp)

target_include_directories(util PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)