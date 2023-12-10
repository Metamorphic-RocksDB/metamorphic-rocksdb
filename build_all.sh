git clone https://github.com/facebook/rocksdb.git
cd rocksdb
mkdir build
cd build
cmake ..
make -j 8
cd ../..

git clone https://github.com/apache/arrow.git
cd arrow
mkdir build
cd build
cmake .. -DARROW_PARQUET=ON
make -j 8
make install
cd ../..

git clone --recurse-submodules https://github.com/aws/aws-sdk-cpp
cd aws-sdk-cpp/
mkdir sdk_build
cd sdk_build/
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/usr/local/ -DCMAKE_INSTALL_PREFIX=/usr/local/  -DBUILD_ONLY="sqs"
make -j 8
make install

cd ../..
mkdir build
cd build
cmake ..
make
./metarocks
