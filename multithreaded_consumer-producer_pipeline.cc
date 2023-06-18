#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <pthread.h>
#include <assert.h>
#include <math.h> // must link with -lm


/*
A program with a pipeline of 3 threads that interact with each other as producers and consumers.
- Input thread is the first thread in the pipeline. It gets input from the user and puts it in a buffer it shares with the next thread in the pipeline.
- Line separator thread is the second thread in the pipeline. It takes a character from the shared buffer and places it in the next buffer. It replaces newline \n with a space character. 
- Plus sign thread is the third thread in the pipeline. It takes a character from the shared buffer and places it in the next buffer. It replaces ++ with a ^. 
- Output thread is the fourth and final thread in the pipeline. It takes a character from the shared buffer to write to stdout. Stdout only prints with the number of characters is 80.
*/


// Size of the buffers
#define SIZE 50001 

// Buffer 1, shared resource between input thread and line separator thread
char buffer_1[SIZE];
// SHARED index that divides producer and consumer for buffer 1
int div_idx_1 = 0;
// Index where the input thread will put the next item
int prod_idx_1 = 0;
// Index where the line separator thread will pick up the next item
int con_idx_1 = 0;
// Initialize the mutex for buffer 1
pthread_mutex_t mutex_1 = PTHREAD_MUTEX_INITIALIZER;
// Initialize the condition variable for buffer 1
pthread_cond_t ready_1 = PTHREAD_COND_INITIALIZER;


// Buffer 2, shared resource between line separator thread and plus sign thread
char buffer_2[SIZE];
// SHARED index that divides producer and consumer for buffer 2
int div_idx_2 = 0;
// Index where the line separator thread will put the next item
int prod_idx_2 = 0;
// Index where the plus sign thread will pick up the next item
int con_idx_2 = 0;
// Initialize the mutex for buffer 2
pthread_mutex_t mutex_2 = PTHREAD_MUTEX_INITIALIZER;
// Initialize the condition variable for buffer 2
pthread_cond_t ready_2 = PTHREAD_COND_INITIALIZER;


// Buffer 3, shared resource between plus sign thread and output thread
char buffer_3[SIZE];
// SHARED index that divides producer and consumer for buffer 3
int div_idx_3 = 0;
// Index where the plus sign thread will put the next item
int prod_idx_3 = 0;
// Index where the output thread will pick up the next item
int con_idx_3 = 0;
// Initialize the mutex for buffer 2
pthread_mutex_t mutex_3 = PTHREAD_MUTEX_INITIALIZER;
// Initialize the condition variable for buffer 2
pthread_cond_t ready_3 = PTHREAD_COND_INITIALIZER;

int end_marker = 0;  

char output_buffer[80];
int output_idx = 0;
char *inputline = NULL;


/* Put item in buffer 1 */
void put_buff_1(char item){
  	// Put the item in the buffer
  	buffer_1[prod_idx_1] = item;
  	// Increment the index where the next item will be put.
  	prod_idx_1 = prod_idx_1 + 1;
  	// Lock the mutex before changing the dividing index
  	pthread_mutex_lock(&mutex_1);		
  	div_idx_1 = div_idx_1 + 1;
  	// Signal to the consumer that the index has been changed
  	pthread_cond_signal(&ready_1);
  	// Unlock the mutex
  	pthread_mutex_unlock(&mutex_1);
				
}


/* Get the next item from buffer 1 */
char get_buff_1(){
	// Lock the mutex before checking if the buffer has data
  	pthread_mutex_lock(&mutex_1); 
 	while (con_idx_1 == div_idx_1)     
    		pthread_cond_wait(&ready_1, &mutex_1);
  	char item = buffer_1[con_idx_1];
  	// Increment the index from which the item will be picked up
  	con_idx_1 = con_idx_1 + 1;
  	// Unlock the mutex
  	pthread_mutex_unlock(&mutex_1); 
  	// Return the item
  	return item;
}


/* Put item in buffer 2 */
void put_buff_2(char item){
  	// Put the item in the buffer
  	buffer_2[prod_idx_2] = item;
  	// Increment the index where the next item will be put.
  	prod_idx_2 = prod_idx_2 + 1;
  	// Lock the mutex before changing the dividing index
  	pthread_mutex_lock(&mutex_2);		
  	div_idx_2 = div_idx_2 + 1;
  	// Signal to the consumer that the index has been changed
  	pthread_cond_signal(&ready_2);
  	// Unlock the mutex
  	pthread_mutex_unlock(&mutex_2);
}


/* Get the next item from buffer 2 */
char get_buff_2(){
	// Lock the mutex before checking if the buffer has data
  	pthread_mutex_lock(&mutex_2); 
 	while (con_idx_2 == div_idx_2)      
    		pthread_cond_wait(&ready_2, &mutex_2);
  	char item = buffer_2[con_idx_2];
  	// Increment the index from which the item will be picked up
  	con_idx_2 = con_idx_2 + 1;
	if (item == '+') {
		if (buffer_2[con_idx_2] == '+') {
		item = '^';
		con_idx_2 = con_idx_2 + 1;
		}
	}
  	// Unlock the mutex
  	pthread_mutex_unlock(&mutex_2); 
  	// Return the item
  	return item;
}


/* Put item in buffer 3 */
void put_buff_3(char item){
  	// Put the item in the buffer
  	buffer_3[prod_idx_3] = item;
  	// Increment the index where the next item will be put.
  	prod_idx_3 = prod_idx_3 + 1;
  	// Lock the mutex before changing the dividing index
  	pthread_mutex_lock(&mutex_3);		
  	div_idx_3 = div_idx_3 + 1;
  	// Signal to the consumer that the index has been changed
  	pthread_cond_signal(&ready_3);
  	// Unlock the mutex
  	pthread_mutex_unlock(&mutex_3);
}


/* Get the next item from buffer 3 */
char get_buff_3(){
	// Lock the mutex before checking if the buffer has data
  	pthread_mutex_lock(&mutex_3); 
 	while (con_idx_3 == div_idx_3)      
    		pthread_cond_wait(&ready_3, &mutex_3);
  	char item = buffer_3[con_idx_3];
  	// Increment the index from which the item will be picked up
  	con_idx_3 = con_idx_3 + 1;
  	// Unlock the mutex
  	pthread_mutex_unlock(&mutex_3); 
  	// Return the item
  	return item;
}




/* input thread */
void get_input(void *args)
{
	for (;;)
   	 {
	    // receive input
		FILE *input = stdin;
		size_t n = 0;
		ssize_t line_len = getline(&inputline, &n, input);

		// if STOP indicator present, set end_marker
		if (strcmp(inputline, "STOP\n") == 0)
		{
			end_marker = 1;
			return NULL;
		}

		// copies inputline to buffer 1
		for (int p = 0; p < line_len; ++p) {
			put_buff_1(inputline[p]);
		}
	}

}


/* line separator thread */
void *line_sep(void *args) 
{
	for (;;) {

		if (end_marker == 1 && (con_idx_1 == div_idx_1)) {
			end_marker = 2;
			return NULL;
		}

		if (con_idx_1 != div_idx_1) {
      			for (int j = con_idx_1; j < div_idx_1; ++j) { 
				char item = get_buff_1();
        			if (item == '\n') {
          				item = ' ';
				}
				put_buff_2(item);
			}

		}
	}
}


/* plus sign thread */
void *plus_sign(void *args) 
{
	for (;;) {
		if (end_marker == 2 && (con_idx_2 == div_idx_2)) {
			end_marker = 3;
			return NULL;
		}

		if (con_idx_2 != div_idx_2) {
      			for (int t = con_idx_2; t < div_idx_2; ++t) { 
				char item = get_buff_2();
        		put_buff_3(item);		
      			}
    		}
  	}
}



void *output(void *args)      
{
	for (;;) {		
		if ((end_marker == 3) && (con_idx_3 == div_idx_3) && (output_idx != 80)) {
			return NULL;
		}
    		if (con_idx_3 != div_idx_3) {
      			for (int f = con_idx_3; f < div_idx_3; ++f) {         
	      			char item = get_buff_3();
        			output_buffer[output_idx] = item;
        			output_idx = output_idx + 1;
					// when output buffer is full, write to stdout, and reset
        			if (output_idx == 80) {
          				fwrite(output_buffer, 1, 80, stdout);
					    putchar('\n');
	  				    fflush(stdout);
         				memset(output_buffer, 0, 80);
          				output_idx = 0;
        			}
	
      			}
		}
	}
}


int main()
{
    	pthread_t get_input_thread, line_sep_thread, plus_sign_thread, output_thread;
   		// Create the threads
   		pthread_create(&get_input_thread, NULL, get_input, NULL);
    	pthread_create(&line_sep_thread, NULL, line_sep, NULL);
  		pthread_create(&plus_sign_thread, NULL, plus_sign, NULL);
  		pthread_create(&output_thread, NULL, output, NULL);
    	// Wait for the threads to terminate
    	pthread_join(get_input_thread, NULL);
    	pthread_join(line_sep_thread, NULL);
    	pthread_join(plus_sign_thread, NULL);
  		pthread_join(output_thread, NULL);
    	return EXIT_SUCCESS;
}
 
