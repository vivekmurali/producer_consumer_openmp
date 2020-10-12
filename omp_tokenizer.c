/*
Producer-consumer tokenizer
Made by Vivek Murali on 10-10-2020
Done for Parallael and distributed computing lab case study
Lab slot - L11+L12

*/
#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#include <stdbool.h>
#include <time.h>
#include "queue.h"

#define NUM_FILES 4
#define QUEUE_SIZE 256
#define MAX_FNAME 16
#define TOKEN_SEPARATOR ' '

Queue queues[NUM_FILES];
int jobs_left;

/*
Checks if an index is at the end of the string called <line>
*/
bool isEndOfLine(char *line, int idx, int size)
{
    return (idx >= size || line[idx] == EOF || line[idx] == '\n');
}

/*
produce_to_queue is run by producer threads, 
which read a line from a file and append it to a queue
*/
void produce_to_queue(char fname[])
{
    int thread_num = omp_get_thread_num();
    FILE *file_pointer = NULL;
    int r;
    file_pointer = fopen(fname, "r");
    char line[256];

    // Reads lines from a file until EOF is reached
    while (fgets(line, sizeof(line), file_pointer))
    {
        // This loop attempts to access a random queue,
        // until a free queue is found
        r = rand() % NUM_FILES;
        bool have_occupied_a_queue = false;
        while (!have_occupied_a_queue)
        {

#pragma omp critical
            have_occupied_a_queue = occupyQueue(&queues[r]);
        }

#pragma omp critical
        insert(&queues[r], line);
#pragma omp flush

        leaveQueue(&queues[r]);
    }
#pragma omp critical
    jobs_left--;
}

/*
consume_from_queue is run by consumer threads, and each watch a respective queue
for new lines, and tokenizes the words within the line
*/
void consume_from_queue(int my_rank)
{
    int thread_num = omp_get_thread_num();
    char line[256];
    int max_chars = sizeof(line) - 1;

    while (true)
    {
        // Repeatedly checks if any jobs are left, and while there are, waits
        // for new lines in the queue
        while (isEmpty(&queues[my_rank]))
        {
            if (jobs_left <= 0)
                return;
        }

#pragma omp critical
        pop(&queues[my_rank], line);

        // tokenizes the line into words
        int idx = 0;
        while (!isEndOfLine(line, idx, max_chars))
        {
            if (line[idx] == ' ')
            {
                line[idx] = '\n';
            }
            idx++;
        }
#pragma omp critical
        printf("%s", line);

        leaveQueue(&queues[my_rank]);
    }
}

/*
runs the main tokinizer program, 
which assigns threads to a producer or consumer job
*/
void tokenizeWords()
{
    jobs_left = NUM_FILES;
    clock_t seconds = clock();
    for (int i = 0; i < NUM_FILES; i++)
    {
        initializeQueue(&queues[i]);
    }

#pragma omp parallel
    {
#pragma omp for schedule(static) nowait
        for (int i = 0; i < NUM_FILES; i++)
        {
            char fname[16];
            sprintf(fname, "./texts/%d.txt", i);
            produce_to_queue(fname);
        }
#pragma omp for schedule(static)
        for (int i = 0; i < NUM_FILES; i++)
        {
            char fname[16];
            consume_from_queue(i);
        }
    }
}

int main()
{
    tokenizeWords();
}
