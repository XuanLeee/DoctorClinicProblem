#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>


void process_data(char *buffer, int bufferSizeInBytes);
int get_external_data(char *buffer, int bufferSizeInBytes);


int min(int a, int b) { return a > b ? b : a; }


int get_external_data(char *buffer, int bufferSizeInBytes)
{
    int status;
    int val;
    char srcString[] = "0123456789abcdefghijklmnopqrstuvwxyxABCDEFGHIJKLMNOPQRSTUVWXYZ";

    val = (int) (random() % min(bufferSizeInBytes, 62));

    if (bufferSizeInBytes < val)
        return (-1);

    strncpy(buffer, srcString, val);

    return val;
}


void process_data(char *buffer, int bufferSizeInBytes)
{
    int i;

    if(buffer)
    {
        printf("thread %i - ", pthread_self());
        for(i=0; i<bufferSizeInBytes; i++)
        {
            printf("%c", buffer[i]);
        }
        printf("\n");
        memset(buffer, 0, bufferSizeInBytes);
    }
    else
        printf("error in process data - %i\n", pthread_self());

    return;
}


// TODO Define global data structures to be used
#define BUFFER_ROWS  8
#define BUFFER_COLS  62

static char *buffer[BUFFER_ROWS][BUFFER_COLS] = { { 0 } };
static int  buffer_wp = 0;
static int  buffer_rp = 0;

static pthread_mutex_t buffer_mutex = { { 0 } };
static pthread_cond_t  buffer_cond_avail_r = { { 0 } };
static pthread_cond_t  buffer_cond_avail_w = { { 0 } };


/**
 * this thread is responsible for pulling data off of the shared data 
 * area and processing it using the process_data() API.
 */
void *reader_thread(void *arg) {
    pthread_mutex_lock(&buffer_mutex);

    while (!(buffer_rp < buffer_wp)) {
        pthread_cond_wait(&buffer_cond_avail_r, &buffer_mutex);
    }


    process_data((char *) buffer[buffer_rp++ % BUFFER_ROWS],BUFFER_COLS);
    pthread_cond_signal(&buffer_cond_avail_w);

    pthread_mutex_unlock(&buffer_mutex);

    return NULL;
}


/**
 * This thread is responsible for pulling data from a device using
 * the get_external_data() API and placing it into a shared area
 * for later processing by one of the reader threads.
 */
void *writer_thread(void *arg) {
    pthread_mutex_lock(&buffer_mutex);


    while (!(buffer_wp - buffer_rp < BUFFER_ROWS)) {
        pthread_cond_wait(&buffer_cond_avail_w, &buffer_mutex);
    }

    get_external_data((char *) buffer[buffer_wp++ % BUFFER_ROWS], BUFFER_COLS-1);
    pthread_cond_signal(&buffer_cond_avail_r);

    pthread_mutex_unlock(&buffer_mutex);

    return NULL;
}


#define M 10
#define N 20

int main(int argc, char **argv) {
    int i,rc;
    pthread_t writer_threads[N] = { 0 };
    pthread_t reader_threads[M] = { 0 };


    pthread_mutex_init(&buffer_mutex, NULL);
    pthread_cond_init(&buffer_cond_avail_r, NULL);
    pthread_cond_init(&buffer_cond_avail_w, NULL);


    for (i = 0; i < N; i++) {
       rc=pthread_create(&writer_threads[i], NULL, writer_thread, NULL);
if(rc){
            printf("ERROR");
            exit(-1);
        }
    }

    for (i = 0; i < M; i++) {
        rc=pthread_create(&reader_threads[i], NULL, reader_thread, NULL);
if(rc){
            printf("ERROR");
            exit(-1);
        }
    }

    for (i = 0; i < N; i++) {
        pthread_join(writer_threads[i], NULL);
    }

    for (i = 0; i < M; i++) {
        pthread_join(reader_threads[i], NULL);
    }

    pthread_mutex_destroy(&buffer_mutex);
    pthread_cond_destroy(&buffer_cond_avail_r);
    pthread_cond_destroy(&buffer_cond_avail_w);
    pthread_exit(NULL);
    
}


//:)~
