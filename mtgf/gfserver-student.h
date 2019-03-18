/*
 *  This file is for use by students to define anything they wish.  It is used by the gf server implementation
 */
#ifndef __GF_SERVER_STUDENT_H__
#define __GF_SERVER_STUDENT_H__

#include "gfserver.h"
#include "gf-student.h"

typedef struct job_info {
    const char *file_path;
    gfcontext_t *context;
//    int client_socket_fd;
//    void *arg;
//    ssize_t bytes_transferred;
} job_info;

#endif // __GF_SERVER_STUDENT_H__