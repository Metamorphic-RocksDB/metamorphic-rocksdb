#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>
#include <string>

#include <rocksdb/db.h>

std::string escapeJSON(const std::string& s) {
    std::string escaped = "";
    for (char c : s) {
        switch (c) {
            case '"':  escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '/':  escaped += "\\/"; break;
            case '\b': escaped += "\\b"; break;
            case '\f': escaped += "\\f"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default:   escaped += c; break;
        }
    }
    return escaped;
}


std::vector<std::vector<std::string>> parseCSV(const std::string& filename) {
    std::vector<std::vector<std::string>> data;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return data;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::vector<std::string> row;
        std::stringstream ss(line);
        std::string value;
        bool insideQuotes = false;

        for (char c : line) {
            if (c == '"' && (value.empty() || value.back() != '\\')) {
                insideQuotes = !insideQuotes;
            } else if (c == ',' && !insideQuotes) {
                // End of a field
                row.push_back(value);
                value.clear();
            } else {
                value += c;
            }
        }
        row.push_back(value); // Add last value

        data.push_back(row);
    }

    file.close();
    return data;
}

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
 
    if(argc!=2) {
        std::cout << "please the supply the key prefix\n";
        exit(1);
    }

    rocksdb::DB* db;
    rocksdb::Options options;

    options.create_if_missing = true;

    // Open DB
    rocksdb::ExternalCompactionService *cs;
    startExternalCompactionService(&options, &cs);
    rocksdb::Status status = rocksdb::DB::Open(options, "/home/ubuntu/s3fuse/conn.db", &db);
    std::this_thread::sleep_for(std::chrono::seconds(3));
    assert(status.ok());

 
   std::string filename = "rows.csv";
   std::vector<std::vector<std::string>> csvData = parseCSV(filename);

   std::string prefix(argv[1]);

   auto headers = csvData[0];
   csvData.erase(csvData.begin());

    // Example of how to print the data
    for (const auto& row : csvData) {
        std::string jsonRow = "{";
        for(int columnIndex=0; columnIndex<headers.size(); columnIndex++) {        
            jsonRow += "\"" + escapeJSON(headers[columnIndex]) + "\": \"" + escapeJSON(row[columnIndex]) + "\"";
            if (++columnIndex < headers.size()) {
                jsonRow += ", ";
            }
        }
        jsonRow += " \"last\":\"foo\"}";
        std::cout << jsonRow <<  std::endl;
        std::string key = prefix+"-"+row[0]+"-"+row[2]+"-"+row[3];
        status = db->Put(rocksdb::WriteOptions(), key, jsonRow);
        assert(status.ok());
        //std::this_thread::sleep_for(std::chrono::milliseconds(100));
        PrintSSTableCounts(db);
    }

    return 0;
}

