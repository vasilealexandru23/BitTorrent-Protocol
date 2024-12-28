#ifndef __UTILIS_H__
#define __UTILIS_H__

#include <bits/stdc++.h>

#define TRACKER_RANK 0
#define MAX_FILES 10
#define MAX_FILENAME 15
#define HASH_SIZE 32
#define MAX_CHUNKS 100

#define MAX_SEG_RUN 10                   // Maximum segments before updating swarm

/* MPI CUSTOM TAGS FOR COMMUNICATION. */
#define SEND_FILE_SWARM 10
#define TRACKER_FILE_SEGMENTS 11
#define CLIENT_ASK_SEGMENT 12
#define CLIENT_DOWNLOAD_SEGMENT 13
#define TRACKER_BARRIER 14
#define REQUEST 15
#define RESPONSE 16
#define I_AM_PEER 17
#define I_AM_SEED 18
#define DONE_MY_DOWNLOAD 19
#define STOP 20

#define MAX_PACKET_SIZE 1024
#define MAX_USERS 100

struct Segment {
    int filePos;
    char hash[HASH_SIZE + 1];             // NULL terminator

    bool operator<(const Segment &other) const {
        return strcmp(hash, other.hash) < 0;
    }

    bool operator==(const Segment &other) const {
        return strcmp(hash, other.hash) == 0 && filePos == other.filePos;
    }
};

enum operation {
    DOWNLOAD,
    ASK
};

struct askSegment {
    char fileName[MAX_FILENAME + 1];
    Segment segment;
    operation op;
};

struct FileData {
    int numSegments;
    char fileName[MAX_FILENAME + 1];      // NULL terminator
    struct Segment Segments[MAX_CHUNKS];
};

struct sendAllFiles {
    int numFiles;
    struct FileData files[MAX_FILES];
};

enum packetType {
    SEND_OWNED_FILES,
    REQUEST_FILE_DATA,
    ACK,
    NACK
};

enum clientType {
    SEED,
    PEER,
    LEECHER
};

struct client_t {
    int rank;
    clientType type;

    bool operator<(const client_t &other) const {
        return rank < other.rank;
    }
};

struct swarm_t {
    int numClients;
    int clients[MAX_USERS];
};

struct packet {
    packetType type;
    void *data;
};

/*
 * Macro de verificare a erorilor
 * Exemplu:
 * 		int fd = open (file_name , O_RDONLY);
 * 		DIE( fd == -1, "open failed");
 */

#define DIE(assertion, call_description)                                       \
  do {                                                                         \
    if (assertion) {                                                           \
      fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);                       \
      perror(call_description);                                                \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

#endif /* __UTILS_H__ */
