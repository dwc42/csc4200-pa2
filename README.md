# Programming Assignment TCP over UDP

### run instructions
1. run 
```bash
cd ../sprint#
```
# is 1, 2, or 3.

2. run 
```bash
make
``` 
to run the client
```bash
./client -s <SERVER_IP> -p <SERVER_PORT> -l <CLIENT_LOG_FILE_LOCATION> -f <FILE_TO_TRANSMIT_LOCATION>
```
\*: required
\*?: only sprint 2 onwards
SERVER_IP\*: The IP of the the server you would like to transmit to.
SERVER_PORT\*: The Port of the the server you would like to transmit to.
CLIENT_LOG_FILE_LOCATION\*: The location of the log file for the client
FILE_TO_TRANSMIT_LOCATION\*?:

to run the server
```bash
./server -p <SERVER_PORT> -l <SERVER_LOG_FILE_LOCATION> -d <SHOULD_DROP>
```
SERVER_PORT\*: The Port of the the server you would like to transmit to.
CLIENT_LOG_FILE_LOCATION\*: The location of the log file for the client
SHOULD_DROP: if 1, runs a drop rule command before the file is transmitted to test retransmission else 0 or default does nothing
### Explanations
1. A description of your three-way handshake implementation.
	Client sets up a connection with the Server then sends a SYN, synchronization packet with the client's ISN and the SYN flag set. Then, the client waits until the server sends back a SYN with the server's ISN, ACK number being the client's ISN+1, and the SYN and ACK flags set. if the client getting the SYN from the server times out then the client retransmits up to max reties. Then the Client sends a ACK with the server's ISN +1 and ACK flag set and the Server waits till it gets the client's ACK packet. If the clients ACK times out the server will also retransmit th ACK of the clients SYN. Then the 3 way handshake is complete.
2. A description of your retransmission logic.'
   If part if the file needs to be retransmitted, ie SYN was not the expected one, the server sends back a duplicate ACK packet where ACK = expected SYN and the client will retransmit. Also if the server does not send a ACK back in time it will also retransmit.
3. A description of your teardown logic.
   When the client is done sending the file it sends a FIN packet, SEQ = final sequence. The server waits for a packet and first check if its a FIN packet, if so sends a FIN ACK packet which is ACK = final sequence +1 and the FIN and ACK flags are set. Else, does the regular file packet process. If the client waiting for that FIN ACK times out it retransmits to max retires.
4. Any known limitations or bugs.
	None known at this time.
### Sprint 3 example output:
Client:
```bash
user@client-instance-id:~/sprint3$ ./client -s 10.128.0.3 -p 5000 -l client.log -f biggest.jpg
Success: Connected to server at 10.128.0.3:5000
ISN: 1972582143
sent cient SYN
recv Server SYN
Handshake complete.
file found at: biggest.jpg
32569ab9df5a392b47fe8ac9812a40429133b830664f467156da5a5a399d8270
File Sent
Connection closed cleanly.
```
Server:
```bash
user@server-instance-id:~/csc4200-pa2/sprint3$ ./server -p 5000 -l server.log 
Server listening on port 5000...
Client ISN: 1972582143, Server ISN: 44161332
recv Client ACK
Handshake complete.
FILENAME: biggest.jpg
Interaction with 10.128.0.2 completed.
32569ab9df5a392b47fe8ac9812a40429133b830664f467156da5a5a399d8270
Waiting for next client...
```

<!-- 1. Your name and the assignment name.
1. How to compile (`make`) and run both programs.
2. A description of your three-way handshake implementation.
3. A description of your retransmission logic.
4. A description of your teardown logic.
5. Any known limitations or bugs.
6. Sample log output from a successful run. -->
