#include <getopt.h>
#include <stdio.h>
#include <sys/signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>

#include "content.h"
#include "gfserver-student.h"
#include "steque.h"

#define USAGE                                                                 \
"usage:\n"                                                                    \
"  gfserver_main [options]\n"                                                 \
"options:\n"                                                                  \
"  -t [nthreads]       Number of threads (Default: 9)\n"                      \
"  -p [listen_port]    Listen port (Default: 50419)\n"                         \
"  -m [content_file]   Content file mapping keys to content files\n"          \
"  -h                  Show this help message.\n"                             \

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
        {"port",     required_argument, NULL, 'p'},
        {"nthreads", required_argument, NULL, 't'},
        {"content",  required_argument, NULL, 'm'},
        {"help",     no_argument,       NULL, 'h'},
        {NULL, 0,                       NULL, 0}
};


extern ssize_t gfs_handler(gfcontext_t *ctx, const char *path, void *arg);

/**
 * MUTEX VARIABLES
 */
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;    // mutex for the job queue
pthread_cond_t job_queue_not_empty_cond = PTHREAD_COND_INITIALIZER;

/**
 * JOB QUEUE. SHARED GLOBAL VARIABLE
 */
steque_t *job_queue;

void *thread_handler(void *arg);

static void _sig_handler(int signo) {
    if ((SIGINT == signo) || (SIGTERM == signo)) {
        exit(signo);
    }
}

/* Main ========================================================= */
int main(int argc, char **argv) {
    int option_char = 0;
    unsigned short port = 50419;
    char *content_map = "content.txt";
    gfserver_t *gfs = NULL;
    int nthreads = 9;

    setbuf(stdout, NULL);

    if (SIG_ERR == signal(SIGINT, _sig_handler)) {
        fprintf(stderr, "Can't catch SIGINT...exiting.\n");
        exit(EXIT_FAILURE);
    }

    if (SIG_ERR == signal(SIGTERM, _sig_handler)) {
        fprintf(stderr, "Can't catch SIGTERM...exiting.\n");
        exit(EXIT_FAILURE);
    }

    // Parse and set command line arguments
    while ((option_char = getopt_long(argc, argv, "t:m:p:hr", gLongOptions, NULL)) != -1) {
        switch (option_char) {
            case 'p': // listen-port
                port = atoi(optarg);
                break;
            case 't': // nthreads
                nthreads = atoi(optarg);
                break;
            case 'h': // help
                fprintf(stdout, "%s", USAGE);
                exit(0);
                break;
            default:
                fprintf(stderr, "%s", USAGE);
                exit(1);
            case 'm': // file-path
                content_map = optarg;
                break;
        }
    }

    /* not useful, but it ensures the initial code builds without warnings */
    if (nthreads < 1) {
        nthreads = 1;
    }

    content_init(content_map);

    /*Initializing server*/
    gfs = gfserver_create();

    /*Setting options*/
    gfserver_set_port(gfs, port);
    gfserver_set_maxpending(gfs, 16);
    gfserver_set_handler(gfs, gfs_handler);
    gfserver_set_handlerarg(gfs, NULL); // doesn't have to be NULL!

    // create job queue
    job_queue = calloc(1, sizeof(steque_t));
    steque_init(job_queue);

    pthread_t threads[nthreads];
    for (int i = 0; i < nthreads; i++) {
        pthread_t *thread = &threads[i];
        int res = pthread_create(thread, NULL, thread_handler, &i);
        if (res < 0) {
            perror("FAILED TO CREATE THREAD\n");
        }
    }
    printf("INIT MUTEX\n");
    pthread_mutex_init(&queue_mutex, NULL);

    /*Loops forever*/
    gfserver_serve(gfs);

//    for (int i = 0; i < nthreads; i++) {
//        pthread_t *thread = &threads[i];
//        pthread_exit()
//    }
    content_destroy();
    pthread_mutex_destroy(&queue_mutex);
    steque_destroy(job_queue);
    free(job_queue);
}

void *thread_handler(void *arg) {
//    int thread_num = *((int *) arg);
//    printf("IN THREAD HANDLER: %d\n", thread_num);
    while (true) {
        job_info *job;
        pthread_mutex_lock(&queue_mutex);
        while (steque_isempty(job_queue)) {
            pthread_cond_wait(&job_queue_not_empty_cond, &queue_mutex);
        }
        job = steque_pop(job_queue);
        printf("POPPED JOB %s FROM QUEUE\n", job->file_path);
        pthread_mutex_unlock(&queue_mutex);

        if (job == NULL) {
            continue;
        }

        int fd = content_get(job->file_path);
        if (fd < 0) {
            // file not found
            printf("FILE %s not found", job->file_path);
            gfs_sendheader(job->context, GF_FILE_NOT_FOUND, 0);
            continue;
        }

        // get file length
        // get the size of the file
        struct stat stat_buff;
        fstat(fd, &stat_buff);
        long file_size = stat_buff.st_size;
//        long file_size = fstat(fd, 0l, SEEK_END);
        printf("FILE LENGTH: %li\n", file_size);// size of file in bytes

        gfs_sendheader(job->context, GF_OK, file_size);

        ssize_t BUFFER_SIZE = 1024;
        char file_buffer[BUFFER_SIZE];
        // read file contents
        int file_bytes_read = 0;
        int client_bytes_written = 0;
        while (client_bytes_written < file_size) {
//            printf("THREAD #%d. CLIENT BYTES WRITTEN: %d. FILE SIZE: %d\n", thread_num, client_bytes_written,
//                   (int) file_size);
            ssize_t pread_res = pread(fd, file_buffer, BUFFER_SIZE, client_bytes_written);
            if (pread_res < 0) {
                // TODO: handle read error
                break;
            }
            file_bytes_read += pread_res;
//            printf("THREAD #%d. FILE BYTES READ: %d\n", thread_num, file_bytes_read);

            // write body
            ssize_t n;
//            printf("THREAD #%d. CLIENT BYTES WRITTEN: %d. PREAD_RES: %d\n", thread_num, client_bytes_written,
//                   (int) pread_res);
            n = gfs_send(job->context, file_buffer, pread_res);
            client_bytes_written += n;
            while (client_bytes_written < pread_res) {
                n = gfs_send(job->context, file_buffer, pread_res);
                if (n < 0) {
                    // TODO: handle error
                    printf("N < 0\n");
                    break;
                }
                client_bytes_written += n;
//                printf("THREAD #%d. WRITTEN %d BYTES TO CLIENT\n", thread_num, client_bytes_written);
            }
//            printf("THREAD #%d. TOTAL FILE BYTES READ: %d\n", thread_num, file_bytes_read);
            bzero(file_buffer, BUFFER_SIZE);
        }
        free(job);
    }
    printf("EXITING THREAD\n");
    pthread_exit(NULL);
}