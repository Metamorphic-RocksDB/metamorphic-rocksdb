// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements. See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership. The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License. You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the License for the
// specific language governing permissions and limitations
// under the License.

#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <utility>
#include <iostream>
#include <string>

#include <aws/core/Aws.h>
#include <aws/core/utils/crypto/Sha256.h>
#include <aws/core/utils/HashingUtils.h>
#include <aws/core/utils/Outcome.h>
#include <aws/sqs/SQSClient.h>
#include <aws/sqs/model/ReceiveMessageRequest.h>
#include <aws/sqs/model/SendMessageRequest.h>
#include <aws/sqs/model/DeleteMessageRequest.h>

#include <rocksdb/db.h>

#include "arrow/io/file.h"
#include "parquet/exception.h"
#include "parquet/stream_reader.h"
#include "parquet/stream_writer.h"


using std::string;
using std::cout;
using std::endl;

// This file gives an example of how to use the parquet::StreamWriter
// and parquet::StreamReader classes.
// It shows writing/reading of the supported types as well as how a
// user-defined type can be handled.

template <typename T>
using optional = parquet::StreamReader::optional<T>;

// Example of a user-defined type to be written to/read from Parquet
// using C++ input/output operators.
class UserTimestamp {
 public:
  UserTimestamp() = default;

  explicit UserTimestamp(const std::chrono::microseconds v) : ts_{v} {}

  bool operator==(const UserTimestamp& x) const { return ts_ == x.ts_; }

  void dump(std::ostream& os) const {
    const auto t = static_cast<std::time_t>(
        std::chrono::duration_cast<std::chrono::seconds>(ts_).count());
    os << std::put_time(std::gmtime(&t), "%Y%m%d-%H%M%S");
  }

  void dump(parquet::StreamWriter& os) const { os << ts_; }

 private:
  std::chrono::microseconds ts_;
};

std::ostream& operator<<(std::ostream& os, const UserTimestamp& v) {
  v.dump(os);
  return os;
}

parquet::StreamWriter& operator<<(parquet::StreamWriter& os, const UserTimestamp& v) {
  v.dump(os);
  return os;
}

parquet::StreamReader& operator>>(parquet::StreamReader& os, UserTimestamp& v) {
  std::chrono::microseconds ts;

  os >> ts;
  v = UserTimestamp{ts};

  return os;
}

std::shared_ptr<parquet::schema::GroupNode> GetSchema() {
  parquet::schema::NodeVector fields;

  fields.push_back(parquet::schema::PrimitiveNode::Make(
      "string_field", parquet::Repetition::OPTIONAL, parquet::Type::BYTE_ARRAY,
      parquet::ConvertedType::UTF8));

  fields.push_back(parquet::schema::PrimitiveNode::Make(
      "char_field", parquet::Repetition::REQUIRED, parquet::Type::FIXED_LEN_BYTE_ARRAY,
      parquet::ConvertedType::NONE, 1));

  fields.push_back(parquet::schema::PrimitiveNode::Make(
      "char[4]_field", parquet::Repetition::REQUIRED, parquet::Type::FIXED_LEN_BYTE_ARRAY,
      parquet::ConvertedType::NONE, 4));

  fields.push_back(parquet::schema::PrimitiveNode::Make(
      "int8_field", parquet::Repetition::REQUIRED, parquet::Type::INT32,
      parquet::ConvertedType::INT_8));

  fields.push_back(parquet::schema::PrimitiveNode::Make(
      "uint16_field", parquet::Repetition::REQUIRED, parquet::Type::INT32,
      parquet::ConvertedType::UINT_16));

  fields.push_back(parquet::schema::PrimitiveNode::Make(
      "int32_field", parquet::Repetition::REQUIRED, parquet::Type::INT32,
      parquet::ConvertedType::INT_32));

  fields.push_back(parquet::schema::PrimitiveNode::Make(
      "uint64_field", parquet::Repetition::OPTIONAL, parquet::Type::INT64,
      parquet::ConvertedType::UINT_64));

  fields.push_back(parquet::schema::PrimitiveNode::Make(
      "double_field", parquet::Repetition::REQUIRED, parquet::Type::DOUBLE,
      parquet::ConvertedType::NONE));

  // User defined timestamp type.
  fields.push_back(parquet::schema::PrimitiveNode::Make(
      "timestamp_field", parquet::Repetition::REQUIRED, parquet::Type::INT64,
      parquet::ConvertedType::TIMESTAMP_MICROS));

  fields.push_back(parquet::schema::PrimitiveNode::Make(
      "chrono_milliseconds_field", parquet::Repetition::REQUIRED, parquet::Type::INT64,
      parquet::ConvertedType::TIMESTAMP_MILLIS));

  return std::static_pointer_cast<parquet::schema::GroupNode>(
      parquet::schema::GroupNode::Make("schema", parquet::Repetition::REQUIRED, fields));
}

struct TestData {
  static const int num_rows = 2000;

  static void init() { std::time(&ts_offset_); }

  static optional<std::string> GetOptString(const int i) {
    if (i % 2 == 0) return {};
    return "Str #" + std::to_string(i);
  }
  static std::string_view GetStringView(const int i) {
    static std::string string;
    string = "StringView #" + std::to_string(i);
    return std::string_view(string);
  }
  static const char* GetCharPtr(const int i) {
    static std::string string;
    string = "CharPtr #" + std::to_string(i);
    return string.c_str();
  }
  static char GetChar(const int i) { return i & 1 ? 'M' : 'F'; }
  static int8_t GetInt8(const int i) { return static_cast<int8_t>((i % 256) - 128); }
  static uint16_t GetUInt16(const int i) { return static_cast<uint16_t>(i); }
  static int32_t GetInt32(const int i) { return 3 * i - 17; }
  static optional<uint64_t> GetOptUInt64(const int i) {
    if (i % 11 == 0) return {};
    return (1ull << 40) + i * i + 101;
  }
  static double GetDouble(const int i) { return 6.62607004e-34 * 3e8 * i; }
  static UserTimestamp GetUserTimestamp(const int i) {
    return UserTimestamp{std::chrono::microseconds{(ts_offset_ + 3 * i) * 1000000 + i}};
  }
  static std::chrono::milliseconds GetChronoMilliseconds(const int i) {
    return std::chrono::milliseconds{(ts_offset_ + 3 * i) * 1000ull + i};
  }

  static char char4_array[4];

 private:
  static std::time_t ts_offset_;
};

char TestData::char4_array[] = "XYZ";
std::time_t TestData::ts_offset_;

void WriteParquetFile() {
  std::shared_ptr<arrow::io::FileOutputStream> outfile;

  PARQUET_ASSIGN_OR_THROW(
      outfile, arrow::io::FileOutputStream::Open("parquet-stream-api-example.parquet"));

  parquet::WriterProperties::Builder builder;

#if defined ARROW_WITH_BROTLI
  builder.compression(parquet::Compression::BROTLI);
#elif defined ARROW_WITH_ZSTD
  builder.compression(parquet::Compression::ZSTD);
#endif

  parquet::StreamWriter os{
      parquet::ParquetFileWriter::Open(outfile, GetSchema(), builder.build())};

  os.SetMaxRowGroupSize(1000);

  for (auto i = 0; i < TestData::num_rows; ++i) {
    // Output string using 3 different types: std::string, std::string_view and
    // const char *.
    switch (i % 3) {
      case 0:
        os << TestData::GetOptString(i);
        break;
      case 1:
        os << TestData::GetStringView(i);
        break;
      case 2:
        os << TestData::GetCharPtr(i);
        break;
    }
    os << TestData::GetChar(i);
    switch (i % 2) {
      case 0:
        os << TestData::char4_array;
        break;
      case 1:
        os << parquet::StreamWriter::FixedStringView{TestData::GetCharPtr(i), 4};
        break;
    }
    os << TestData::GetInt8(i);
    os << TestData::GetUInt16(i);
    os << TestData::GetInt32(i);
    os << TestData::GetOptUInt64(i);
    os << TestData::GetDouble(i);
    os << TestData::GetUserTimestamp(i);
    os << TestData::GetChronoMilliseconds(i);
    os << parquet::EndRow;

    if (i == TestData::num_rows / 2) {
      os << parquet::EndRowGroup;
    }
  }
  std::cout << "Parquet Stream Writing complete." << std::endl;
}

void ReadParquetFile() {
  std::shared_ptr<arrow::io::ReadableFile> infile;

  PARQUET_ASSIGN_OR_THROW(
      infile, arrow::io::ReadableFile::Open("parquet-stream-api-example.parquet"));

  parquet::StreamReader os{parquet::ParquetFileReader::Open(infile)};

  optional<std::string> opt_string;
  char ch;
  char char_array[4];
  int8_t int8;
  uint16_t uint16;
  int32_t int32;
  optional<uint64_t> opt_uint64;
  double d;
  UserTimestamp ts_user;
  std::chrono::milliseconds ts_ms;
  int i;

  for (i = 0; !os.eof(); ++i) {
    os >> opt_string;
    os >> ch;
    os >> char_array;
    os >> int8;
    os >> uint16;
    os >> int32;
    os >> opt_uint64;
    os >> d;
    os >> ts_user;
    os >> ts_ms;
    os >> parquet::EndRow;

    if (0) {
      // For debugging.
      std::cout << "Row #" << i << std::endl;

      std::cout << "string[";
      if (opt_string) {
        std::cout << *opt_string;
      } else {
        std::cout << "N/A";
      }
      std::cout << "] char[" << ch << "] charArray[" << char_array << "] int8["
                << int(int8) << "] uint16[" << uint16 << "] int32[" << int32;
      std::cout << "] uint64[";
      if (opt_uint64) {
        std::cout << *opt_uint64;
      } else {
        std::cout << "N/A";
      }
      std::cout << "] double[" << d << "] tsUser[" << ts_user << "] tsMs["
                << ts_ms.count() << "]" << std::endl;
    }
    // Check data.
    switch (i % 3) {
      case 0:
        assert(opt_string == TestData::GetOptString(i));
        break;
      case 1:
        assert(*opt_string == TestData::GetStringView(i));
        break;
      case 2:
        assert(*opt_string == TestData::GetCharPtr(i));
        break;
    }
    assert(ch == TestData::GetChar(i));
    switch (i % 2) {
      case 0:
        assert(0 == std::memcmp(char_array, TestData::char4_array, sizeof(char_array)));
        break;
      case 1:
        assert(0 == std::memcmp(char_array, TestData::GetCharPtr(i), sizeof(char_array)));
        break;
    }
    assert(int8 == TestData::GetInt8(i));
    assert(uint16 == TestData::GetUInt16(i));
    assert(int32 == TestData::GetInt32(i));
    assert(opt_uint64 == TestData::GetOptUInt64(i));
    assert(std::abs(d - TestData::GetDouble(i)) < 1e-6);
    assert(ts_user == TestData::GetUserTimestamp(i));
    assert(ts_ms == TestData::GetChronoMilliseconds(i));
  }
  assert(TestData::num_rows == i);

  std::cout << "Parquet Stream Reading complete." << std::endl;
}


string waitForResponse(const string& queueUrl) {
    Aws::SDKOptions options;
    Aws::InitAPI(options);
    string result = "";
    {
        Aws::Client::ClientConfiguration clientConfig;
        clientConfig.region = Aws::Region::US_EAST_2; // Set the region to Ohio

        Aws::SQS::SQSClient sqs(clientConfig);
  
        // Create a receive message request
        Aws::SQS::Model::ReceiveMessageRequest receive_request;
        receive_request.SetQueueUrl(queueUrl);
        receive_request.SetMaxNumberOfMessages(1); // Max number of messages to receive
        receive_request.SetVisibilityTimeout(30); // Visibility timeout
        receive_request.SetWaitTimeSeconds(20); // Long polling wait time

        // Receive the message
        auto receive_outcome = sqs.ReceiveMessage(receive_request);

        if (receive_outcome.IsSuccess()) {
            const auto &messages = receive_outcome.GetResult().GetMessages();
            if (!messages.empty()) {
                for (const auto &message: messages) {
                    result = message.GetBody();

                    // After processing, delete the message from the queue
                    Aws::SQS::Model::DeleteMessageRequest delete_request;
                    delete_request.SetQueueUrl(queueUrl);
                    delete_request.SetReceiptHandle(message.GetReceiptHandle());
                    auto delete_outcome = sqs.DeleteMessage(delete_request);
                    if (!delete_outcome.IsSuccess()) {
                        std::cerr << "Error deleting message: " << delete_outcome.GetError().GetMessage() << endl;
                    }
                }
            } else {
                cout << "No messages to process." << endl;
            }
        } else {
            std::cerr << "Error receiving messages: " << receive_outcome.GetError().GetMessage() << endl;
        }
    }
    Aws::ShutdownAPI(options);
    return result;
}

void sendMessage(const string& message,const string& queueUrl) {
    Aws::SDKOptions options;
    Aws::InitAPI(options);
    {
        auto now = std::chrono::high_resolution_clock::now();

        // Convert the time point to a duration since the epoch
        auto duration_since_epoch = now.time_since_epoch();

        // Convert the duration to a specific unit (e.g., nanoseconds)
        auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration_since_epoch).count();

        std::stringstream ss;
        ss << nanoseconds;
        string nanoStr = ss.str();

        // Hash the input string
        Aws::Utils::Crypto::Sha256 sha256;
        auto hashBytes = sha256.Calculate(message + nanoStr);
        auto hash = Aws::Utils::HashingUtils::HexEncode(hashBytes.GetResult());

        Aws::Client::ClientConfiguration clientConfig;
        clientConfig.region = Aws::Region::US_EAST_2; // Set the region to Ohio

        Aws::SQS::SQSClient sqs(clientConfig);

        while (1) {
            Aws::SQS::Model::SendMessageRequest smReq;
            smReq.SetQueueUrl(queueUrl);
            smReq.SetMessageGroupId("group");
            smReq.SetMessageDeduplicationId(hash);
            smReq.SetMessageBody(message);

            auto sm_out = sqs.SendMessage(smReq);
            if (sm_out.IsSuccess()) {
                return;
            } else {
                std::cerr << "Error sending message: " << sm_out.GetError().GetMessage() << endl;
            }
        }
    }
    Aws::ShutdownAPI(options);

}

void rockscalls() {
    rocksdb::DB* db;
    rocksdb::Options options;

    options.create_if_missing = true;

    // Open DB
    rocksdb::Status status = rocksdb::DB::Open(options, "test.db", &db);
    assert(status.ok());

    // Put key-value
    status = db->Put(rocksdb::WriteOptions(), "key1", "value1");
    assert(status.ok());
    status = db->Put(rocksdb::WriteOptions(), "key2", "value2");
    assert(status.ok());

    // Get value
    std::string value;
    status = db->Get(rocksdb::ReadOptions(), "key1", &value);
    assert(status.ok());
    std::cout << "Got value: " << value << std::endl;

    // Close DB
    delete db;
}


int main() {
    WriteParquetFile();
    ReadParquetFile();

    rockscalls();

    sendMessage("the message ggg", "https://sqs.us-east-2.amazonaws.com/848490464384/request.fifo");
    string msg = waitForResponse("https://sqs.us-east-2.amazonaws.com/848490464384/request.fifo");
    cout << "got back:" << msg << endl;

  return 0;
}
