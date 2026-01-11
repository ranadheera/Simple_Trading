add_library(market MarketState.cpp Liquidity.cpp L2Book.cpp)

target_include_directories(market PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(market PUBLIC util message)