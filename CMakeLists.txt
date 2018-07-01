cmake_minimum_required(VERSION 3.0)
project(sign_detecc)

set(CMAKE_MODULE_PATH)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)

find_package(TensorFlow REQUIRED)
find_package(OpenCV REQUIRED)
find_package(Eigen REQUIRED)
find_package(Protobuf REQUIRED)


set(CMAKE_CXX_STANDARD 11)


SET(GCC_COVERAGE_LINK_FLAGS    "-ltensorflow_all")

include_directories(sign_detecc
        include
        ${Eigen_INCLUDE_DIRS}
        ${Protobuf_INCLUDE_DIRS}
        ${TensorFlow_INCLUDE_DIRS}
        include/public)


set(SOURCE_FILES main.cpp src/TrainingData.cpp src/SignIdentifier.cpp src/SignPlace.cpp src/SignClassifier.cpp src/SignClassifier.h)
add_executable(sign_detecc ${SOURCE_FILES})
target_link_libraries(sign_detecc
        ${TensorFlow_LIBRARIES}
        ${OpenCV_LIBS}
        ${Protobuf_LIBRARIES}
        )



# tests
#add_executable(unitTest tests/SignPlaceTest.cpp tests/TestMain.cpp src/SignPlace.cpp)
#target_link_libraries(unitTest ${OpenCV_LIBS} )