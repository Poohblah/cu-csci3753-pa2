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

#define SBUFSIZE 1025
#define MINARGS 3
#define USAGE "<inputFilePath> <outputFilePath>"
#define INPUTFS "%1024s"

void* thread_read_ifile (thread_request_arg_t args) {

    return NULL;

    FILE* inputfp = NULL;
    
    /* Open Input File */
    inputfp = fopen(args.fname, "r");
    if(!inputfp){
        char errorstr[SBUFSIZE];
        sprintf(errorstr, "Error Opening Input File: %s", args.fname);
        perror(errorstr);
    }	
    
    /* Read File and Process*/
    char hostname[SBUFSIZE];
    while(fscanf(inputfp, INPUTFS, hostname) > 0){

        /* Lock request queue mutex */

        /* Add hostname to queue */

        /* Unlock request queue mutex */
    
    }
    
    /* Close Input File */
    fclose(inputfp);
}

void* thread_dnslookup (thread_resolve_arg_t args) {

    return NULL;

    /* While true*/

    /* Lock queue mutex */

    /* Exit if queue is empty and requester threads have finished */

    /* Pop hostname off queue, unlock mutex, lookup hostname and get IP string */
    char hostname[SBUFSIZE];
    char firstipstr[INET6_ADDRSTRLEN];
    if(dnslookup(hostname, firstipstr, sizeof(firstipstr))
       == UTIL_FAILURE){
        fprintf(stderr, "dnslookup error: %s\n", hostname);
        strncpy(firstipstr, "", sizeof(firstipstr));
    }
    
    /* Lock output file mutex, write to file, unlock mutex */
    fprintf(args.outputfp, "%s,%s\n", hostname, firstipstr);

    /* End while */

}

int main(int argc, char* argv[]){
    
    /* Sanity check */
    if(argc < MINARGS){
	fprintf(stderr, "Not enough arguments: %d\n", (argc - 1));
	fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
	return EXIT_FAILURE;
    }

    fprintf(stdout, "local variables\n");
    /* Local variables */
    FILE* outputfp = NULL;
    bool request_queue_finished = false;
    queue request_queue;
    int request_queue_size = QUEUEMAXSIZE;
    pthread_t threads_request[argc-1];
    pthread_t threads_resolve[MAX_RESOLVER_THREADS];
    int i;

    fprintf(stdout, "open output file\n");
    /* Open Output File */
    outputfp = fopen(argv[(argc-1)], "w");
    if(!outputfp){
	perror("Error Opening Output File");
	return EXIT_FAILURE;
    }

    fprintf(stdout, "create queues and mutexen\n");
    /* Create request queue */
    queue_init(&request_queue, request_queue_size);

    /* Create mutexen for queue and output file */
    pthread_mutex_t mutex_queue = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t mutex_ofile = PTHREAD_MUTEX_INITIALIZER;

    fprintf(stdout, "req threads\n");
    /* Spawn requester thread for each input file */
    for(i=1; i<(argc-1); i++){
        thread_request_arg_t args;
        args.fname = argv[i];
        args.request_queue = &request_queue;
        args.mutex_queue = &mutex_queue;
	int rc = pthread_create(threads_request[i-1], NULL, thread_read_ifile, &args);
	if (rc){
	    printf("Error creating request thread: return code from pthread_create() is %d\n", rc);
	    exit(EXIT_FAILURE);
	}
    }

    fprintf(stdout, "res threads\n");
    /* Spawn resolver threads up to MAX_RESOLVER_THREADS */
    thread_resolve_arg_t args;
    args.rqueue = &request_queue;
    args.outputfp = outputfp;
    args.mutex_ofile = &mutex_ofile;
    args.mutex_queue = &mutex_queue;
    for(i=0; i<MAX_RESOLVER_THREADS; i++){
        fprintf(stdout, "thread #%d\n", i);
	int rc = pthread_create(&(threads_resolve[i]), NULL, thread_dnslookup, &args);
	if (rc){
	    printf("Error creating resolver thread: return code from pthread_create() is %d\n", rc);
	    exit(EXIT_FAILURE);
	}
    }

    fprintf(stdout, "join req threads\n");
    /* Join requester threads and wait for them to finish */
    for(i=0; i<argc-2; i++){
	pthread_join(&(threads_request[i]), NULL);
    }
    request_queue_finished = true;

    /* Join resolver threads and wait for them to finish */
    for(i=0; i<MAX_RESOLVER_THREADS; i++){
	pthread_join(threads_resolve[i], NULL);
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
