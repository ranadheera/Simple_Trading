cmake_minimum_required(VERSION 3.10)

# set the project name
project(Simple_Trading)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# add the executable
add_executable(Simple_Trading main.cpp CsvParse.cpp Dispatcher.cpp L1Book.cpp L2Book.cpp SymbolIds.cpp Updaters.cpp MarketTick.cpp StrategyEngine.cpp)
