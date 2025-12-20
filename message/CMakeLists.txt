add_library(message Message.cpp)

target_include_directories(message PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(message PUBLIC util)