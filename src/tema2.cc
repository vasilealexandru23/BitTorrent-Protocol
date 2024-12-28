#include <mpi.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <bits/stdc++.h>
#include "utils.h"

/* Keep local files for interogation. */
std::unordered_map<std::string, std::vector<struct Segment>> myFiles;

/* Requested file names. */
std::vector<std::string> requestedFiles;

void *download_thread_func(void *arg)
{
    int rank = *(int*) arg;
    MPI_Status status;

    /* Read the requested files. */
    for (auto file : requestedFiles) {
        char requestedFile[MAX_FILENAME + 1] = {0};
        strcpy(requestedFile, file.c_str());

        /* Request segments from tracker. */
        MPI_Send(requestedFile, MAX_FILENAME + 1, MPI_BYTE, TRACKER_RANK, TRACKER_FILE_SEGMENTS, MPI_COMM_WORLD);
        struct FileData fileData;
        MPI_Recv(&fileData, sizeof(FileData), MPI_BYTE, TRACKER_RANK, TRACKER_FILE_SEGMENTS, MPI_COMM_WORLD, &status);

        DIE(strcmp(fileData.fileName, requestedFile) != 0, "File name mismatch.");

        int numSegments = fileData.numSegments;
        struct Segment *file_segments = fileData.Segments;
        struct swarm_t swarm;
        int numClients;
        int *clients;

        int recvSegments = 0;
        while (true) {
            if (recvSegments % MAX_SEG_RUN == 0) {
                /* Request the swarm list update. */
                MPI_Send(requestedFile, MAX_FILENAME + 1, MPI_BYTE, TRACKER_RANK, SEND_FILE_SWARM, MPI_COMM_WORLD);
                MPI_Recv(&swarm, sizeof(swarm_t), MPI_BYTE, TRACKER_RANK, SEND_FILE_SWARM, MPI_COMM_WORLD, &status);
                numClients = swarm.numClients;
                clients = swarm.clients;
            }

            /* Check every client and find the one with loawest number of uploads. */
            int bestClient = -1;
            int lowestNumberUpload = INT_MAX;
            struct askSegment toAsk;
            strcpy(toAsk.fileName, requestedFile);
            toAsk.segment = file_segments[recvSegments];
            toAsk.op = ASK;

            for (int i = 0; i < numClients; ++i) {
                MPI_Send(&toAsk, sizeof(askSegment), MPI_BYTE, clients[i], REQUEST, MPI_COMM_WORLD);
                int response;
                MPI_Recv(&response, sizeof(int), MPI_BYTE, clients[i], RESPONSE, MPI_COMM_WORLD, &status);
                if (response != -1 && response < lowestNumberUpload) {
                    lowestNumberUpload = response;
                    bestClient = clients[i];
                }
            }

            toAsk.op = DOWNLOAD;

            if (bestClient == -1) {
                std::cout << "ERROR: No client has the segment. RETRY...\n";
                continue;
            } else {
                MPI_Send(&toAsk, sizeof(askSegment), MPI_BYTE, bestClient, REQUEST, MPI_COMM_WORLD);
                struct packet response;
                MPI_Recv(&response, sizeof(packet), MPI_BYTE, bestClient, RESPONSE, MPI_COMM_WORLD, &status);
                if (response.type == ACK) {
                    /* Mark segment as downloaded and increment the index of the current segment.*/
                    myFiles[requestedFile].push_back(file_segments[recvSegments++]);
                    if (recvSegments == 1) {
                        /* Say to tracker that I am now peer. */
                        MPI_Send(requestedFile, MAX_FILENAME + 1, MPI_BYTE, TRACKER_RANK, I_AM_PEER, MPI_COMM_WORLD);
                    }
                } else {
                    std::cout << "ERROR: Client didn't send the segment. RETRY...\n";
                    continue;
                }
            }

            if (recvSegments == numSegments) {
                /* Say to tracker that I finished my download and make me seed. */
                MPI_Send(requestedFile, MAX_FILENAME + 1, MPI_BYTE, TRACKER_RANK, I_AM_SEED, MPI_COMM_WORLD);

                /* Create the file with the rank and file name with segments. */
                std::ofstream fout("client" + std::to_string(rank) + "_" + requestedFile);
                DIE(!fout.is_open(), "Error opening the file.");

                for (auto segment : myFiles[requestedFile]) {
                    fout << segment.hash << '\n';
                }

                fout.close();
                break;
            }
        }
    }

    /* Send to tracker the message that all files are installed. */
    MPI_Send(NULL, 0, MPI_BYTE, TRACKER_RANK, DONE_MY_DOWNLOAD, MPI_COMM_WORLD);

    return NULL;
}

void *upload_thread_func(void *arg)
{
    int numberUploads = 0;

    MPI_Status status;
    void *recvData = calloc(MAX_PACKET_SIZE, 1);

    while (true) {
        /* Receive packets until I get from tracker (ONLY MESSAGE I CAN GET IS TO STOP). */
        MPI_Recv(recvData, MAX_PACKET_SIZE, MPI_BYTE, MPI_ANY_SOURCE, REQUEST, MPI_COMM_WORLD, &status);
        if (status.MPI_SOURCE == TRACKER_RANK) {
            break;
        } else {
            struct askSegment askSegment = *(struct askSegment *) recvData;
            if (askSegment.op == ASK) {
                if (find(myFiles[askSegment.fileName].begin(), myFiles[askSegment.fileName].end(), askSegment.segment) != myFiles[askSegment.fileName].end()) {
                    /* Send the number of uploads. */
                    MPI_Send(&numberUploads, sizeof(int), MPI_BYTE, status.MPI_SOURCE, RESPONSE, MPI_COMM_WORLD);
                } else {
                    /* Send -1 to signal that I don't have the segment. */
                    int response = -1;
                    MPI_Send(&response, sizeof(int), MPI_BYTE, status.MPI_SOURCE, RESPONSE, MPI_COMM_WORLD);
                }
            } else if (askSegment.op == DOWNLOAD) {
                if (find(myFiles[askSegment.fileName].begin(), myFiles[askSegment.fileName].end(), askSegment.segment) != myFiles[askSegment.fileName].end()) {
                    struct packet ack = {ACK, NULL};
                    MPI_Send(&ack, sizeof(packet), MPI_BYTE, status.MPI_SOURCE, RESPONSE, MPI_COMM_WORLD);
                    numberUploads++;
                } else {
                    struct packet nack = {NACK, NULL};
                    MPI_Send(&nack, sizeof(packet), MPI_BYTE, status.MPI_SOURCE, RESPONSE, MPI_COMM_WORLD);
                }
            }
        }
    }

    return NULL;
}

void tracker(int numtasks, int rank) {
    /* <File, <Segments>> */
    std::unordered_map<std::string, std::vector<struct Segment>> fileSegments;

    /* <File, <Owner's rank>> */
    std::unordered_map<std::string, std::vector<client_t>> fileSwarm;

    MPI_Status status;
    int recvFromClients = 0;

    while (recvFromClients != numtasks - 1) {
        /* Receive packets from clients. */
        struct sendAllFiles recvOwnedFiles;
        MPI_Recv(&recvOwnedFiles, sizeof(sendAllFiles), MPI_BYTE, MPI_ANY_SOURCE, TRACKER_BARRIER, MPI_COMM_WORLD, &status);
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

            /* Add the owner of the file to the swarm. */
            fileSwarm[fileName].push_back({status.MPI_SOURCE, SEED});
        }
    }

    /* Send ACK to all ranks. */
    struct packet ack = {ACK, NULL};
    for (int i = 1; i <= numtasks - 1; ++i) {
        MPI_Send(&ack, sizeof(packet), MPI_BYTE, i, TRACKER_BARRIER, MPI_COMM_WORLD);
    }

    int finishedClients = 0;
    void *recvData = calloc(MAX_PACKET_SIZE, 1);
    while (true) {
        MPI_Recv(recvData, MAX_PACKET_SIZE, MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status); 
        if (status.MPI_TAG == TRACKER_FILE_SEGMENTS) {
            std::string requestedFile((char *) recvData);
            struct FileData fileData;
            fileData.numSegments = fileSegments[requestedFile].size();
            strcpy(fileData.fileName, requestedFile.c_str());
            for (int i = 0; i < fileData.numSegments; ++i) {
                fileData.Segments[i] = fileSegments[requestedFile][i];
            }
            MPI_Send(&fileData, sizeof(FileData), MPI_BYTE, status.MPI_SOURCE, TRACKER_FILE_SEGMENTS, MPI_COMM_WORLD);
        } else if (status.MPI_TAG == SEND_FILE_SWARM) {
            std::string requestedFile((char *) recvData);
            struct swarm_t swarm;
            swarm.numClients = fileSwarm[requestedFile].size();
            for (int i = 0; i < swarm.numClients; ++i) {
                swarm.clients[i] = fileSwarm[requestedFile][i].rank;
            }
            MPI_Send(&swarm, sizeof(swarm_t), MPI_BYTE, status.MPI_SOURCE, SEND_FILE_SWARM, MPI_COMM_WORLD);
        } else if (status.MPI_TAG == I_AM_PEER) {
            std::string requestedFile((char *) recvData);
            fileSwarm[requestedFile].push_back({status.MPI_SOURCE, PEER});
        } else if (status.MPI_TAG == I_AM_SEED) {
            std::string requestedFile((char *) recvData);
            for (auto &client : fileSwarm[requestedFile]) {
                if (client.rank == status.MPI_SOURCE) {
                    client.type = SEED;
                    break;
                }
            }
        } else if (status.MPI_TAG == DONE_MY_DOWNLOAD) {
            finishedClients++;
            if (finishedClients == numtasks - 1) {
                /* Send to all clients that they can stop upload thread. */
                for (int i = 1; i <= numtasks - 1; ++i) {
                    MPI_Send(NULL, 0, MPI_BYTE, i, REQUEST, MPI_COMM_WORLD);
                }
                free(recvData);
                break;
            }
        }
    }
}

void peer(int numtasks, int rank) {
    pthread_t download_thread;
    pthread_t upload_thread;
    void *status;
    int r;

    /* Initial step before uploading/downloading. */
    struct sendAllFiles ownedFiles;

    /* Open the coresponding file. */
    std::ifstream fin("in" + std::to_string(rank) + ".txt");
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
            myFiles[ownedFiles.files[f].fileName].push_back(newSegment);
        }
    }

    int requestedFilesNumber;
    fin >> requestedFilesNumber;
    for (int i = 0; i < requestedFilesNumber; ++i) {
        std::string requestedFile;
        fin >> requestedFile;
        requestedFiles.push_back(requestedFile);
    }

    /* Close the file. */
    fin.close();
    
    /* Send the owned files to the tracker. */
    MPI_Send(&ownedFiles, sizeof(sendAllFiles), MPI_BYTE, TRACKER_RANK, TRACKER_BARRIER, MPI_COMM_WORLD);

    /* Wait until we can request files from tracker. */
    MPI_Status statusMPI;
    struct packet recv;
    do {
      MPI_Recv(&recv, sizeof(packet), MPI_BYTE, TRACKER_RANK, MPI_ANY_TAG, MPI_COMM_WORLD, &statusMPI);
    } while (recv.type != ACK);

    /* Start download thread. */
    r = pthread_create(&download_thread, NULL, download_thread_func, (void *) &rank);
    if (r) {
        printf("Eroare la crearea thread-ului de download\n");
        exit(-1);
    }

    /* Start upload thread. */
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
