# ComputerNetworks

Contributor:
	Adam
	Ke Wu


## Packet helpers
Currently there are two main types of packet helpers: packet generator and packet parser.

### Packet generator
It takes in an array of chunk hashes, and generates either an array of packets or a single packet. When it generates an array of packets, it's because the total size of chunk hashes exceed the maximum number of chunk hashes allowed in a packet.

### Packet parser
It takes in a single packet and parses out an array of chunk hashes in it.

