#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <thread>
#include <vector>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

const int payloadSize = 1024; // Payload size in bytes

// Function to create a payload with sequence number and current time
std::vector<uint8_t> makePayload(int64_t seq, int64_t currentTime) {
    std::vector<uint8_t> payload(payloadSize);
    uint64_t seq_net = htobe64(seq); // Convert sequence number to network byte order
    uint64_t time_net = htobe64(currentTime); // Convert timestamp to network byte order
    memcpy(payload.data(), &seq_net, sizeof(seq_net)); // Copy sequence number to payload
    memcpy(payload.data() + 8, &time_net, sizeof(time_net)); // Copy timestamp to payload
    return payload;
}

int main(int argc, char* argv[]) {
    // Default server settings
    std::string addrString = "0.0.0.0:5200";  // 默认端口 5200
    int64_t rate = 10; // Default rate of packets per second
    std::string filename = "server.log"; // Default log file name

    // Parse command line arguments
    if (argc > 1) {
        addrString = argv[1]; // Override default address if provided
    }
    if (argc > 2) {
        rate = std::stoll(argv[2]); // Override default rate if provided
    }
    if (argc > 3) {
        filename = argv[3]; // Override default log file name if provided
    }

    std::cout << "Server started and listening on " << addrString << std::endl;
    std::cout << "Sending at " << rate << " packet/s" << std::endl;
    std::cout << "Trace saved at " << filename << std::endl;

    // Create a UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return 1;
    }

    // Setup server address structure
    struct sockaddr_in serverAddr, clientAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(5200);  // 使用端口 5200

    // Bind to all available interfaces
    if (inet_pton(AF_INET, "0.0.0.0", &serverAddr.sin_addr) <= 0) {
        std::cerr << "Invalid address or Address not supported" << std::endl;
        close(sockfd);
        return 1;
    }

    // Bind the socket to the address and port
    if (bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error binding socket" << std::endl;
        close(sockfd);
        return 1;
    }

    // Wait for the initial packet from the client
    char buffer[payloadSize];
    socklen_t clientAddrLen = sizeof(clientAddr);
    if (recvfrom(sockfd, buffer, payloadSize, 0, (struct sockaddr*)&clientAddr, &clientAddrLen) < 0) {
        std::cerr << "Error reading from UDP" << std::endl;
        close(sockfd);
        return 1;
    }

    std::cout << "Received initial packet from " << inet_ntoa(clientAddr.sin_addr) << std::endl;

    // Open the log file for appending
    std::ofstream file(filename, std::ios::app);
    if (!file.is_open()) {
        std::cerr << "Error opening file" << std::endl;
        close(sockfd);
        return 1;
    }

    int64_t lastSeq = -1;

    // Start sending packets to the client
    auto start_time = std::chrono::high_resolution_clock::now();  // 精确计时
    while (true) {
        auto now = std::chrono::high_resolution_clock::now();
        int64_t currentTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

        // Use current Unix time as sequence number and reset it every second
        int64_t seq = currentTimeMs / 1000;
        if (seq != lastSeq) {
            lastSeq = seq;  // Update last sequence number
        }

        // Create the payload with the current sequence number and timestamp
        std::vector<uint8_t> payload = makePayload(seq, currentTimeMs);
        
        // Send the payload to the client
        if (sendto(sockfd, payload.data(), payloadSize, 0, (struct sockaddr*)&clientAddr, clientAddrLen) < 0) {
            std::cerr << "Error writing to UDP" << std::endl;
        }

        // Log the sequence number and timestamp
        file << seq << "," << currentTimeMs << std::endl;
        if (!file.good()) {
            std::cerr << "Error writing to file" << std::endl;
        }

        // Calculate elapsed time to avoid accumulating error
        auto elapsed = std::chrono::high_resolution_clock::now() - start_time;
        auto sleep_duration = std::chrono::duration<double, std::milli>(1000.0 / rate) - elapsed;

        // Adjust sleep to maintain accurate rate
        if (sleep_duration.count() > 0) {
            std::this_thread::sleep_for(sleep_duration);
        }

        start_time = std::chrono::high_resolution_clock::now();  // 重置计时
    }

    // Close the socket
    close(sockfd);
    return 0;
}