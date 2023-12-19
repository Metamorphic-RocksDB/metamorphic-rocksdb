#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>
#include <string>

#include <rocksdb/db.h>



std::string kDBPath = "/home/ubuntu/s3fuse/conn.db";
std::string kDBCompactionOutputPath = "/home/ubuntu/s3fuse/conn.db/output";
std::string kCompactionRequestQueueUrl = "https://sqs.us-east-2.amazonaws.com/848490464384/request.fifo";
std::string kCompactionResponseQueueUrl ="https://sqs.us-east-2.amazonaws.com/848490464384/response.fifo";

void startExternalCompactionService(
    rocksdb::Options *db_options,
    rocksdb::ExternalCompactionService **cs /*output*/) {
  rocksdb::Options compactor_options;
  {
    rocksdb::Options options;
    options.env = db_options->env;
    options.create_if_missing = false; // secondary
    options.fail_if_options_file_error = true;
    compactor_options = options;
  }

  std::shared_ptr<rocksdb::ExternalCompactionService> compaction_service;
  // ExternalCompactionService* cs; // inherit from output param
  {
    std::string db_path = kDBPath;
    std::shared_ptr<rocksdb::Statistics> compactor_statistics =
        ROCKSDB_NAMESPACE::CreateDBStatistics();
    std::vector<std::shared_ptr<rocksdb::EventListener>> listeners;
    std::vector<std::shared_ptr<rocksdb::TablePropertiesCollectorFactory>>
        table_properties_collector_factories;

    compaction_service =
        std::make_shared<ROCKSDB_NAMESPACE::ExternalCompactionService>(
            db_path, compactor_options, compactor_statistics, listeners,
            table_properties_collector_factories);
    *cs = compaction_service.get();
  }

  db_options->compaction_service = compaction_service;
}

static void PrintSSTableCounts(rocksdb::DB *db) {
  std::vector<rocksdb::LiveFileMetaData> metadata;
  db->GetLiveFilesMetaData(&metadata);
  std::cout << "SSTable Counts: " << metadata.size() << std::endl;

  rocksdb::ColumnFamilyMetaData meta;
  db->GetColumnFamilyMetaData(&meta);

  std::vector<std::vector<rocksdb::SstFileMetaData>::size_type>
      sstable_files_by_level;
  for (const auto &level : meta.levels) {
    sstable_files_by_level.push_back(level.files.size());
  }

  std::cout << "SSTable Files by Level: ";
  for (std::vector<int>::size_type i = 0; i < sstable_files_by_level.size();
       i++) {
    std::cout << sstable_files_by_level[i] << " ";
  }
  std::cout << std::endl;
}

int main(int argc, char** argv) {
 
    srand(time(NULL));

    rocksdb::DB* db;
    rocksdb::Options options;

    options.create_if_missing = true;

    // Open DB
    rocksdb::ExternalCompactionService *cs;
    startExternalCompactionService(&options, &cs);
    rocksdb::Status status = rocksdb::DB::Open(options, "/home/ubuntu/s3fuse/conn.db", &db);
    std::this_thread::sleep_for(std::chrono::seconds(3));
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

    PrintSSTableCounts(db);
    
    return 0;
}

