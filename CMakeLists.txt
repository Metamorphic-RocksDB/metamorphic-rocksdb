cmake_minimum_required(VERSION 3.16)

project(metarocks)

set(CMAKE_CXX_STANDARD 17)

# RocksDB Configuration
set(ROCKSDB_INCLUDE_DIR /home/ubuntu/cs839/rocksdb/include)
set(ROCKSDB_LIBRARY /home/ubuntu/cs839/rocksdb/build/librocksdb.a)
include_directories(${ROCKSDB_INCLUDE_DIR})

# Include AWS SDK
find_package(AWSSDK REQUIRED COMPONENTS sqs)
find_package(Arrow)

add_executable(metarocks src/main.cpp)
target_link_libraries(metarocks PRIVATE ${ROCKSDB_LIBRARY} snappy lz4 zstd bz2 pthread)
target_link_libraries(metarocks PRIVATE aws-cpp-sdk-core aws-cpp-sdk-sqs)
target_link_libraries(metarocks PRIVATE arrow parquet)



