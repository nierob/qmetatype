cmake_minimum_required(VERSION 3.12)

find_package (Qt5Core CONFIG REQUIRED)
find_package (Qt5Test CONFIG REQUIRED)
set(CMAKE_AUTOMOC ON)

include_directories (${METATYPE_SOURCE_DIR}/src)
add_executable(convert main.cpp)
target_link_libraries(convert Metatype Qt5::Core Qt5::Test )

