# op_systems_c

## Disclaimer:
The files in this repository were developed with instruction and guidance from Oregon State University's Operating Systems course (CS 344) in Spring 2023.

## Files include:
libtree.c,
libtree.h,
main.c,
multithreaded_consumer_producer_pipeline.c.

### Tree
Involves files libtree.c, libtree.h, and main.c.
Some skeleton code was provided by OSU CS 344. 
This program uses system calls to access the filesystem, navigate the directory structure, and parse file stats.
A small library is written that parses files and directory structures to list files and directory contents recursively in a tree format.
Each subdirectory is indented from the last. 
Basic sorting is included as well as printing of file permissions, username, group, and file size to display.

### Multithreaded Consumer-Producer Pipeline
Some skeleton code was provided by OSU CS 344. 
This program creates 4 threads to process input from standard input. 
Thread 1 reads in lines of characters from standard input.
Thread 2 replaces every line separator in the input with a space.
Thread 3 replaces every pair of plus signs (++) with a caret (^).
Thread 4 writes the processed data to standard output as lines of exactly 80 characters.
