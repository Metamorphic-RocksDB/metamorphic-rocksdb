#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>
#include <string>

#include <rocksdb/db.h>


int main(int argc, char** argv) {
 
    srand(time(NULL));

    rocksdb::DB* db;
    rocksdb::Options options;

    options.create_if_missing = true;
    rocksdb::Status status = rocksdb::DB::Open(options, "/home/ubuntu/nofuse.db", &db);
    assert(status.ok());

 
   std::ifstream file("/home/ubuntu/rows.json"); // Replace 'example.txt' with your file name

    if (!file.is_open()) {
        std::cerr << "Unable to open file\n";
        return 1;
    }

    int count = 0;
    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss;
        ss << rand();
        std::string key = ss.str();
        status = db->Put(rocksdb::WriteOptions(), key, line);
        assert(status.ok());
        
        if(++count % 100000 == 0) {
            std::cout << count << std::endl;
        }
    }
   
    return 0;
}

