cmake_minimum_required(VERSION 3.0)
project(sign_detecc)

find_package(OpenCV REQUIRED)

set(CMAKE_CXX_STANDARD 14)

include_directories(
        include
)

set(SOURCE_FILES main.cpp Line.cpp LinesProcessor.cpp Utils.cpp)
add_executable(sign_detecc ${SOURCE_FILES})

target_link_libraries(sign_detecc ${OpenCV_LIBS} )