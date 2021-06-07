#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#define PORT 8080
#define BUFLEN 4

void p_err(const char* err) {
    perror(err);
    exit(EXIT_FAILURE);
}

void r_file(const char* file) {
    // Try to open file
    FILE* f = fopen(file, "wb");
    if (f == NULL)
        perror("Error opening file");

    // Listen for UDP connections
    int u_sock_fd;
    struct sockaddr_in u_servaddr, u_cliaddr;

    // Create socket
    if ((u_sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        p_err("Error in creating socket file descriptor");

    u_servaddr.sin_family = AF_INET;
    u_servaddr.sin_addr.s_addr = INADDR_ANY;
    u_servaddr.sin_port = htons(PORT);

    // Bind address
    if (bind(u_sock_fd, (const struct sockaddr*)&u_servaddr, sizeof(u_servaddr)) < 0)
        p_err("Error binding UDP");

    int len, n;
    char u_buf[4];
    len = sizeof(u_cliaddr);  //len is value/resuslt
  
    n = recvfrom(u_sock_fd, (char *)u_buf, 4, MSG_WAITALL, ( struct sockaddr *) &u_cliaddr, &len);
    u_buf[n] = '\0';

    // Check if syn message is true
    if (strcmp(u_buf, "abcd") != 0)
        p_err("Wrong syn message");
    printf("UDP syn accepted\n");

    // Get self ip
    struct in_addr ip_addr = u_cliaddr.sin_addr; // TODO: MIGHT BE AN ERROR
    char self_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ip_addr, self_ip, INET_ADDRSTRLEN);

    sendto(u_sock_fd, (const char *)self_ip, INET_ADDRSTRLEN, 
        MSG_CONFIRM, (const struct sockaddr *) &u_cliaddr,
            len);

    struct sockaddr_in s_address;
    int s_sock_fd, s_in_sock;
    int s_addrlen = sizeof(s_address);
    int opt = 1;

    // Creates the socket and gets the file descriptor
    s_sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    // Error
    if (s_sock_fd == -1)
        p_err("Error in creating socket file descriptor");

    // Set a socket option to catch errors early
    if (setsockopt(s_sock_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                                                  &opt, sizeof(opt)))
        p_err("Error socket option");

    s_address.sin_family = AF_INET;
    s_address.sin_addr.s_addr = INADDR_ANY;
    s_address.sin_port = htons( PORT );

    // Bind the socket to the address
    if (bind(s_sock_fd, (struct sockaddr *)&s_address, sizeof(s_address)) < 0)
        p_err("Error binding socket");

    // Listen to incoming connections
    if (listen(s_sock_fd, 3) < 0) 
        p_err("Error listening");

    // Accepts new connection
    s_in_sock = accept(s_sock_fd, (struct sockaddr *)&s_address, (socklen_t*)&s_addrlen);
    if (s_in_sock < 0)
        p_err("Error accepting new connection");

    // Create buffer
    char file_buf[BUFLEN] = {0};
    int ch_r;

    while ((ch_r = read(s_in_sock, file_buf, BUFLEN)) > 0) {
        printf("Lole: %s\n", file_buf);
        fwrite(file_buf, 1, ch_r, f);
    }

    fclose(f);
}

void s_file(const char* file) {

    // Check if file exists first
    FILE* f = fopen(file, "rb");
    if (f == NULL)
        p_err("Error opening file");

    int u_sock_fd;
    struct sockaddr_in u_address;

    // Make socket
    if ((u_sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
        p_err("Error creating UDP sock");

    u_address.sin_family = AF_INET;
    u_address.sin_port = htons(PORT);
    u_address.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    // Get permission to send broadcast packet
    int broadcast = 1;
    if (setsockopt(u_sock_fd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) != 0)
        p_err("Failed getting broadcast permission");

    // Send syn
    sendto(u_sock_fd, "abcd", 4, 0, (const struct sockaddr*)&u_address, sizeof(u_address));
    char ip_dest[INET_ADDRSTRLEN];
    int len;
    int n = recvfrom(u_sock_fd, (char *)ip_dest, INET_ADDRSTRLEN, MSG_WAITALL, (struct sockaddr*)&u_address, &len);
    ip_dest[n] = '\0';
    printf("Found at: %s\n", ip_dest);

    // TCP time
    struct sockaddr_in s_address;
    int s_sock_fd;

    // Creates the socket and gets the file descriptor
    s_sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    // Error
    if (s_sock_fd == -1)
        p_err("Error in creating socket file descriptor");

    s_address.sin_family = AF_INET;
    s_address.sin_port = htons( PORT );

    // Convert string ipv4 ip address to c form
    if(inet_pton(AF_INET, ip_dest, &s_address.sin_addr) <= 0)
        p_err("Invalid address");

    // Connect to the received address
    if (connect(s_sock_fd, (struct sockaddr *)&s_address, sizeof(s_address)) < 0)
        p_err("Error connecting");


    int ch_r;
    char file_buf[BUFLEN] = {0};

    // Sends the file in chunks of BUFLEN
    while ((ch_r = fread(file_buf, 1, BUFLEN, f)) > 0) {
        printf("Lole(%d): %s\n", ch_r, file_buf);
        send(s_sock_fd, file_buf, ch_r, 0);
    }

    fclose(f);
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