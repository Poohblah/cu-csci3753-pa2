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

bool request_queue_finished = false;

void microsleep () {
    struct timespec ts;
    ts.tv_sec = 0;
    srand(time(NULL));
    ts.tv_nsec = rand() % 100000;
    nanosleep(&ts, &ts);
    return;
}

void* thread_read_ifile (thread_request_arg_t* args) {

    fprintf(stdout, "hello from read file thread\n");
    FILE* inputfp = NULL;
    
    /* Open Input File */
        
    fprintf(stdout, "reading from input file: %s\n", args->fname);
    inputfp = fopen(args->fname, "r");
    if(!inputfp){
        char errorstr[SBUFSIZE];
        sprintf(errorstr, "Error Opening Input File: %s", args->fname);
        perror(errorstr);
    }	
    
    /* Read File and Process*/
    char hostname[SBUFSIZE];
    while(fscanf(inputfp, INPUTFS, hostname) > 0){

        /* Add hostname to queue */
        struct timespec ts;
        ts.tv_sec = 0;
        srand(time(NULL));
        while (1) {

            /* Lock request queue mutex */
            pthread_mutex_lock(args->mutex_queue);
            
            if (queue_is_full(args->request_queue)) {

                /* Wait for queue if full */
                ts.tv_nsec = rand() % 100000;
                nanosleep(&ts, &ts);

            } else {

                /* malloc space for the hostname */
                int hs = sizeof(hostname);
                char* hp = malloc(hs);
                strncpy(hp, hostname, hs);

                /* Add hostname to queue */
                fprintf(stdout, "adding hostname to queue (%s): %s\n",args->fname, hp);
                queue_push(args->request_queue, hp);

                /* Unlock request queue mutex */
                pthread_mutex_unlock(args->mutex_queue);
                break;
            }

            /* Unlock request queue mutex */
            pthread_mutex_unlock(args->mutex_queue);

        }
    
    }
    
    /* Close Input File */
    fclose(inputfp);

    return NULL;
}

void* thread_dnslookup (thread_resolve_arg_t* args) {

    /* While true*/
    while (1) {
    
        /* Lock queue mutex, pop item, unlock mutex */
        pthread_mutex_lock(args->mutex_queue);
        char* hostnamep = queue_pop(args->rqueue);
        pthread_mutex_unlock(args->mutex_queue);

        /* If queue is not empty, read a hostname and look it up */
        if (hostnamep) {
            char hostname[SBUFSIZE];
            sprintf(hostname, hostnamep);
            free(hostnamep);
            fprintf(stdout, "looking up hostname: %s\n", hostname);

            /* Lookup hostname and get IP string */

            char firstipstr[INET6_ADDRSTRLEN];
            if (dnslookup(hostname, firstipstr, sizeof(firstipstr))
               == UTIL_FAILURE){
                fprintf(stderr, "dnslookup error: %s\n", hostname);
                strncpy(firstipstr, "", sizeof(firstipstr));
            }
            
            /* Lock output file mutex, write to file, unlock mutex */
            fprintf(stdout, "%s,%s\n", hostname, firstipstr);
            pthread_mutex_lock(args->mutex_ofile);
            fprintf(args->outputfp, "%s,%s\n", hostname, firstipstr);
            pthread_mutex_unlock(args->mutex_ofile);
    
        /* If queue is empty, check if requester threads have finished */
        } else {

            /* Exit if requester threads have finished */
            if (request_queue_finished) { break; }

            /* Sleep if requester threads are still active */
            else { microsleep(); }

        }
    
        /* End while */

    }

    return NULL;
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
    thread_request_arg_t req_args[argc-2];
    for(i=1; i<(argc-1); i++){
        fprintf(stdout, "creating args for thread: %d\n", i);
        req_args[i-1].fname = argv[i];
        req_args[i-1].request_queue = &request_queue;
        req_args[i-1].mutex_queue = &mutex_queue;
        fprintf(stdout, "creating thread for input file: %s\n", req_args[i-1].fname);
	int rc = pthread_create(&(threads_request[i-1]), NULL, thread_read_ifile, &(req_args[i-1]));
	if (rc){
	    printf("Error creating request thread: return code from pthread_create() is %d\n", rc);
	    exit(EXIT_FAILURE);
	}
    }

    fprintf(stdout, "res threads\n");
    /* Spawn resolver threads up to MAX_RESOLVER_THREADS */
    thread_resolve_arg_t res_args;
    res_args.rqueue = &request_queue;
    res_args.outputfp = outputfp;
    res_args.mutex_ofile = &mutex_ofile;
    res_args.mutex_queue = &mutex_queue;
    for(i=0; i<MAX_RESOLVER_THREADS; i++){
        fprintf(stdout, "thread #%d\n", i);
	int rc = pthread_create(&(threads_resolve[i]), NULL, thread_dnslookup, &res_args);
	if (rc){
	    printf("Error creating resolver thread: return code from pthread_create() is %d\n", rc);
	    exit(EXIT_FAILURE);
	}
    }

    fprintf(stdout, "join req threads\n");
    /* Join requester threads and wait for them to finish */
    for(i=0; i<argc-2; i++){
        fprintf(stdout, "waiting for thread %d to terminate...\n", i);
	int rv = pthread_join(threads_request[i], NULL);
	if (rv) {
            fprintf(stderr, "Error: pthread_join on requester thread returned %d\n", rv);
        }
    }
    request_queue_finished = true;
    // char* hostnamep;
    // char hostname[SBUFSIZE];
    // while (hostnamep = queue_pop(&request_queue)) {
    //         sprintf(hostname, hostnamep);
    //         fprintf(stdout, "Queued hostname: %s\n", hostname);
    // }

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
