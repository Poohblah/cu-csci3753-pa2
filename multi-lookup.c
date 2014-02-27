/*
 * File: multi-lookup.c
 * Author: Jay Hendren
 * Project: CSCI 3753 Programming Assignment 2
 *  
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "queue.h"
#include "util.h"
#include "multi-lookup.h"

#define DEBUG 0
#define SBUFSIZE 1025
#define MINARGS 3
#define USAGE "<inputFilePath> <outputFilePath>"
#define INPUTFS "%1024s"

/* Create condition variables for queue */
pthread_cond_t cond_queue_empty = PTHREAD_COND_INITIALIZER;

bool request_queue_finished = false;

void microsleep () {
    struct timespec ts;
    ts.tv_sec = 0;
    srand(time(NULL));
    ts.tv_nsec = rand() % 1000000;
    nanosleep(&ts, &ts);
    return;
}

void* thread_read_ifile (void* a) {
    thread_request_arg_t* args = (thread_request_arg_t*) a;

    if (DEBUG) { fprintf(stderr, "initializing requester thread...\n"); }

    FILE* inputfp = NULL;
    
    /* Open Input File */
        
    if (DEBUG) { fprintf(stderr, "opening input file: %s\n", args->fname); }
    inputfp = fopen(args->fname, "r");
    if(!inputfp){
        char errorstr[SBUFSIZE];
        sprintf(errorstr, "Error Opening Input File: %s", args->fname);
        perror(errorstr);
        return NULL;
    }	

    
    /* Read File and Process*/
    char hostname[SBUFSIZE];
    while(fscanf(inputfp, INPUTFS, hostname) > 0){

        /* While the queue is not full, push a hostname onto the queue and signal */

        /* If the queue is full, wait for a signal */

        /* Wait for queue to become available */
        while (1) {

            /* Lock request queue mutex */
            pthread_mutex_lock(args->mutex_queue);
            
            /* If queue is full, unlock mutex and wait; else add hostname and unlock */
            if (queue_is_full(args->request_queue)) {
                if (DEBUG) { fprintf(stderr, "queue is full, waiting to push\n"); }
                pthread_mutex_unlock(args->mutex_queue);
                microsleep();

            } else { break; }

        }

        /* malloc space for the hostname */
        int hs = sizeof(hostname);
        char* hp = malloc(hs);
        strncpy(hp, hostname, hs);

        /* Add hostname to queue */
        if (DEBUG) { fprintf(stderr, "pushing hostname onto queue: %s\n", hostname); }
        queue_push(args->request_queue, hp);

        /* Unlock request queue mutex */
        pthread_mutex_unlock(args->mutex_queue);

        /* Signal queue not empty */
        if (DEBUG) { fprintf(stderr, "signalling queue is ready to pop: %s\n", hostname); }
        pthread_cond_signal(&cond_queue_empty);
    
    }
    
    /* Close Input File */
    fclose(inputfp);

    return NULL;
}

void* thread_dnslookup (void* a) {
    thread_resolve_arg_t* args = (thread_resolve_arg_t*) a;
    
    if (DEBUG) { fprintf(stderr, "initializing lookup thread...\n"); }

    /* While true, pop items off the queue until thread exits */
    while (1) {
    
        char* hostnamep;

        /* While the queue is empty and the requester threads are still running,
           wait for a signal and pop a hostname off the queue */
        if (DEBUG) { fprintf(stderr, "grabbing hostname from queue\n"); }
        pthread_mutex_lock(args->mutex_queue);
        while ( (hostnamep = queue_pop(args->rqueue)) == NULL ) {
            /* if requester threads are finished, then exit */
            if (request_queue_finished) {
                if (DEBUG) { fprintf(stderr, "requesters finished, exiting\n"); }
                pthread_mutex_unlock(args->mutex_queue);
                return NULL;
                }
            /* if requester threads are still running, then wait for signal */
            if (DEBUG) { fprintf(stderr, "no hostname available on queue, waiting...\n"); }
            pthread_cond_wait(&cond_queue_empty, args->mutex_queue);
        }
        pthread_mutex_unlock(args->mutex_queue);

        /* After popping a hostname, signal that queue is ready */

        /* If queue is not empty, read a hostname and look it up */
        char hostname[SBUFSIZE];
        sprintf(hostname, "%s", hostnamep);
        free(hostnamep);

        /* Lookup hostname and get IP string */
        if (DEBUG) { fprintf(stderr, "resolving hostname: %s\n", hostname); }
        char firstipstr[INET6_ADDRSTRLEN];
        if (dnslookup(hostname, firstipstr, sizeof(firstipstr))
           == UTIL_FAILURE){
            fprintf(stderr, "dnslookup error: %s\n", hostname);
            strncpy(firstipstr, "", sizeof(firstipstr));
        }
        
        /* Lock output file mutex, write to file, unlock mutex */
        if (DEBUG) { fprintf(stderr, "resolved hostname, writing IP to file: %s %s\n", hostname, firstipstr); }
        pthread_mutex_lock(args->mutex_ofile);
        fprintf(args->outputfp, "%s,%s\n", hostname, firstipstr);
        pthread_mutex_unlock(args->mutex_ofile);
    
    }
}

int main(int argc, char* argv[]){
    
    /* Sanity check */
    if(argc < MINARGS){
	fprintf(stderr, "Not enough arguments: %d\n", (argc - 1));
	fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
	return EXIT_FAILURE;
    }

    /* Local variables */
    FILE* outputfp = NULL;
    queue request_queue;
    int request_queue_size = QUEUEMAXSIZE;
    pthread_t threads_request[argc-1];
    pthread_t threads_resolve[MAX_RESOLVER_THREADS];
    int i;

    /* Open Output File */
    outputfp = fopen(argv[(argc-1)], "w");
    if(!outputfp){
	perror("Error Opening Output File");
	return EXIT_FAILURE;
    }

    /* Create request queue */
    queue_init(&request_queue, request_queue_size);

    /* Create mutexen for queue and output file */
    pthread_mutex_t mutex_queue = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t mutex_ofile = PTHREAD_MUTEX_INITIALIZER;

    /* Spawn requester thread for each input file */
    thread_request_arg_t req_args[argc-2];
    for(i=1; i<(argc-1); i++){
        req_args[i-1].fname = argv[i];
        req_args[i-1].request_queue = &request_queue;
        req_args[i-1].mutex_queue = &mutex_queue;
	int rc = pthread_create(&(threads_request[i-1]), NULL, thread_read_ifile, &(req_args[i-1]));
	if (rc){
	    printf("Error creating request thread: return code from pthread_create() is %d\n", rc);
	    exit(EXIT_FAILURE);
	}
    }

    /* Spawn resolver threads up to MAX_RESOLVER_THREADS */
    thread_resolve_arg_t res_args;
    res_args.rqueue = &request_queue;
    res_args.outputfp = outputfp;
    res_args.mutex_ofile = &mutex_ofile;
    res_args.mutex_queue = &mutex_queue;
    for(i=0; i<MAX_RESOLVER_THREADS; i++){
	int rc = pthread_create(&(threads_resolve[i]), NULL, thread_dnslookup, &res_args);
	if (rc){
	    printf("Error creating resolver thread: return code from pthread_create() is %d\n", rc);
	    exit(EXIT_FAILURE);
	}
    }

    /* Join requester threads and wait for them to finish */
    for(i=0; i<argc-2; i++){
	int rv = pthread_join(threads_request[i], NULL);
	if (rv) {
            fprintf(stderr, "Error: pthread_join on requester thread returned %d\n", rv);
        }
    }
    request_queue_finished = true;

    /* Join resolver threads and wait for them to finish */
    for(i=0; i<MAX_RESOLVER_THREADS; i++){
	int rv = pthread_join(threads_resolve[i], NULL);
	if (rv) {
            fprintf(stderr, "Error: pthread_join on resolver thread returned %d\n", rv);
        }
    }

    /* Destroy queue */
    queue_cleanup(&request_queue);

    /* Close output file */
    fclose(outputfp);

    /* Destroy mutexen */
    pthread_mutex_destroy(&mutex_queue);
    pthread_mutex_destroy(&mutex_ofile);

    return EXIT_SUCCESS;
}
