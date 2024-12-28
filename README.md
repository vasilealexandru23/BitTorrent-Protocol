# BitTorrent Protocol: simulate the BitTorrent peer-to-peer file sharing protocol using MPI.

### This directory contains the implementation and simulation for BitTorrent protocol.

## `Structure of the directory:`
  * `checker/` -> Directory containing the testing files to check correctness.
  * `src/` -> Directory with implementation of the protocol.
  * `docker*` -> Files for testing the program with docker.
    
## Implementation:

First, each BitTorrent client reads it's own file, to see which files have and which
files need to request. It send the owned file data (hash + position) to the tracker
and wait for the tracker to start the download of the files. After the tracker
send the signal to start, each client starts its own download and upload thread.

For the download thread, it starts by iterating over all files needed. For each file,
it first asks the tracker to get the segments and the swarm of the file. After each
10 segments received, it asks again to update the swarm. For each segment, it asks
every node in the swarm if it has that requiered segment, and first waits to see
the number of uploads for each client and chooses the one with the lowest number, for
efficiency. Then asks him for download and wait for the ACK. After the download of the
first segment, the client signal the tracker that now it can be PEER for it. And after
the all file is downloaded, the client signal the tracker that now is SEED for it, and
it moves forward to the next file. After all files are downloaded, the client signals
the tracker that it's downloads are complete and stops the DOWNLOAD thread.

For the upload thread, the client only receives REQUEST tagged packets from other users
and the tracker (the only message that gets from tracker is when to stop the work). From
clients it gets two types of requests, one when the request asks if the client has some
segment and the other one when it requests to download the segment.
The mechanism used for interogating a peer by another peer about the ownership   
of a segment (part of a file) is by sending to the proposed owner the segment data
{HASH, POS_IN_FILE}, with the special flag, to recognize the type of request. If
the computer has that segment it will respond with it's number of uploads done (ACK),
otherwise will respond with -1 (NACK). The requester will go over all owners of the file
and will choose for transfer, the one with the lowest number of uploads done. (The memory
of a client is implemented by a global variable, an unordered_map that keeps for all
filenames, the owned segments).

For the tracker, for the file data, I keep an unordered_map to associate for each
filename the coressponding segments and for the swarm of a file, another unordered_map
to associate for each file, which users have some segments of it. First, the tracker
waits from all clients the owned files, then respond to them to start the download.
The tracker than receives packets from clients that are different by the tag source
of the packet. It can either be a request for the filedata, for the swarm of a file,
to make a client peer or seed for a file, or a signal that a client finished to download
it's requested files. After all clients have finished, it sends a signal to them to stop
the UPLOAD thread and it stops.

### Note: Read more about the protocol [here](https://en.wikipedia.org/wiki/BitTorrent).

### Copyright 2024 Vasile Alexandru-Gabriel (vasilealexandru37@gmail.com)