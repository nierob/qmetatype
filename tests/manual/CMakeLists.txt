cmake_minimum_required(VERSION 3.12)

find_package (Qt5Core CONFIG REQUIRED)
find_package (Qt5Test CONFIG REQUIRED)

include_directories (${METATYPE_SOURCE_DIR}/src)
add_executable(manual main.cpp)
target_link_libraries(manual Metatype Qt5::Core Qt5::Test dl)

set(CMAKE_AUTOMOC ON)
