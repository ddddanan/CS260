#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <numeric> // for std::accumulate
#include "D:\learning\master\project\code\max-min\json.hpp" // Make sure to provide the correct path to the json.hpp

using json = nlohmann::json;
using namespace std;

void process_file(const string& file_path) {
    /*ifstream json_file(file_path);
    if (!json_file.is_open()) {
        cerr << "Cannot open file: " << file_path << endl;
        return;
    }

    json data;
    json_file >> data;

    vector<double> rtts;
    double total_bps = 0;
    int bps_count = 0;

    for (const auto& interval : data["intervals"]) {
        for (const auto& stream : interval["streams"]) {
            rtts.push_back(stream["rtt"].get<double>());
            total_bps += stream["bits_per_second"].get<double>();
                bps_count++;
        }
    }

    json_file.close();*/

    double rtt_max = 1.241;
    double rtt_min = 0.013;
    // Convert RTTs from microseconds to seconds
    //rtt_max /= 1e6;
    //rtt_min /= 1e6;

    // blksize as S 数据包大小
    const int blksize_bytes = 1024; // tcp_mss_default   1024bytes/packet * number of packet send/s
    //const double blksize_bits = blksize_bytes * 8.0;

    // Calculate the average bits per second as C 平均链路容量
    //double avg_bps1 = total_bps / static_cast<double>(bps_count);
    //double avg_bps = 39443740;
    //cout<<avg_bps1<<endl;

    //double C = 113554;
    double C = 58000000;

    // Calculate the buffer size (L) in bits
    double L_bytes = (rtt_max - rtt_min) * C / blksize_bytes;
    //double L_bytes = L_bits / 8.0;

    //cout << "File: " << file_path << endl;
    cout << "Estimated Buffer Size (L): " << L_bytes << " KB" << endl << endl;
}

int main() {
    // Add the actual file paths of your logs here
    vector<string> files = {
        /*"iperf-sender-tcp-1-2023-02-14-062856.log",
        "iperf-sender-tcp-2-2023-02-14-064650.log",
        "iperf-sender-tcp-3-2023-02-14-065051.log",
        "iperf-sender-tcp-4-2023-02-14-065333.log",*/
        "client.log"
    };

    for (const string& file_path : files) {
        process_file(file_path);
    }

    return 0;
}
