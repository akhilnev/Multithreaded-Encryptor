# Multi-threaded Encryption Pipeline
Author: Akhilesh Nevatia

## Overview
This project implements a multi-threaded encryption system using a pipeline architecture. The system reads input text, encrypts it, counts character frequencies, and writes the encrypted output to a file, all while maintaining maximum concurrency through careful thread synchronization.

## Architecture
The system uses five concurrent threads operating in a pipeline pattern, with circular buffers for data transfer and semaphores for synchronization.

### Thread Logic

#### 1. Reader Thread
- Reads characters one at a time from input file
- Places each character in the input buffer
- Uses circular buffer to manage input storage
- Signals input counter and encryptor when new data is available
- Waits for both threads to finish before reading next character
- Handles EOF by sending final signals to downstream threads

#### 2. Input Counter Thread
- Monitors input buffer for new characters
- Counts frequency of each character in input stream
- Maintains input character statistics
- Signals completion back to reader
- Coordinates with reset handler for key changes

#### 3. Encryptor Thread
- Reads characters from input buffer
- Encrypts each character using provided encryption function
- Places encrypted characters in output buffer
- Manages synchronization between input and output stages
- Handles EOF propagation to output threads
- Coordinates with output threads for flow control

#### 4. Output Counter Thread
- Monitors output buffer for encrypted characters
- Counts frequency of encrypted characters
- Maintains output character statistics
- Signals completion to encryptor
- Coordinates with reset handler for key changes

#### 5. Writer Thread
- Reads encrypted characters from output buffer
- Writes characters to output file
- Manages output buffer position
- Signals completion to encryptor
- Coordinates with reset handler for key changes

### Main Function Logic
1. Initialization:
   - Validates command line arguments (input, output, log files)
   - Gets buffer sizes from user input
   - Allocates input and output circular buffers
   - Initializes all semaphores and mutex
   - Sets up character counters

2. Thread Management:
   - Creates all five threads
   - Waits for all threads to complete
   - Handles cleanup of resources

3. Reset Handling:
   - Supports encryption key changes during execution
   - Coordinates thread synchronization during reset
   - Manages buffer resets and counter logging

### Synchronization Mechanisms
- Uses semaphores for:
  - Data availability signaling
  - Processing completion notification
  - Reset coordination
- Uses mutex for:
  - Reader thread synchronization during reset
- Uses circular buffers for:
  - Thread-safe data transfer
  - Efficient memory usage

### Buffer Management
- Input Buffer:
  - Size specified by user
  - Managed using circular buffer pattern
  - Synchronized access between reader and consumers

- Output Buffer:
  - Size specified by user
  - Managed using circular buffer pattern
  - Synchronized access between encryptor and output threads

## Compilation and Execution
```bash
# Compile the program
make

# Run the program
./encrypt input_file output_file log_file

# Clean the build
make clean
```
