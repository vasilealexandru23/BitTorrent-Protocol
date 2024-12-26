#ifndef __UTILIS_H__
#define __UTILIS_H__

#include <bits/stdc++.h>

struct Segment {
    std::string hash;
    int filePos;
};

struct sendFileData {
    std::string fileName;
    std::vector<struct Segment> Segments;
};

enum packetType {
    REQUEST_FILE,
    RESPONSE,
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
