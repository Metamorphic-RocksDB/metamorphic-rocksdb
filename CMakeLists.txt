cmake_minimum_required(VERSION 3.16)

project(metarocks)

set(CMAKE_CXX_STANDARD 17)

# RocksDB Configuration
set(ROCKSDB_INCLUDE_DIR /home/ubuntu/metamorphic-rocksdb/metarocks/include)
set(ROCKSDB_LIBRARY /home/ubuntu/metamorphic-rocksdb/metarocks/build/librocksdb.a)
include_directories(${ROCKSDB_INCLUDE_DIR})

# Include AWS SDK
find_package(AWSSDK REQUIRED COMPONENTS sqs)
find_package(Arrow)

add_executable(metarocks src/main.cpp)
target_link_libraries(metarocks PRIVATE ${ROCKSDB_LIBRARY} snappy lz4 zstd bz2 pthread)
target_link_libraries(metarocks PRIVATE aws-cpp-sdk-core aws-cpp-sdk-sqs)
target_link_libraries(metarocks PRIVATE arrow parquet)

add_executable(inserter src/inserter.cpp)
target_link_libraries(inserter PRIVATE ${ROCKSDB_LIBRARY} snappy lz4 zstd bz2 pthread)
target_link_libraries(inserter PRIVATE aws-cpp-sdk-core aws-cpp-sdk-sqs)
target_link_libraries(inserter PRIVATE arrow parquet)

add_executable(inserter2 src/inserter2.cpp)
target_link_libraries(inserter2 PRIVATE ${ROCKSDB_LIBRARY} snappy lz4 zstd bz2 pthread)
target_link_libraries(inserter2 PRIVATE aws-cpp-sdk-core aws-cpp-sdk-sqs)
target_link_libraries(inserter2 PRIVATE arrow parquet)


add_executable(inserter3 src/inserter3.cpp)
target_link_libraries(inserter3 PRIVATE ${ROCKSDB_LIBRARY} snappy lz4 zstd bz2 pthread)
target_link_libraries(inserter3 PRIVATE aws-cpp-sdk-core aws-cpp-sdk-sqs)
target_link_libraries(inserter3 PRIVATE arrow parquet)


add_executable(inserter4 src/inserter4.cpp)
target_link_libraries(inserter4 PRIVATE ${ROCKSDB_LIBRARY} snappy lz4 zstd bz2 pthread)
target_link_libraries(inserter4 PRIVATE aws-cpp-sdk-core aws-cpp-sdk-sqs)
target_link_libraries(inserter4 PRIVATE arrow parquet)

add_executable(inserter5 src/inserter5.cpp)
target_link_libraries(inserter5 PRIVATE ${ROCKSDB_LIBRARY} snappy lz4 zstd bz2 pthread)
target_link_libraries(inserter5 PRIVATE aws-cpp-sdk-core aws-cpp-sdk-sqs)
target_link_libraries(inserter5 PRIVATE arrow parquet)

add_executable(inserter6 src/inserter6.cpp)
target_link_libraries(inserter6 PRIVATE ${ROCKSDB_LIBRARY} snappy lz4 zstd bz2 pthread)
target_link_libraries(inserter6 PRIVATE aws-cpp-sdk-core aws-cpp-sdk-sqs)
target_link_libraries(inserter6 PRIVATE arrow parquet)


add_executable(dumper src/dumper.cpp)
target_link_libraries(dumper PRIVATE ${ROCKSDB_LIBRARY} snappy lz4 zstd bz2 pthread)
