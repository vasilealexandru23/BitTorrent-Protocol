#ifndef __UTILIS_H__
#define __UTILIS_H__

#include <bits/stdc++.h>

#define TRACKER_RANK 0
#define MAX_FILES 10
#define MAX_FILENAME 15
#define HASH_SIZE 32
#define MAX_CHUNKS 100

struct Segment {
    int filePos;
    char hash[HASH_SIZE + 1];             // NULL terminator

    bool operator<(const Segment &other) const {
        return strcmp(hash, other.hash) < 0;
    }
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
    REQUEST_FILE,
    SEND_OWNED_FILES,
    ACK
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
