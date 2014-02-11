#ifndef MULTI_LOOKUP_H
#define MULTI_LOOKUP_H

#define MAX_RESOLVER_THREADS 5

extern bool request_queue_finished;

typedef struct {
    char* fname;
    queue* request_queue;
    pthread_mutex_t* mutex_queue;
} thread_request_arg_t ;

typedef struct {
    queue* rqueue;
    FILE* outputfp;
    pthread_mutex_t* mutex_ofile;
    pthread_mutex_t* mutex_queue;
} thread_resolve_arg_t ;

void* thread_read_ifile (thread_request_arg_t*);

void* thread_dnslookup (thread_resolve_arg_t*);

#endif
