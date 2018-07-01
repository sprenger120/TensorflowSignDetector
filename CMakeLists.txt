cmake_minimum_required(VERSION 3.0)
project(sign_detecc)

find_package(OpenCV REQUIRED)

set(CMAKE_CXX_STANDARD 14)

include_directories(sign_detecc include)
set(SOURCE_FILES main.cpp src/TrainingData.cpp src/SignIdentifier.cpp src/SignPlace.cpp include/Types.h)
add_executable(sign_detecc ${SOURCE_FILES})
target_link_libraries(sign_detecc
        ${OpenCV_LIBS}
        pthread
        )



# tests
add_executable(unitTest tests/SignPlaceTest.cpp tests/TestMain.cpp src/SignPlace.cpp)
target_link_libraries(unitTest ${OpenCV_LIBS} )