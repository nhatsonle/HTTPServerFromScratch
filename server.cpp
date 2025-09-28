// server.cpp
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <cstdio>   // perror
#include <cstring>  // memset

using namespace std;

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket failed");
        return 1;
    }

    // prepare address
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // or INADDR_ANY

    // bind
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
        close(sockfd);
        return 1;
    }

    // listen
    if (listen(sockfd, 10) < 0) {
        perror("listen failed");
        close(sockfd);
        return 1;
    }

    cout << "Listening on 0.0.0.0:8080 ...\n";

    // accept a single client (toy server)
    struct sockaddr_in clientAddr{};
    socklen_t clientLen = sizeof(clientAddr);
    int clientSock = accept(sockfd, (struct sockaddr*)&clientAddr, &clientLen);
    if (clientSock < 0) {
        perror("accept failed");
        close(sockfd);
        return 1;
    }

    

    // receive request
    char buf[4096];
    ssize_t n = recv(clientSock, buf, sizeof(buf) - 1, 0);
    if (n < 0) {
        perror("recv failed");
        close(clientSock);
        close(sockfd);
        return 1;
    }
    if (n == 0) {
        // client closed connection
        cerr << "client closed connection before sending data\n";
        close(clientSock);
        close(sockfd);
        return 1;
    }
    buf[n] = '\0';
    cout << "---- request start ----\n" << buf << "\n---- request end ----\n";

    // prepare response
    const string body = "Hello, world!";
    string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Length: " + to_string(body.size()) + "\r\n";
    response += "Content-Type: text/plain\r\n";
    response += "\r\n";
    response += body;

    // send response (handle partial writes)
    const char* data = response.c_str();
    size_t to_send = response.size();
    ssize_t total_sent = 0;
    while (to_send > 0) {
        ssize_t s = send(clientSock, data + total_sent, to_send, 0);
        if (s < 0) {
            perror("send failed");
            break;
        }
        total_sent += s;
        to_send -= s;
    }

    // cleanup
    close(clientSock);
    close(sockfd);
    cout << "Connection handled, server exiting.\n";
    return 0;
}
