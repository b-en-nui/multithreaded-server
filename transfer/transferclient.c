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

#define BUFSIZE 5041

#define USAGE                                                \
    "usage:\n"                                               \
    "  transferclient [options]\n"                           \
    "options:\n"                                             \
    "  -s                  Server (Default: localhost)\n"    \
    "  -p                  Port (Default: 50419)\n"           \
    "  -o                  Output file (Default cs6200.txt)\n" \
    "  -h                  Show this help message\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
        {"server", required_argument, NULL, 's'},
        {"port",   required_argument, NULL, 'p'},
        {"output", required_argument, NULL, 'o'},
        {"help",   no_argument,       NULL, 'h'},
        {NULL, 0,                     NULL, 0}};

/* Main ========================================================= */
int main(int argc, char **argv) {
    int option_char = 0;
    char *hostname = "localhost";
    unsigned short portno = 50419;
    char *filename = "cs6200.txt";

    setbuf(stdout, NULL);

    // Parse and set command line arguments
    while ((option_char = getopt_long(argc, argv, "s:p:o:hx", gLongOptions, NULL)) != -1) {
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
            case 'o': // filename
                filename = optarg;
                break;
            case 'h': // help
                fprintf(stdout, "%s", USAGE);
                exit(0);
                break;
        }
    }

    if (NULL == hostname) {
        fprintf(stderr, "%s @ %d: invalid host name\n", __FILE__, __LINE__);
        exit(1);
    }

    if (NULL == filename) {
        fprintf(stderr, "%s @ %d: invalid filename\n", __FILE__, __LINE__);
        exit(1);
    }

    if ((portno < 1025) || (portno > 65535)) {
        fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__, portno);
        exit(1);
    }

    /* Socket Code Here */
    int sock_fd;
    struct sockaddr_in server_addr;
    struct hostent *server;

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

    // create file
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        fprintf(stderr, "Failed to create file %s\n", filename);
        exit(EXIT_FAILURE);
    }

    size_t BUFFER_SIZE = 16;
    char buffer[BUFFER_SIZE];
    ssize_t num_bytes_read = 0;
    while ((num_bytes_read = recv(sock_fd, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, (size_t) num_bytes_read, sizeof(char), file);
    }

    if (num_bytes_read == -1) {
        perror("Error reading from server");
        exit(EXIT_FAILURE);
    }

    fflush(file);
    fclose(file);

}
