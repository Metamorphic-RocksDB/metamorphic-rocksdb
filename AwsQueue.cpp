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
#include <aws/sqs/model/ListQueuesRequest.h>

using std::string;
using std::cout;
using std::endl;

string waitForResponse(const string& queueUrl) {
    Aws::SDKOptions options;
    Aws::InitAPI(options);
    string result = "";
    {
        Aws::Client::ClientConfiguration clientConfig;
        clientConfig.region = Aws::Region::US_EAST_2; // Set the region to Ohio

        Aws::SQS::SQSClient sqs(clientConfig);
        // Queue URL

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
                    std::cout << "Message received: " << result << std::endl;

                    // After processing, delete the message from the queue
                    Aws::SQS::Model::DeleteMessageRequest delete_request;
                    delete_request.SetQueueUrl(queueUrl);
                    delete_request.SetReceiptHandle(message.GetReceiptHandle());
                    auto delete_outcome = sqs.DeleteMessage(delete_request);
                    if (!delete_outcome.IsSuccess()) {
                        std::cerr << "Error deleting message: " << delete_outcome.GetError().GetMessage() << std::endl;
                    }
                }
            } else {
                std::cout << "No messages to process." << std::endl;
            }
        } else {
            std::cerr << "Error receiving messages: " << receive_outcome.GetError().GetMessage() << std::endl;
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
        Aws::String buffer(message + nanoStr);

        auto hashBytes = sha256.Calculate(buffer);
        auto hash = Aws::Utils::HashingUtils::HexEncode(hashBytes.GetResult());

        cout << "hash:" << hash << " nanos:" << nanoStr << endl;

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
                std::cerr << "Error sending message: " << sm_out.GetError().GetMessage() << std::endl;
            }
        }
    }
    Aws::ShutdownAPI(options);

}

int main() {

    sendMessage("the message", "https://sqs.us-east-2.amazonaws.com/848490464384/request.fifo");
    string msg = waitForResponse("https://sqs.us-east-2.amazonaws.com/848490464384/request.fifo");
    cout << "got back:" << msg << endl;
    return 0;
}
