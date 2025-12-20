add_library(strategy StrategyEngine.cpp)

target_include_directories(strategy PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(strategy PUBLIC util)