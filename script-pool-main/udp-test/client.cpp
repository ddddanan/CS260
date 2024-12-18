#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    std::string addrString = "54.241.85.37:5200";  // 将端口改为5200
    std::string filename = "client.log";

    if (argc > 1) {
        addrString = argv[1];
    }
    if (argc > 2) {
        filename = argv[2];
    }

    std::cout << "Server address: " << addrString << std::endl;
    std::cout << "Trace saved at: " << filename << std::endl;

    // Split addrString into IP address and port
    size_t colonPos = addrString.find(':');
    if (colonPos == std::string::npos) {
        std::cerr << "Invalid address format. Expected format: <IP>:<Port>" << std::endl;
        return -1;
    }

    std::string ipAddress = addrString.substr(0, colonPos);
    int port = std::stoi(addrString.substr(colonPos + 1));

    int sockfd;
    struct sockaddr_in serverAddr;
    socklen_t addrLen = sizeof(serverAddr);

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return -1;
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    if (inet_pton(AF_INET, ipAddress.c_str(), &serverAddr.sin_addr) <= 0) {
        std::cerr << "Invalid address or Address not supported" << std::endl;
        return -1;
    }

    if (connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Connection failed" << std::endl;
        return -1;
    }

    std::ofstream file(filename, std::ios::app);
    if (!file.is_open()) {
        std::cerr << "Error opening file" << std::endl;
        return -1;
    }

    // Send a message to the server
    const char* message = "Hello from client!";
    if (send(sockfd, message, strlen(message), 0) < 0) {
        std::cerr << "Error sending message" << std::endl;
        return -1;
    }

    // Start receiving packets
    char buffer[1024];
    while (true) {
        int n = recv(sockfd, buffer, sizeof(buffer), 0);
        if (n < 0) {
            std::cerr << "Error receiving message" << std::endl;
            return -1;
        }

        int64_t seq;
        int64_t sendTimeMs;
        memcpy(&seq, buffer, sizeof(seq));
        memcpy(&sendTimeMs, buffer + sizeof(seq), sizeof(sendTimeMs));

        seq = be64toh(seq);
        sendTimeMs = be64toh(sendTimeMs);

        auto currentTime = std::chrono::system_clock::now();
        auto currentTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime.time_since_epoch()).count();

        // Write the line to the file
        file << "Seq: " << seq << ", Sent: " << sendTimeMs << ", Received: " << currentTimeMs << std::endl;
        if (!file.good()) {
            std::cerr << "Error writing to file" << std::endl;
        }
    }

    close(sockfd);
    return 0;
}