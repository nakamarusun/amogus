#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#define PORT 8080
#define MAXWAIT 60000
#define BUFLEN 1024

void r_file(const char* file) {
    struct sockaddr_in address;
    int sock_fd, in_sock;
    int addrlen = sizeof(address);
    int opt = 1;
    char file_buf[BUFLEN] = {0};

    // Creates the socket and gets the file descriptor
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    // Error
    if (sock_fd == -1) {
        perror("Error in creating socket file descriptor");
        exit(EXIT_FAILURE);
    }

    // Set a socket option to catch errors early
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                                                  &opt, sizeof(opt))) {
        perror("Error socket option");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );

    // Bind the socket to the address
    if (bind(sock_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Error binding socket");
        exit(EXIT_FAILURE);
    }

    // Listen to incoming connections
    if (listen(sock_fd, 3) < 0) {
        perror("Error listening");
        exit(EXIT_FAILURE);
    }
    
    // Accepts new connection
    in_sock = accept(sock_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
    if (in_sock < 0) {
        perror("Error accepting new connection");
        exit(EXIT_FAILURE);
    }

    int valread = read(in_sock, file_buf, BUFLEN);
    printf("%s\n", file_buf);
}

void s_file(const char* file) {
    struct sockaddr_in address;
    int sock_fd;
    char file_buf[BUFLEN] = {0};

    // Creates the socket and gets the file descriptor
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    // Error
    if (sock_fd == -1) {
        perror("Error in creating socket file descriptor");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_port = htons( PORT );

    // Convert string ipv4 ip address to c form
    if(inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) <= 0) {
        perror("Invalid address");
        exit(EXIT_FAILURE);
    }

    // Connect to the specified address
    if (connect(sock_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Error connecting");
        exit(EXIT_FAILURE);
    }

    send(sock_fd, file, strlen(file), 0);
}

int main(int argc, char const *argv[]) {

    if (argc == 3) {
        const char* opt = argv[1];
        const char* file = argv[2];
        // Check arguments

        if (strcmp(opt, "--receive") == 0) {
            r_file(file);
        } else if (strcmp(opt, "--send") == 0) {
            s_file(file);
        } else {
            perror("Wrong argument");
            exit(EXIT_FAILURE);
        }
    } else {
        printf("Automated Mission for Online Graphs Utility System (AMOGUS)\n\n" \
        "Usage:\n" \
        "  amogus --receive <directory>     Opens a socket to receive a file to a dir\n" \
        "  amogus --send <file>             Sends a file to an open socket in the net\n" \
        "  amogus                           Shows this help prompt\n" \
        );
    }

    return 0;
}

// TODO: Close socket