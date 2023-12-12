#include <iostream>
#include <fstream>
#include <sstream>
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

int main(int argc, char** argv) {
 
    if(argc!=2) {
        std::cout << "please the supply the key prefix\n";
        exit(1);
    }

    rocksdb::DB* db;
    rocksdb::Options options;

    options.create_if_missing = true;

    // Open DB
    rocksdb::Status status = rocksdb::DB::Open(options, "test.db", &db);
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
        jsonRow += "}";
        // std::cout << jsonRow <<  std::endl;
        std::string key = prefix+"-"+row[0]+"-"+row[2]+"-"+row[3];
        status = db->Put(rocksdb::WriteOptions(), key, jsonRow);
        assert(status.ok());
 
    }

    return 0;
}

