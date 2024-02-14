#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <fstream>

int main() {
    // Initialize AWS SDK
    Aws::SDKOptions options;
    Aws::InitAPI(options);

    // S3 client configuration
    Aws::Client::ClientConfiguration clientConfig;
    clientConfig.region = "us-west-2"; // Adjust this to your bucket's region

    // Create S3 client
    Aws::S3::S3Client s3Client(clientConfig);

    // Specify object request
    Aws::S3::Model::GetObjectRequest objectRequest;
    objectRequest.WithBucket("your-bucket-name").WithKey("your-object-key");

    // Set byte range to fetch. For example: bytes=0-499
    objectRequest.SetRange("bytes=0-499");

    // Get object (part of the file in this case)
    auto getObjectOutcome = s3Client.GetObject(objectRequest);

    if (getObjectOutcome.IsSuccess()) {
        // For this example, we'll just output the fetched bytes to a local file
        std::ofstream outputFile("output_part.txt", std::ios::binary);
        outputFile << getObjectOutcome.GetResult().GetBody().rdbuf();
    } else {
        std::cout << "Error getting object: " << getObjectOutcome.GetError().GetMessage() << std::endl;
    }

    // Shutdown AWS SDK
    Aws::ShutdownAPI(options);
    return 0;
}
