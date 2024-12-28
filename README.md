# BitTorrent Protocol: simulate the BitTorrent peer-to-peer file sharing protocol using MPI.

### This directory contains the...

## `Structure of the directory:`
  * `checker/` -> Directory containing the testing files to check correctness.
  * `src/` -> Directory with implementation of the model directly applied on the inverted index task.
  * `docker*` -> Files for testing the program with docker.
    
## Implementation:

The mechanism used for interogating a peer by another peer about the ownership   
of a segment (part of a file) is by sending to the proposed peer the segment data
{HASH, POS_IN_FILE}, with the special flag, to recognize the type of request. If
the computer has that segment it will respond with it's number of uploads done,
otherwise will respond with -1. The requester will go over all owners of the file
and will choose for transfer, the one with the lowest number of uploads done.

### Note: Read more about the protocol [here](https://en.wikipedia.org/wiki/BitTorrent).

### Copyright 2024 Vasile Alexandru-Gabriel (vasilealexandru37@gmail.com)