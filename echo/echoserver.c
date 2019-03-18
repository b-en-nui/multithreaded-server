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
#include <signal.h>
#include <stdbool.h>

#define BUFSIZE 5041

#define USAGE                                                                 \
"usage:\n"                                                                    \
"  echoserver [options]\n"                                                    \
"options:\n"                                                                  \
"  -p                  Port (Default: 50419)\n"                                \
"  -m                  Maximum pending connections (default: 1)\n"            \
"  -h                  Show this help message\n"                              \

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
        {"port",        required_argument, NULL, 'p'},
        {"maxnpending", required_argument, NULL, 'm'},
        {"help",        no_argument,       NULL, 'h'},
        {NULL, 0,                          NULL, 0}
};

void handle_echo_client(int);

int main(int argc, char **argv) {
    int option_char;
    int portno = 50419; /* port to listen on */
    int maxnpending = 1;

    // Parse and set command line arguments
    while ((option_char = getopt_long(argc, argv, "p:m:hx", gLongOptions, NULL)) != -1) {
        switch (option_char) {
            case 'p': // listen-port
                portno = atoi(optarg);
                break;
            default:
                fprintf(stderr, "%s ", USAGE);
                exit(1);
            case 'm': // server
                maxnpending = atoi(optarg);
                break;
            case 'h': // help
                fprintf(stdout, "%s ", USAGE);
                exit(0);
                break;
        }
    }

    setbuf(stdout, NULL); // disable buffering

    if ((portno < 1025) || (portno > 65535)) {
        fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__, portno);
        exit(1);
    }
    if (maxnpending < 1) {
        fprintf(stderr, "%s @ %d: invalid pending count (%d)\n", __FILE__, __LINE__, maxnpending);
        exit(1);
    }


    /* Socket Code Here */
    struct addrinfo hints, *result, *p;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // get the string length of the port
    int portlen = snprintf(NULL, 0, "%d", portno) + 1;
    char *portstr = malloc(portlen);
    snprintf(portstr, portlen, "%d", portno);

    int addr_info = getaddrinfo(NULL, portstr, &hints, &result);
    if (addr_info != 0) {
        fprintf(stderr, "Failed to get address info. %s", gai_strerror(addr_info));
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
            perror("Failed to bind server to port");
            continue;
        }
        break;
    }

    freeaddrinfo(result);
    if (p == NULL) {
        fprintf(stderr, "Server bind failed\n");
        exit(EXIT_FAILURE);
    }

    if (sock_fd < 0) {
        fprintf(stderr, "Socket creation failed");
        exit(EXIT_FAILURE);
    }

    listen(sock_fd, maxnpending);

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
            handle_echo_client(newsock_fd);
            exit(EXIT_SUCCESS);
        } else {
            close(newsock_fd);
        }
    }

    close(sock_fd);
    return 0;
}


void handle_echo_client(int sock_fd) {
    ssize_t n_read;
    size_t BUFF_SIZE = 16;
    char buffer[BUFF_SIZE];
    memset(buffer, 0, sizeof(buffer));
    n_read = read(sock_fd, buffer, BUFF_SIZE - 1);
    if (n_read < 0) {
        perror("Error reading from socket");
        exit(EXIT_FAILURE);
    }
    n_read = write(sock_fd, buffer, BUFF_SIZE - 1);
    if (n_read < 0) {
        perror("Failed to write to socket");
        exit(EXIT_FAILURE);
    }
}