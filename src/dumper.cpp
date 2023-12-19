#include <iostream>
#include <rocksdb/db.h>

int main(int argc, char** argv) {
    rocksdb::DB* db;
    rocksdb::Options options;
    options.create_if_missing = false;

    std::string dbName = argv[1];

    // Open the database
    rocksdb::Status status = rocksdb::DB::Open(options, dbName, &db);
    if (!status.ok()) {
        std::cerr << "Unable to open/create RocksDB: " << status.ToString() << std::endl;
        return 1;
    }

    // Create an iterator over the database
    rocksdb::Iterator* it = db->NewIterator(rocksdb::ReadOptions());

    // Iterate over each key-value pair in the database
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        std::cout << it->key().ToString() << ": " << it->value().ToString() << std::endl;
    }

    // Check for any errors found during the scan
    if (!it->status().ok()) {
        std::cerr << "An error was found during the scan: " << it->status().ToString() << std::endl;
    }

    // Clean up
    delete it;
    delete db;

    return 0;
}

