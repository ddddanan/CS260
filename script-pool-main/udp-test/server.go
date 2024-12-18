package main

import (
	"encoding/binary"
    "flag"
    "fmt"
    "log"
    "net"
    "os"
    "time"
)

const payloadSize = 1024 // Maximum is 1472

func makePayload(seq int64, currentTime int64) []byte {
    dataBytes := make([]byte, 16) // 8 bytes for seq, 8 bytes for time
    binary.BigEndian.PutUint64(dataBytes[:8], uint64(seq))
    binary.BigEndian.PutUint64(dataBytes[8:16], uint64(currentTime))
    payload := make([]byte, payloadSize)
    copy(payload, dataBytes)
    return payload
}

func main() {
    addrString := flag.String("addr", "0.0.0.0:8080", "Server IP address and port number")
    rate := flag.Int64("rate", 10, "Packet sends per second")
    filename := flag.String("file", "server.log", "Default trace filename")

    flag.Parse()

    log.Println("Server started and listening on ", *addrString)
    log.Printf("Send at %d packet/s\n", *rate)
    log.Println("Trace saved at ", *filename)

    serverAddr, err := net.ResolveUDPAddr("udp", *addrString)
    if err != nil {
        log.Fatalf("Error resolving UDP address: %v", err)
    }
    serverConn, err := net.ListenUDP("udp", serverAddr)
    if err != nil {
        log.Fatalf("Error listening on UDP: %v", err)
    }
    defer serverConn.Close()

    buffer := make([]byte, payloadSize)
    _, remoteAddr, err := serverConn.ReadFromUDP(buffer)
    if err != nil {
        log.Fatalf("Error reading from UDP: %v", err)
    }

    log.Printf("Received initial packet from %v", remoteAddr)

    f, err := os.OpenFile(*filename, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
    if err != nil {
        log.Fatalf("Error Opening file: %v", err)
    }
    defer f.Close()

    ticker := time.NewTicker(time.Second / time.Duration(*rate))
    defer ticker.Stop()

    seq := time.Now().Unix() // Use current Unix time as initial sequence number
    for range ticker.C {
        currentTime := time.Now().UnixMilli()
        if time.Now().Second() == 0 { // Reset sequence number every second
            seq = time.Now().Unix()
        }

        payload := makePayload(seq, currentTime)
        _, err = serverConn.WriteToUDP(payload, remoteAddr)
        if err != nil {
            log.Printf("Error writing to UDP: %v", err)
        }

        _, err = fmt.Fprintf(f, "%d,%d\n", seq, currentTime)
        if err != nil {
            log.Printf("Error writing to file: %v", err)
        }
    }
}
