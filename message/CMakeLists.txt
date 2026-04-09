add_library(message MessageBuilder.cpp FixTagValueReader.cpp MessageParser.cpp FixMessage.cpp)

target_include_directories(message PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(message PUBLIC util network)