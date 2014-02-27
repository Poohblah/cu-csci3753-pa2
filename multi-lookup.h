#ifndef MULTI_LOOKUP_H
#define MULTI_LOOKUP_H

#define MAX_RESOLVER_THREADS 5

typedef struct {
    char* fname;
    queue* request_queue;
} thread_request_arg_t ;

typedef struct {
    queue* rqueue;
    FILE* outputfp;
} thread_resolve_arg_t ;

void* thread_read_ifile (void*);

void* thread_dnslookup (void*);

#endif
