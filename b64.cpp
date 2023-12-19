#include <aws/core/Aws.h>
#include <aws/core/utils/base64/Base64.h>
#include <aws/sqs/SQSClient.h>
#include <aws/sqs/model/SendMessageRequest.h>
#include <iostream>
#include <vector>

int main() {
    // Initialize AWS SDK
    Aws::SDKOptions options;
    Aws::InitAPI(options);

    // Your binary data
    std::vector<unsigned char> binaryData = {/* ...binary data... */};

    // Encode binary data to Base64
    Aws::Utils::Base64::Base64Encoder encoder;
    Aws::String base64Encoded = encoder.Encode(Aws::Utils::ByteBuffer(binaryData.data(), binaryData.size()));

    // Create SQS client
    Aws::SQS::SQSClient sqs;

    // Specify your queue URL
    Aws::String queueUrl = "YOUR_SQS_QUEUE_URL";

    // Create and configure the SendMessage request
    Aws::SQS::Model::SendMessageRequest sendMessageRequest;
    sendMessageRequest.SetQueueUrl(queueUrl);
    sendMessageRequest.SetMessageBody(base64Encoded);

    // Send the message
    auto sendMessageOutcome = sqs.SendMessage(sendMessageRequest);
    if (!sendMessageOutcome.IsSuccess()) {
        std::cerr << "Error sending message to SQS: " << sendMessageOutcome.GetError().GetMessage() << std::endl;
    } else {
        std::cout << "Message sent successfully!" << std::endl;
    }

    // Shutdown AWS SDK
    Aws::ShutdownAPI(options);

    return 0;
}

