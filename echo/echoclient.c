#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <getopt.h>
#include <arpa/inet.h>

/* Be prepared accept a response of this length */
#define BUFSIZE 5041

#define USAGE                                                                       \
    "usage:\n"                                                                      \
    "  echoclient [options]\n"                                                      \
    "options:\n"                                                                    \
    "  -s                  Server (Default: localhost)\n"                           \
    "  -p                  Port (Default: 50419)\n"                                  \
    "  -m                  Message to send to server (Default: \"hello world.\")\n" \
    "  -h                  Show this help message\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
        {"server",  required_argument, NULL, 's'},
        {"port",    required_argument, NULL, 'p'},
        {"message", required_argument, NULL, 'm'},
        {"help",    no_argument,       NULL, 'h'},
        {NULL, 0,                      NULL, 0}};

/* Main ========================================================= */
int main(int argc, char **argv) {
    int option_char = 0;
    char *hostname = "localhost";
    unsigned short portno = 50419;
    char *message = "Hello World!!";

    // Parse and set command line arguments
    while ((option_char = getopt_long(argc, argv, "s:p:m:hx", gLongOptions, NULL)) != -1) {
        switch (option_char) {
            case 's': // server
                hostname = optarg;
                break;
            case 'p': // listen-port
                portno = atoi(optarg);
                break;
            default:
                fprintf(stderr, "%s", USAGE);
                exit(1);
            case 'm': // message
                message = optarg;
                break;
            case 'h': // help
                fprintf(stdout, "%s", USAGE);
                exit(0);
                break;
        }
    }

    setbuf(stdout, NULL); // disable buffering

    if ((portno < 1025) || (portno > 65535)) {
        fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__, portno);
        exit(1);
    }

    if (NULL == message) {
        fprintf(stderr, "%s @ %d: invalid message\n", __FILE__, __LINE__);
        exit(1);
    }

    if (NULL == hostname) {
        fprintf(stderr, "%s @ %d: invalid host name\n", __FILE__, __LINE__);
        exit(1);
    }

    int sock_fd;
    ssize_t n_read;
    ssize_t n_written;
    struct sockaddr_in server_addr;
    struct hostent *server;

    size_t BUFFER_SIZE = 16;
    char buffer[BUFFER_SIZE];
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("Failed to open socket");
        exit(EXIT_FAILURE);
    }

    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr, "Host not found");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    memcpy((char *) &server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    server_addr.sin_port = htons(portno);

    if (connect(sock_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("Error connecting to server");
        exit(EXIT_FAILURE);
    }

    memset(buffer, 0, BUFFER_SIZE);
    strcpy(buffer, "Hello world From client");
    n_written = write(sock_fd, buffer, strlen(buffer));
    if (n_written < 0) {
        perror("Error writing to socket");
        exit(EXIT_FAILURE);
    }

    memset(buffer, 0, BUFFER_SIZE);
    n_read = read(sock_fd, buffer, BUFFER_SIZE);
    if (n_read < 0) {
        perror("Error reading from socket");
        exit(EXIT_FAILURE);
    }

    printf("%s", buffer);
    close(sock_fd);
    return 0;
}
