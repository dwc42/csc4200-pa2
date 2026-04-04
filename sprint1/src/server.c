#include "../include/protocol.h"
/**
 * 1. Create a UDP socket, bind to port.
2. Wait for SYN (blocking `recvfrom` — no timeout yet).
3. Validate `FLAG_SYN` is set. Ignore other packets.
4. Generate your own random ISN.
5. Send SYN|ACK (seq = server_isn, ack = client_isn + 1).
6. Set `SO_RCVTIMEO`. Wait for ACK.
7. If timeout, retransmit SYN|ACK.
8. On valid ACK: print "Handshake complete", log the event.
 */
int main()
{
	int server_socket, client_socket;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);
	// 1. create local socket
	server_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (server_socket < 0)
	{
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}
	// configure server address
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(REMOTE_SERVER_PORT);
	if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in)) < 0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	};
	printf("Server listening on port %d...\n", REMOTE_SERVER_PORT);
	while (1)
	{
		client_socket = accept(server_socket, (struct sockaddr *)&client_addr,
							   &client_addr_len);
		if (client_socket < 0)
		{
			close(client_socket);
			perror("accept failed");
		}
		char *bufferClientRawPacketSYN;
		bufferClientRawPacketSYN = malloc(HEADER_SIZE);
		if (recvfrom(server_socket, bufferClientRawPacketSYN, HEADER_SIZE, 0, &client_addr, &client_addr_len) < 0)
		{
			close(client_socket);
			perror("receive syc failed");
		}
		Packet clientPacketSYN = packet_deserialize(bufferClientRawPacketSYN);
		srand((unsigned)time(NULL) ^ getpid());
		uint32_t initialSequenceNumber = rand();
		if (!clientPacketSYN.header.synchronizeSequence)
		{
			close(client_socket);
			perror("packet.header.synchronizeSequence not 1");
		}
		Packet packetSYN = make_packet();
		packetSYN.header.sequenceNumber = initialSequenceNumber;
		packetSYN.header.acknowledgmentNumber = clientPacketSYN.header.sequenceNumber + 1;
		packetSYN.header.synchronizeSequence = 1;
		packetSYN.header.acknowledgmentValid = 1;
		packetSYN.header.payloadLength = 0;
		char *serializedPacketSYN = packet_serialize(packetSYN);
		struct timeval timeout = {TIMEOUT_SEC, TIMEOUT_USEC};
		if (setsockopt(server_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
		{
			perror("setsockopt failed");
			close(client_socket);
		}
		uint32_t retries = 0;
		char *bufferClientRawPacketACK;
		bufferClientRawPacketACK = malloc(HEADER_SIZE);
		Packet clientPacketACK;
		do
		{
			if (sendto(client_socket, serializedPacketSYN, strlen(serializedPacketSYN), 0, &client_addr, &client_addr_len) < 0)
			{
				perror("send SYN failed");
				close(client_socket);
			};
			if (recvfrom(client_socket, bufferClientRawPacketACK, HEADER_SIZE, 0, &client_addr, &client_addr_len) < 0)
			{
				perror("timeout or recv failed, retransmit?");
				continue;
			}
			clientPacketACK = packet_deserialize(bufferClientRawPacketACK);

			if (!clientPacketACK.header.acknowledgmentValid)
			{
				perror("sequenceNumber and acknowledgmentValid flags both not 1, retransmit?");
				continue;
			}
			if (clientPacketACK.header.acknowledgmentNumber != (initialSequenceNumber + 1))
			{
				perror("sequenceNumber and acknowledgmentValid flags both not 1, retransmit?");
				continue;
			}
			printf("recv Client ACK");
			break;
		} while (++retries < MAX_RETRIES);
		if (retries > MAX_RETRIES)
		{
			perror("MAX_RETRIES, closed connection");
			close(client_socket);
			exit(EXIT_FAILURE);
		}
		printf("Handshake complete.");
	}
}