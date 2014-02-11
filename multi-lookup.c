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

#define MINARGS 3
#define USAGE "<inputFilePath> <outputFilePath>"
#define SBUFSIZE 1025
#define INPUTFS "%1024s"
#define MAX_RESOLVER_THREADS 5

void thread_dnslookup (queue* rqueue,
                       FILE* outputfp,
                       pthread_mutex_t* mutex_ofile,
                       pthread_mutex_t* mutex_queue) {

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
    fprintf(outputfp, "%s,%s\n", hostname, firstipstr);

    /* End while */

}

void thread_read_ifile(char* fname,
                       queue* request_queue,
                       pthread_mutex_t* mutex_queue,
                       char errorstr[SBUFSIZE]) {

    FILE* inputfp = NULL;
    
    /* Open Input File */
    inputfp = fopen(fname, "r");
    if(!inputfp){
        sprintf(errorstr, "Error Opening Input File: %s", fname);
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

int main(int argc, char* argv[]){
    
    /* Sanity check */
    if(argc < MINARGS){
	fprintf(stderr, "Not enough arguments: %d\n", (argc - 1));
	fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
	return EXIT_FAILURE;
    }

    /* Local variables */
    FILE* outputfp = NULL;
    char errorstr[SBUFSIZE];
    bool request_queue_finished = false;
    queue request_queue;
    int request_queue_size = QUEUEMAXSIZE;
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
    for(i=1; i<(argc-1); i++){
	
    }

    /* Spawn resolver threads up to MAX_RESOLVER_THREADS */
    for(i=0; i<MAX_RESOLVER_THREADS; i++){
	
    }

    /* Join requester threads and wait for them to finish */
    request_queue_finished = true;

    /* Join resolver threads and wait for them to finish */

    /* Destroy queue */

    /* Close Output File */
    fclose(outputfp);

    /* Destroy mutexen */
    pthread_mutex_destroy(&mutex_queue);
    pthread_mutex_destroy(&mutex_ofile);

    return EXIT_SUCCESS;
}
