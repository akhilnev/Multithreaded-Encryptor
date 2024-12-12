/*
 * Author:@akhilnev / Akhilesh Saket Nevatia
 * 
 * Multi-threaded Encryption Pipeline Project
 * 
 * This project implements a multi-threaded encryption system using a pipeline architecture
 * with five concurrent threads: reader, input counter, encryptor, output counter, and writer.
 * The system uses circular buffers and semaphores for thread synchronization, allowing
 * maximum concurrency while maintaining data consistency. The pipeline processes input text,
 * encrypts it, counts character frequencies, and writes the encrypted output to a file.
 * 
 * Usage:
 * Compile the program : $make 
 * Run the program: $./encrypt input_file output_file log_file
 * Clean the build: $make clean
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#include "encrypt-module.h"

// buffer counts
int input_count, output_count;

// input and output buffers
char* input_buffer;
char* output_buffer;

// Semaphores for signaling data availability in buffers
sem_t reader_data_ready, counter_data_ready, encryptor_data_ready, output_counter_data_ready, writer_data_ready;
// Semaphores for signaling processing completion
sem_t counter_done, encryptor_done, output_counter_done, writer_done;
// Semaphores for reset coordination
sem_t counter_reset_ready, encryptor_reset_ready, output_counter_reset_ready, writer_reset_ready;

// input and output buffer sizes, respectively
int n, m;

pthread_mutex_t reader_lock; // locks reader for reset purposes

// Add buffer tracking variables at the top with other globals
int input_buffer_in = 0;  // Track write position
int input_buffer_out = 0; // Track read position
int output_buffer_in = 0; // Track write position
int output_buffer_out = 0;// Track read position

// Reads each character of the file and puts it into the input buffer
void* readerThread() {
    char c;
    do {
        pthread_mutex_lock(&reader_lock);
        c = read_input();

        // Write to the next input position
        input_buffer[input_buffer_in] = c;
        input_buffer_in = (input_buffer_in + 1) % n;

        // Signal threads waiting for input
        sem_post(&counter_data_ready);
        sem_post(&encryptor_data_ready);
        
        // Wait for processing to complete
        sem_wait(&counter_done);
        sem_wait(&encryptor_done);

        pthread_mutex_unlock(&reader_lock);
    } while (c != EOF);

    // Signal one more time to ensure other threads process EOF
    sem_post(&counter_data_ready);
    sem_post(&encryptor_data_ready);
    return NULL;
}

// reads each character in input buffer as they are entered and adds it to the count
void* inputCounterThread() {
    char c;
    do {
        sem_wait(&counter_data_ready);
        c = input_buffer[input_buffer_out];
        sem_post(&counter_done);
        sem_post(&counter_reset_ready);

        if(c != EOF) {
            count_input(c);
        }
    } while (c != EOF);
    return NULL;
}

// gets character from input, encrypts it, and places it into output
void* encryptorThread() {
    char c;
    do {
        sem_wait(&encryptor_data_ready);
        c = input_buffer[input_buffer_out];
        input_buffer_out = (input_buffer_out + 1) % n;
        sem_post(&encryptor_done);

        if (c != EOF) {
            output_buffer[output_buffer_in] = encrypt(c);
            output_buffer_in = (output_buffer_in + 1) % m;
            
            // Signal output processing
            sem_post(&output_counter_data_ready);
            sem_post(&writer_data_ready);
        }
        sem_post(&encryptor_reset_ready);

        // Wait for output processing only if not EOF
        if (c != EOF) {
            sem_wait(&output_counter_done);
            sem_wait(&writer_done);
        }
    } while (c != EOF);

    // Signal EOF to output threads
    output_buffer[output_buffer_in] = EOF;
    output_buffer_in = (output_buffer_in + 1) % m;
    sem_post(&output_counter_data_ready);
    sem_post(&writer_data_ready);
    return NULL;
}

// reads output from output buffer and counts for logging
void* outputCounterThread(void* arg) {
    char c;
    do {
        sem_wait(&output_counter_data_ready);
        c = output_buffer[output_buffer_out];
        if(c != EOF) {
            count_output(c);
        }
        sem_post(&output_counter_done);
        sem_post(&output_counter_reset_ready);
    } while (c != EOF);
    return NULL;
}

// reads output buffer and places it into output file
void* writerThread(void* arg) {
    char c;
    do {
        sem_wait(&writer_data_ready);
        c = output_buffer[output_buffer_out];
        output_buffer_out = (output_buffer_out + 1) % m;
        
        if(c != EOF) {
            write_output(c);
        }
        sem_post(&writer_done);
        sem_post(&writer_reset_ready);
    } while (c != EOF);
    return NULL;
}

// when reset is called, locks reader, waits for other threads to catch up, resets buffer counter and logs result for current key
void reset_requested() {
    pthread_mutex_lock(&reader_lock);
    sem_wait(&counter_reset_ready);
    sem_wait(&encryptor_reset_ready);
    sem_wait(&output_counter_reset_ready);
    sem_wait(&writer_reset_ready);
    
    // Reset all buffer positions
    input_buffer_in = 0;
    input_buffer_out = 0;
    output_buffer_in = 0;
    output_buffer_out = 0;
    input_count = 0;
    output_count = 0;
    
    log_counts();
}

// when called, unlocks reader
void reset_finished() {
    pthread_mutex_unlock(&reader_lock);
}

int main(int argc, char *argv[]) {
    // checks for exactly 4 arguments
    if (argc != 4) {
        printf("Error: Must include an input file, an output file, and a log file in arguments\n");
        exit(1);
    }

    // initializes files for encrypt module
    init(argv[1], argv[2], argv[3]);

    // loops until receiving valid buffer input
    do {
        printf("\nEnter buffer size for the input: ");
        scanf("%d", &n);
        if (n < 1) printf("\nInvalid buffer, must be greater than 0\n");
    } while(n < 1);
    
    do {
        printf("Enter buffer size for the output: ");
        scanf("%d", &m);
        if (m < 1) printf("\nInvalid buffer, must be greater than 0\n");
    } while(m < 1);

    // allocates buffer size
    input_buffer = malloc(sizeof(char) * n);
    output_buffer = malloc(sizeof(char) * m); 
    if (!input_buffer || !output_buffer) {
        printf("Error: Memory allocation failed\n");
        exit(1);
    }

    // keeps track of buffer count
    input_count = 0;
    output_count = 0;

    // Initialize semaphores for data availability
    sem_init(&reader_data_ready, 0, 0);
    sem_init(&counter_data_ready, 0, 0);
    sem_init(&encryptor_data_ready, 0, 0);
    sem_init(&output_counter_data_ready, 0, 0);
    sem_init(&writer_data_ready, 0, 0);

    // Initialize semaphores for processing completion
    sem_init(&counter_done, 0, 0);
    sem_init(&encryptor_done, 0, 0);
    sem_init(&output_counter_done, 0, 0);
    sem_init(&writer_done, 0, 0);

    // Initialize semaphores for reset coordination
    sem_init(&counter_reset_ready, 0, 0);
    sem_init(&encryptor_reset_ready, 0, 0);
    sem_init(&output_counter_reset_ready, 0, 0);
    sem_init(&writer_reset_ready, 0, 0);

    // initializes reader mutex
    pthread_mutex_init(&reader_lock, NULL);
    
    // creates threads
    pthread_t reader, input_counter, encryptor, output_counter, writer;
    pthread_create(&reader, NULL, readerThread, NULL);
    pthread_create(&input_counter, NULL, inputCounterThread, NULL);
    pthread_create(&encryptor, NULL, encryptorThread, NULL);
    pthread_create(&output_counter, NULL, outputCounterThread, NULL);
    pthread_create(&writer, NULL, writerThread, NULL);

    // runs threads
    pthread_join(reader, NULL);
    pthread_join(input_counter, NULL);
    pthread_join(encryptor, NULL);
    pthread_join(output_counter, NULL);
    pthread_join(writer, NULL);

    // destroys reader lock
    pthread_mutex_destroy(&reader_lock);

    // Cleanup semaphores for data availability
    sem_destroy(&reader_data_ready);
    sem_destroy(&counter_data_ready);
    sem_destroy(&encryptor_data_ready);
    sem_destroy(&output_counter_data_ready);
    sem_destroy(&writer_data_ready);

    // Cleanup semaphores for processing completion
    sem_destroy(&counter_done);
    sem_destroy(&encryptor_done);
    sem_destroy(&output_counter_done);
    sem_destroy(&writer_done);

    // Cleanup semaphores for reset coordination
    sem_destroy(&counter_reset_ready);
    sem_destroy(&encryptor_reset_ready);
    sem_destroy(&output_counter_reset_ready);
    sem_destroy(&writer_reset_ready);

    // free allocated memory
    free(input_buffer);
    free(output_buffer);

    printf("\nEnd of file reached.\n"); 
    log_counts();
    return 0;
}
