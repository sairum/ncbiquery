cmake_minimum_required(VERSION 3.16)

project(ncbiquery LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core Network)
find_package(Qt6 REQUIRED COMPONENTS Core Network)

add_executable(ncbiquery
  main.cpp
  gbquery.h gbquery.cpp
  esearch.h esearch.cpp
  efetch.h efetch.cpp
)
target_link_libraries(ncbiquery Qt6::Core Qt6::Network)

include(GNUInstallDirs)
install(TARGETS ncbiquery
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
