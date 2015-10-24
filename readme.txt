# ComputerNetworks

Contributor:
'' Adam
'' Ke Wu

### Peer
Add network socket and stdin to select(), and put select() in a infinite loop. If a new I/O is from stdin, then it means user input a new GET command. Parse the command, read `get_chunk_file`, generate WHOHAS packet, flood the network. If a new I/O is from network socket, then it means the program has received a UDP packet from other peers. The program first demultiplex the packet to get the packet type. Then invoke different process functions to process the packet.

### Process WHOHAS packet
First parse packet, get an array of chunks (hash value) that asked for. Lookup the chunk one by one to check if the program has the chunk. Then generate IHAVE packet, send it back.

### Process IHAVE packet
First parse packet, then store it (which peer owns which chunks). 

## Packet helpers
Currently there are two main types of packet helpers: packet generator and packet parser.

### Packet generator
It takes in an array of chunk hashes, and generates either an array of packets or a single packet. When it generates an array of packets, it's because the total size of chunk hashes exceed the maximum number of chunk hashes allowed in a packet.

### Packet parser
It takes in a single packet and parses out an array of chunk hashes in it.