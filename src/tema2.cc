#include <mpi.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <bits/stdc++.h>
#include "utils.h"

/* For testing. */
const std::string PATH = "../checker/tests/test1/";

void *download_thread_func(void *arg)
{
    int rank = *(int*) arg;
    int requestedFiles;
    struct sendAllFiles ownedFiles;
    struct packet send, recv;

    /* Open the coresponding file. */
    std::ifstream fin(PATH + "in" + std::to_string(rank) + ".txt");
    DIE(!fin.is_open(), "Error opening the file.");

    /* Read content of ownedFiles. */
    fin >> ownedFiles.numFiles;
    for (int f = 0; f < ownedFiles.numFiles; ++f) {
        fin >> ownedFiles.files[f].fileName;
        fin >> ownedFiles.files[f].numSegments;

        for (int s = 0; s < ownedFiles.files[f].numSegments; ++s) {
            struct Segment newSegment;
            fin >> newSegment.hash;
            newSegment.filePos = s;
            ownedFiles.files[f].Segments[s] = newSegment;
        }
    }

    /* Send the owned files to the tracker. */
    MPI_Send(&ownedFiles, sizeof(sendAllFiles), MPI_BYTE, TRACKER_RANK, 0, MPI_COMM_WORLD);

    /* Wait until we can request files from tracker. */
    MPI_Status status;
    MPI_Recv(&recv, sizeof(packet), MPI_BYTE, TRACKER_RANK, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

    if (recv.type == ACK) {
        std::cout << "Received ACK from tracker.\n";
    }
    
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
    std::map<struct Segment, std::unordered_set<int>> segmentSwarm;

    MPI_Status status;
    int recvFromClients = 0;

    while (true) {
        /* Receive packets from clients. */
        struct sendAllFiles recvOwnedFiles;
        MPI_Recv(&recvOwnedFiles, sizeof(sendAllFiles), MPI_BYTE, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
        recvFromClients++;

        /* Iterate over all files. */
        for (int f = 0; f < recvOwnedFiles.numFiles; ++f) {
            std::string fileName(recvOwnedFiles.files[f].fileName);

            /* Check if we have the segment data already for this file. */
            if (fileSegments[fileName].size() == 0) {
                for (int s = 0; s < recvOwnedFiles.files[f].numSegments; ++s) {
                    struct Segment segment = recvOwnedFiles.files[f].Segments[s];
                    fileSegments[fileName].push_back(segment);
                }
            }

            /* Add the owner of the segment to the swarm. */
            for (int s = 0; s < recvOwnedFiles.files[f].numSegments; ++s) {
                struct Segment segment = recvOwnedFiles.files[f].Segments[s];
                segmentSwarm[segment].insert(status.MPI_SOURCE);
            }
        }

        if (recvFromClients == numtasks - 1) {
            break;
        }
    }

    /* Send ACK to all ranks. */
    struct packet ack = {ACK, NULL};
    for (int i = 1; i <= numtasks - 1; ++i) {
        MPI_Send(&ack, sizeof(packet), MPI_BYTE, i, 0, MPI_COMM_WORLD);
    }

    // while (true) {
    //     /* Primeste pachete si trimite inapoi swarmuri. */
    // }
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
