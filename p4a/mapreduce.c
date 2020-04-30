#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include "mapreduce.h"

struct pairs {
	char *key;
	char *value;
};

struct files {
	char *name;
};

struct pairs **partitions;
struct pairs **mapArray;
struct files *fileNames;
int* pairCountInMap;
int* pairAllocatedInMap;
int* numberOfAccessInMap;
int* pairCountInPartition;
int* pairAllocatedInPartition;
int* numberOfAccessInPartition;
long* tidArray;

pthread_mutex_t lock, fileLock;
Partitioner p;
Combiner c;
Reducer r;
Mapper m;
int numberPartitions;
int numberMaps;
int filesProcessed;
int totalFiles;

// Sort files by increasing size
int compareFiles(const void* p1, const void* p2) {
	struct files *f1 = (struct files*) p1;
	struct files *f2 = (struct files*) p2;
	struct stat st1, st2;
	stat(f1->name, &st1);
	stat(f2->name, &st2);
	long int size1 = st1.st_size;
	long int size2 = st2.st_size;
	return (size1 - size2);
}

int getIndex(long value, long* array, int len) {
	for (int i = 0; i < len; i++) {
		if (array[i] == 0) {
			array[i] = value;
			return i;
		} 
		if (array[i] == value)
			return i;
	}
	return -1;
}

// Sort the buckets by key and then by value in ascending order
int compare(const void* p1, const void* p2) {
	struct pairs *pair1 = (struct pairs*) p1;
	struct pairs *pair2 = (struct pairs*) p2;
	if(strcmp(pair1->key, pair2->key) == 0) {
		return strcmp(pair1->value, pair2->value);
	}
	return strcmp(pair1->key, pair2->key);
}

char* combiner_get_next(char *key) {
	long tid = (long)pthread_self();
	int index = getIndex(tid, tidArray, numberMaps);
	int num = numberOfAccessInMap[index];
	if(num < pairCountInMap[index] && strcmp(key, mapArray[index][num].key) == 0) {
		numberOfAccessInMap[index]++;
		return mapArray[index][num].value;
	}
	else {
		return NULL;
	}
}

void* mapperHelper(void *arg) {
	while(filesProcessed < totalFiles) {
		pthread_mutex_lock(&fileLock);
		char *filename = NULL;
		if(filesProcessed < totalFiles) {
			filename = fileNames[filesProcessed].name;
			filesProcessed++;
		}
		pthread_mutex_unlock(&fileLock);
		if(filename != NULL)
			m(filename);
	}
	if (c == NULL) {
		return arg;
	} else {
		long tid = (long)pthread_self();
		int index = getIndex(tid, tidArray, numberMaps);
		qsort(mapArray[index], pairCountInMap[index], sizeof(struct pairs), compare)	;
		for(int i = 0; i < pairCountInMap[index]; i++) {
			if(i == numberOfAccessInMap[index]) {
				c(mapArray[index][i].key, combiner_get_next);
			}
		}
		return arg;
	}
}


char* reducer_get_next(char *key, int partition_number) {
	int num = numberOfAccessInPartition[partition_number];
	if(num < pairCountInPartition[partition_number] && strcmp(key, partitions[partition_number][num].key) == 0) {
		numberOfAccessInPartition[partition_number]++;
		return partitions[partition_number][num].value;
	}
	else {
		return NULL;
	}
}


void* reducerHelper(void *arg) {
	int* partitionNumber = (int *)arg;
	for(int i = 0; i < pairCountInPartition[*partitionNumber]; i++) {
		if(i == numberOfAccessInPartition[*partitionNumber]) {
			r(partitions[*partitionNumber][i].key, NULL, reducer_get_next, *partitionNumber);
		}
	}
	return arg;
}


void MR_EmitToCombiner(char *key, char *value) {
	if (c == NULL) {
		pthread_mutex_lock(&lock); 
		// Getting the partition number
		unsigned long hashPartitionNumber = p(key, numberPartitions);
		pairCountInPartition[hashPartitionNumber]++;
		int curCount = pairCountInPartition[hashPartitionNumber];
		// Checking if allocated memory has been exceeded,if yes allocating more 	memory
		if (curCount > pairAllocatedInPartition[hashPartitionNumber]) {
			pairAllocatedInPartition[hashPartitionNumber] *= 2;
			partitions[hashPartitionNumber] = (struct pairs *) realloc(partitions	[hashPartitionNumber], pairAllocatedInPartition[hashPartitionNumber] * 	sizeof(struct pairs));
		}
		partitions[hashPartitionNumber][curCount-1].key = (char*)malloc((strlen(key)	+1) * sizeof(char));
		strcpy(partitions[hashPartitionNumber][curCount-1].key, key);
		partitions[hashPartitionNumber][curCount-1].value = (char*)malloc((strlen	(value)+1) * sizeof(char));
		strcpy(partitions[hashPartitionNumber][curCount-1].value, value);
		pthread_mutex_unlock(&lock); 
	} else {
		pthread_mutex_lock(&lock); 
		long tid = (long)pthread_self();
		int index = getIndex(tid, tidArray, numberMaps);
		pairCountInMap[index]++;
		int curCount = pairCountInMap[index];
		// Checking if allocated memory has been exceeded,if yes allocating more 	memory
		if (curCount > pairAllocatedInMap[index]) {
			pairAllocatedInMap[index] *= 2;
			mapArray[index] = (struct pairs *) realloc(mapArray[index], 	pairAllocatedInMap[index] * sizeof(struct pairs));
		}
		mapArray[index][curCount-1].key = (char*)malloc((strlen(key)+1) * sizeof	(char));
		strcpy(mapArray[index][curCount-1].key, key);
		mapArray[index][curCount-1].value = (char*)malloc((strlen(value)+1) * sizeof	(char));
		strcpy(mapArray[index][curCount-1].value, value);
		pthread_mutex_unlock(&lock); 
	}
}


void MR_EmitToReducer(char *key, char *value) {
	pthread_mutex_lock(&lock); 
	// Getting the partition number
	unsigned long hashPartitionNumber = p(key, numberPartitions);
	pairCountInPartition[hashPartitionNumber]++;
	int curCount = pairCountInPartition[hashPartitionNumber];
	// Checking if allocated memory has been exceeded,if yes allocating more memory
	if (curCount > pairAllocatedInPartition[hashPartitionNumber]) {
		pairAllocatedInPartition[hashPartitionNumber] *= 2;
		partitions[hashPartitionNumber] = (struct pairs *) realloc(partitions[hashPartitionNumber], pairAllocatedInPartition[hashPartitionNumber] * sizeof(struct pairs));
	}
	partitions[hashPartitionNumber][curCount-1].key = (char*)malloc((strlen(key)+1) * sizeof(char));
	strcpy(partitions[hashPartitionNumber][curCount-1].key, key);
	partitions[hashPartitionNumber][curCount-1].value = (char*)malloc((strlen(value)+1) * sizeof(char));
	strcpy(partitions[hashPartitionNumber][curCount-1].value, value);
	pthread_mutex_unlock(&lock); 
}

void MR_Run(int argc, char *argv[], Mapper map, int num_mappers, Reducer reduce, int num_reducers, Combiner combine, Partitioner partition) {
    totalFiles = argc - 1;
    if(totalFiles < num_mappers) {
		num_mappers = totalFiles;
	}
    pthread_t mapperThreads[num_mappers];
	pthread_t reducerThreads[num_reducers];
	pthread_mutex_init(&lock, NULL);
	pthread_mutex_init(&fileLock, NULL);
	p = partition;
	m = map;
	c = combine;
	r = reduce;
    numberPartitions = num_reducers;
	numberMaps = num_mappers;
    partitions = malloc(num_reducers * sizeof(struct pairs*));
	mapArray = malloc(num_mappers * sizeof(struct pairs*));
	tidArray = malloc(num_mappers * sizeof(long));
    fileNames = malloc(totalFiles * sizeof(struct files));
    pairCountInPartition = malloc(num_reducers * sizeof(int));
	pairAllocatedInPartition = malloc(num_reducers * sizeof(int));
	numberOfAccessInPartition = malloc(num_reducers * sizeof(int));
	pairCountInMap = malloc(num_mappers * sizeof(int));
	pairAllocatedInMap = malloc(num_mappers * sizeof(int));
	numberOfAccessInMap = malloc(num_mappers * sizeof(int));
    filesProcessed = 0;
    int arrayPosition[num_reducers];
    // Initialising the arrays needed to store the key value pairs in the partitions
	for (int i = 0; i < num_mappers; i++) {
		mapArray[i] = malloc(1024 * sizeof(struct pairs));
		pairCountInMap[i] = 0;
		pairAllocatedInMap[i] = 1024;
		numberOfAccessInMap[i] = 0;
		tidArray[i] = 0;
	}
	
	for(int i = 0; i < num_reducers; i++) {
		partitions[i] = malloc(1024 * sizeof(struct pairs));
		pairCountInPartition[i] = 0;
		pairAllocatedInPartition[i] = 1024;
		arrayPosition[i] = i;
		numberOfAccessInPartition[i] = 0;
	}

    // Copying files for sorting in struct
	for(int i = 0; i < argc-1; i++) {
		fileNames[i].name = malloc((strlen(argv[i+1])+1) * sizeof(char));
		strcpy(fileNames[i].name, argv[i+1]);
	}
    // Sorting files as Shortest File first
	qsort(&fileNames[0], argc-1, sizeof(struct files), compareFiles);
    // Creating the threads for the number of mappers
	for (int i = 0; i < num_mappers; i++) {
		pthread_create(&mapperThreads[i], NULL, mapperHelper, NULL);
	}

    // Waiting for threads to finish
	for(int i = 0; i < num_mappers; i++) {
		pthread_join(mapperThreads[i], NULL); 
	}

	// Sorting the partitions
	for(int i = 0; i < num_reducers; i++) {
		qsort(partitions[i], pairCountInPartition[i], sizeof(struct pairs), compare);
	}

	// Creating the threads for the number of reducers
	for (int i = 0; i < num_reducers; i++){
	    if(pthread_create(&reducerThreads[i], NULL, reducerHelper, &arrayPosition[i])) {
	    	printf("Error\n");
	    }
	}

	//Waiting for the threads to finish
	for(int i = 0; i < num_reducers; i++) {
		pthread_join(reducerThreads[i], NULL); 
	}

	pthread_mutex_destroy(&lock);
	pthread_mutex_destroy(&fileLock);
	for(int i = 0; i < num_reducers; i++) {
		// Freeing the keys and values
		for(int j = 0; j < pairCountInPartition[i]; j++) {
			if(partitions[i][j].key != NULL && partitions[i][j].value != NULL) {
				free(partitions[i][j].key);
		    	free(partitions[i][j].value);
			}
		}
		// Freeing the pair struct array
		free(partitions[i]);
	}
	for(int i = 0; i < num_mappers; i++) {
		// Freeing the keys and values
		for(int j = 0; j < pairCountInMap[i]; j++) {
			if(mapArray[i][j].key != NULL && mapArray[i][j].value != NULL) {
				free(mapArray[i][j].key);
		    	free(mapArray[i][j].value);
			}
		}
		// Freeing the pair struct array
		free(mapArray[i]);
	}
	// Freeing filenames
	for(int i = 0; i < argc-1; i++) {
		free(fileNames[i].name);
	}

	// Freeing memory
	free(partitions);
	free(fileNames);
	free(mapArray);
	free(pairCountInPartition);
	free(pairAllocatedInPartition);
	free(numberOfAccessInPartition);
	free(pairCountInMap);
	free(pairAllocatedInMap);
	free(numberOfAccessInMap);
	free(tidArray);
}

//Default hash function
unsigned long MR_DefaultHashPartition(char *key, int num_partitions) {
    unsigned long hash = 5381;
    int c;
    while ((c = *key++) != '\0')
        hash = hash * 33 + c;
    return hash % num_partitions;
}
