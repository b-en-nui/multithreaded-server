#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>

#include "gfserver.h"
#include "content.h"
#include "steque.h"
#include "gfserver-student.h"

#define BUFFER_SIZE 50419

extern pthread_mutex_t queue_mutex;
extern steque_t *job_queue;
extern pthread_cond_t job_queue_not_empty_cond;

//  The purpose of this function is to handle a get request
//
//  The ctx is the "context" operation and it contains connection state
//  The path is the path being retrieved
//  The arg allows the registration of context that is passed into this routine.
//  Note: you don't need to use arg. The test code uses it in some cases, but
//        not in others.
//
ssize_t gfs_handler(gfcontext_t *ctx, const char *path, void *arg) {
    job_info *job = calloc(1, sizeof(job_info));
    job->context = ctx;
    job->file_path = path;
//    job->client_socket_fd = ctx->client_socket;
    printf("ADDING JOB TO QUEUE\n");
    pthread_mutex_lock(&queue_mutex);
    steque_enqueue(job_queue, job);
    pthread_cond_broadcast(&job_queue_not_empty_cond);
    pthread_mutex_unlock(&queue_mutex);
    return 0;
}

