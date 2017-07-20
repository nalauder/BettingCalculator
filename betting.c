#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#define PRINT 0
#define SAVE_RESULTS 1
#define NUMBER_OF_THREADS 1

int betLimit;

int **odds;
size_t noOdds;


typedef struct {
    pthread_t thread;
    int id;         //Thread id
    int *bets;        //Bet amounts currently being checked
    size_t validNo;    //Number of valid results
#if SAVE_RESULTS
    int **valid;    // Array of array to store valid results
    size_t validSize;     // Size of array holding valid results
#endif
}  threadData;

#if SAVE_RESULTS
void ensureCapacity(threadData *data) {
    if (data->validNo == data->validSize) {
        data->valid = realloc(data->valid, (data->validSize *= 1.2) * sizeof(int*));
        if (data->valid == NULL) {
            puts("Out of memory!");
            exit(1);
        }
        for (int l = data->validNo; l < data->validSize; l++) {
            data->valid[l] = (int*) malloc(sizeof(int) * noOdds);
            if (data->valid[l] == NULL) {
                puts("Out of memory!");
                exit(1);
            }
        }
    }
}
#endif

 /*
  * Recursive function searching all combinations
  * of choices within the given bounds
  *
  * @param data
  *         The threads config struct
  * @param startNo
  *         The starting point of the current threads iteration
  * @param endNo
  *         The ending point of the current threads iteration
  * @param optionsNo
  *         The recessive level of the thread.
  *         The first odd is level 0, the second odd is 1 etc...
  * @param currBet
  *         The current set bet made of of the previous levels
  */
void playOption(threadData *data, int startNo, int endNo, int optionsNo, int currBet) {

    for(int i = startNo; i <= endNo-currBet; i++) {
        data->bets[optionsNo] = i;
        int totalbet = currBet + i;

        if (optionsNo >= noOdds-1) { // If this recursion is now checking the last option

            char profit = 1;
            for (int j = 0; j < noOdds; j++) {

                // Records how much under the break even amount we are
                double diff = data->bets[j]*odds[j][0] + data->bets[j]*(odds[j][1]/100.0) - totalbet;
                if (diff < 0) {
                    // Skip entire option if one of the initial options cannot succeed
                    if (j != optionsNo) { return; }
                    profit = 0;

                    // Skips to the next level where we would break even
                    i -= 1/(odds[j][0]/diff + (odds[j][1]/100.0)/diff);
                }
            }

            if (profit) {
                data->validNo++;
#if SAVE_RESULTS
                // Adds options to the valid choices array
                for (int k = 0; k < noOdds; k++)
                    data->valid[data->validNo-1][k] = data->bets[k];
                // Ensures array does not run out of memory
                ensureCapacity(data);
#endif
            }
        }

        if (optionsNo < noOdds-1) {
            //Runs the next option recursively
            playOption(data, 1, betLimit, optionsNo+1, totalbet);
        }
    }

}
 /*
  * Handles the thread execution
  * calculates the starting and end search point
  * @param argv
  *         the threadData struct containing all the threads config
  */
void *threadHandler(void *argv) {
    threadData *data = argv;
    playOption(data, (betLimit/NUMBER_OF_THREADS)*(data->id)+1, (betLimit/NUMBER_OF_THREADS)*((data->id)+1), 0, 0);
    pthread_exit(NULL);
    return NULL;
}



int main(int argc, char *argv[]) {

    if (argc < 4) {
        printf( "Program usage:\r\n\t%s <betting limit> <odd 1> <odd 2> ...\r\n\t"
                "Any number of odds can be added\r\n\t"
                "Odds much be no more accurate than 2dp\r\n", argv[0]);
        exit(1);
    }

    betLimit = atoi(argv[1]);
    noOdds = argc-2;
    odds = malloc(noOdds * sizeof(int*));
    for (int i = 0; i < argc-2; i++) {

        // Odds stored is in array 1st element is integer, second element is decimal
        odds[i] = (int*) calloc (2, sizeof(int));
        int decimal1 = 0, decimal2 = 0;
        sscanf(argv[i+2], "%d.%1d%1d", &odds[i][0], &decimal1, &decimal2);
        odds[i][1] = decimal1*10 + decimal2;

    }

    clock_t start = clock();
    threadData threads[NUMBER_OF_THREADS];
    for (int i = 0; i < NUMBER_OF_THREADS; i++) {
        threads[i].id = i;
        threads[i].validNo = 0;
        threads[i].bets = (int*) malloc(sizeof(int) * noOdds);
#if SAVE_RESULTS
        threads[i].validSize = 10;

        //Allocating default memory to store the array of arrays
        threads[i].valid = (int**) malloc(threads[i].validSize * sizeof(int*));
        for (int j = 0; j < threads[i].validSize; j++) {
            threads[i].valid[j] = (int*) malloc(sizeof(int) * noOdds);
        }
#endif
        pthread_create(&threads[i].thread, NULL, threadHandler, (void *)&threads[i]);
    }
    for (int i = 0; i < NUMBER_OF_THREADS; i++) {
        pthread_join(threads[i].thread, NULL);
    }
    clock_t finish = clock();

#if SAVE_RESULTS
#if PRINT
    for (int p = 0; p < NUMBER_OF_THREADS; p++) {
        for (int i = 0; i < threads[p].validNo; i++) {
            int j;
            for (j = 0, printf("%d - [%d", threads[p].id, threads[p].valid[i][j]); j < noOdds-1; \
                                            printf(", %d", threads[p].valid[i][++j]));
            printf("]\r\n");
        }
    }
#endif
#endif

    printf("time = %f\r\n", (double)(finish - start)/CLOCKS_PER_SEC);

    size_t total = 0;
    for (int i = 0; i < NUMBER_OF_THREADS; total += threads[i++].validNo);
    printf("Valid solutions: %d\r\n", total);

    return 0;
}

