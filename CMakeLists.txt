cmake_minimum_required(VERSION 3.23)
project(AwsQueue)

include_directories(/usr/local/include)

find_package(AWSSDK REQUIRED COMPONENTS sqs)

add_executable(AwsQueue AwsQueue.cpp)

target_compile_features(AwsQueue PUBLIC cxx_std_11)
target_link_libraries(AwsQueue ${AWSSDK_LINK_LIBRARIES})

