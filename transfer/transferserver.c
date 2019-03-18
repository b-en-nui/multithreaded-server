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
#include <stdbool.h>

#define BUFSIZE 5041

#define USAGE                                                \
    "usage:\n"                                               \
    "  transferserver [options]\n"                           \
    "options:\n"                                             \
    "  -f                  Filename (Default: 6200.txt)\n" \
    "  -h                  Show this help message\n"         \
    "  -p                  Port (Default: 50419)\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
        {"filename", required_argument, NULL, 'f'},
        {"help",     no_argument,       NULL, 'h'},
        {"port",     required_argument, NULL, 'p'},
        {NULL, 0,                       NULL, 0}};

void handle_transfer_client(int, FILE *);

int main(int argc, char **argv) {
    int option_char;
    int portno = 50419;             /* port to listen on */
    char *filename = "6200.txt"; /* file to transfer */

    setbuf(stdout, NULL); // disable buffering

    // Parse and set command line arguments
    while ((option_char = getopt_long(argc, argv, "p:hf:x", gLongOptions, NULL)) != -1) {
        switch (option_char) {
            case 'p': // listen-port
                portno = atoi(optarg);
                break;
            default:
                fprintf(stderr, "%s", USAGE);
                exit(1);
            case 'h': // help
                fprintf(stdout, "%s", USAGE);
                exit(0);
                break;
            case 'f': // listen-port
                filename = optarg;
                break;
        }
    }


    if ((portno < 1025) || (portno > 65535)) {
        fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__, portno);
        exit(1);
    }

    if (NULL == filename) {
        fprintf(stderr, "%s @ %d: invalid filename\n", __FILE__, __LINE__);
        exit(1);
    }

    /* Socket Code Here */
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Failed to open file %s", filename);
        exit(EXIT_FAILURE);
    }

    struct addrinfo hints, *result, *p;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // get the string length of the port
    int portlen = snprintf(NULL, 0, "%d", portno) + 1;
    char *portstr = malloc(portlen);
    // convert portno to string
    snprintf(portstr, portlen, "%d", portno);

    int addr_info = getaddrinfo(NULL, portstr, &hints, &result);
    if (addr_info != 0) {
        fprintf(stderr, "Failed to get the address info. %s", gai_strerror(addr_info));
        exit(EXIT_FAILURE);
    }

    int sock_fd = -1, newsock_fd;
    socklen_t client_len;
    struct sockaddr_in client_addr;
    int yes = 1;

    for (p = result; p != NULL; p = p->ai_next) {
        sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sock_fd < 0) {
            continue;
        }

        if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes,
                       sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sock_fd, p->ai_addr, p->ai_addrlen) < 0) {
            close(sock_fd);
//            fprintf(stderr, "")
            perror("Failed to bind server to port");
            continue;
        }
        break;
    }

    freeaddrinfo(result);
    if (p == NULL) {
        fprintf(stderr, "Server bind failed. Could not bind to port %s\n", portstr);
        exit(EXIT_FAILURE);
    }

    if (sock_fd < 0) {
        fprintf(stderr, "Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // socket queue length of 10
    listen(sock_fd, 10);

    client_len = sizeof(client_addr);

    int pid;
    while (true) {

        newsock_fd = accept(sock_fd, (struct sockaddr *) &client_addr, &client_len);
        if (newsock_fd < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }
        pid = fork();
        if (pid < 0) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            close(sock_fd);
            handle_transfer_client(newsock_fd, file);
            exit(EXIT_SUCCESS);
        } else {
            close(newsock_fd);
        }
    }
}

// send file to client
void handle_transfer_client(int socket_fd, FILE *file) {
    // get the size of the file
    fseek(file, 0l, SEEK_END);
    long file_size = ftell(file);   // size of file in bytes

    // reset file position cursor
    fseek(file, 0l, SEEK_SET);

//    char *file_contents;
//    file_contents = malloc(file_size * sizeof(char));
//    memset(file_contents, 0, sizeof(char));
    fprintf(stdout, "FILE SIZE: %lu ", file_size);
    char file_contents[file_size];
    memset(file_contents, 0, sizeof(char));

    // read file contents into file_contents array
    size_t bytes_read = fread(file_contents, sizeof(char), (size_t) file_size, file);
    if (bytes_read != file_size) {
        fprintf(stderr, "Failed to read file.");
        return;
    }

    char *data = file_contents;
    // write file contents to socket
    size_t data_len = (size_t) file_size;
    ssize_t num_bytes_sent;
    while (data_len > 0) {
        num_bytes_sent = send(socket_fd, data, data_len, 0);
        if (num_bytes_sent <= 0) {
            fprintf(stderr, "Failed to send file contents");
            return;
        }
        data += num_bytes_sent;
        data_len -= num_bytes_sent;
    }
    fflush(file);
    fclose(file);
    close(socket_fd);
}
