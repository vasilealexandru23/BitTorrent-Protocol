#include <mpi.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <bits/stdc++.h>
#include "utils.h"

#define TRACKER_RANK 0
#define MAX_FILES 10
#define MAX_FILENAME 15
#define HASH_SIZE 32
#define MAX_CHUNKS 100

void *download_thread_func(void *arg)
{
    int rank = *(int*) arg;
    int numberOwnedFiles;
    int requestedFiles;

    /* Open the coresponding file. */
    std::ifstream fin("in" + std::to_string(rank) + ".txt");
    DIE(!fin.is_open(), "Error opening the file.");

    /* Read content of ownedFiles. */
    fin >> numberOwnedFiles;

    std::vector<struct sendFileData> ownedFiles(numberOwnedFiles);
    for (auto &file : ownedFiles) {
        int numberSegments;

        fin >> file.fileName;
        fin >> numberSegments;

        for (int i = 0; i < numberSegments; ++i) {
            struct Segment newSegment;
            fin >> newSegment.hash;
            newSegment.filePos = i;
            file.Segments.push_back(newSegment);
        }
    }

    /* TODO: send the owned files to the tracker. */

    /* TODO: Wait until we can request files from tracker. */

    /* Read the requested files. */
    fin >> requestedFiles;
    while (requestedFiles--) {

    }

    /* Close the file. */
    fin.close();

    return NULL;
}

void *upload_thread_func(void *arg)
{
    int rank = *(int*) arg;

    return NULL;
}

void tracker(int numtasks, int rank) {
    /* <File, <Segments>> */
    std::unordered_map<std::string, std::vector<struct Segment>> fileSegments;

    /* <Segment, <Clients>> */
    std::map<struct Segment, std::vector<int>> segmentSwarm;

    int recvFromClients = 0;
    while (true) {
        /* TODO: Primeste pachete. */
        recvFromClients++;

        if (recvFromClients == numtasks - 1) {
            break;
        }
    }

    /* TODO: Send ACK to all ranks. */
    for (int i = 1; i <= numtasks; ++i) {

    }

    while (true) {
        /* Primeste pachete si trimite inapoi swarmuri. */
    }
}

void peer(int numtasks, int rank) {
    pthread_t download_thread;
    pthread_t upload_thread;
    void *status;
    int r;

    r = pthread_create(&download_thread, NULL, download_thread_func, (void *) &rank);
    if (r) {
        printf("Eroare la crearea thread-ului de download\n");
        exit(-1);
    }

    r = pthread_create(&upload_thread, NULL, upload_thread_func, (void *) &rank);
    if (r) {
        printf("Eroare la crearea thread-ului de upload\n");
        exit(-1);
    }

    r = pthread_join(download_thread, &status);
    if (r) {
        printf("Eroare la asteptarea thread-ului de download\n");
        exit(-1);
    }

    r = pthread_join(upload_thread, &status);
    if (r) {
        printf("Eroare la asteptarea thread-ului de upload\n");
        exit(-1);
    }
}
 
int main (int argc, char *argv[]) {
    int numtasks, rank;
 
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    if (provided < MPI_THREAD_MULTIPLE) {
        fprintf(stderr, "MPI nu are suport pentru multi-threading\n");
        exit(-1);
    }
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == TRACKER_RANK) {
        tracker(numtasks, rank);
    } else {
        peer(numtasks, rank);
    }

    MPI_Finalize();
}
