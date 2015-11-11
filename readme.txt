# ComputerNetworks

Contributor:
'' Junqiang Li
'' Ke Wu

Main
====

### Peer
Add network socket and stdin to select(), and put select() in a infinite loop. 
    1. If a new I/O is from stdin, then it means user input a new GET command. Parse the command, read `get_chunk_file`, generate WHOHAS packet, flood the network. 
    2. If a new I/O is from network socket, then it means the program has received a UDP packet from other peers. The program first demultiplex the packet to get the packet type. Then invoke different process functions to process the packet.
    3. If a new I/O is from a timer_fd, then it means one type (GET, DATA or ACK) is lost. It will start processing timeout. For DATA & ACK, use Congestion Control mechanism to handle.

### Packet helpers
Currently there are two main types of packet helpers: packet generator and packet parser.

### Packet generator
It takes in an array of chunk hashes, and generates either an array of packets or a single packet. When it generates an array of packets, it's because the total size of chunk hashes exceed the maximum number of chunk hashes allowed in a packet.

### Packet parser
It takes in a single packet and parses out an array of chunk hashes in it.


Reliability & correctness
==================================

### Lifecycle of a peer as receiver

#### WHOHAS & IHAVE
1. It starts with the peer sending out WHOHAS packet
2. It will receive its peers' IHAVE packet
3. It will set ready flag (is_check) for sending GET packet

#### WHOHAS timeouts
Will wait 5 seconds before timeouts, then flood peers again if no response.


#### GET
1. It will check whether max download is reached, if yes, abort
2. If not, it tries to find a peer who has the packet it wants. It will 1st look in non-crashed peers. 
   If cannot find 1 non-crashed peer with the packet, it goes to find among crashed peers.
3. Send the GET packet to the peer.
4. Increase the # of current downloading counter.
5. Go back to step 1.

#### ACK
1. For each data packet it receives, it will send an ACK.

#### GET timeouts
1. If sussessive failures are larger than the upper limit, set the peer as crashed.
2. Otherwise, send GET to the peer again and increase the successive failure counter.


### Lifecycle of a peer as sender

#### WHOHAS & IHAVE
1. When it receives WHOHAS and finds out it has one / more packets the other peer wants, send IHAVE

#### GET & DATA
1. When it receives GET packet, build the coresponding data packet by reading chunk data from master
   data file.
2. Send the data packets to receiver.

#### DATA packet loss
See congestion control section


### Timeout implementation
We used timer_fd for each connection between sender and receiver for timeout. This file descriptor
will be stored into select's read set. Every time it pops out, we will know which specific connection
and also which type of packet (i.e. GET, DATA or ACK) is timeout.


## Congestion control
=======================
We built congestion control on the top of the reliable connection we made before. As suggested in handout, there are two pheses in congestion control. In slow start, each ack make cwnd+= 1, in congestion avoidance, each ack make cwnd += 1/cwnd. Sender will send the next data packet if the window permit. When sender receive three duplicate ack or timer timeout, it means a packet loss. In this case, sender will ignore all timer of the following unack packets. Sender will resend the last unack data packet. Then ssthresh = cwnd/2, cwns = 1, go back to slow start phase again. In this case, it is very likely that sender will receive a cumulative ack. Then sender will just shift the window based on the cumulative ack.
