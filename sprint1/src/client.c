#include "../include/protocol.h"

/**
 * 1. Create a UDP socket with `socket(AF_INET, SOCK_DGRAM, 0)`.
2. Set `SO_RCVTIMEO` to `{TIMEOUT_SEC, TIMEOUT_USEC}`.
3. Generate a random ISN: seed with `srand((unsigned)time(NULL) ^ getpid())`, then call `rand()`.
4. Send a SYN packet (flags = `FLAG_SYN`, seq = ISN, ack = 0).
5. Wait for SYN|ACK. Validate: `FLAG_SYN | FLAG_ACK` both set, `ack_num == client_isn + 1`.
6. If timeout or invalid, retransmit SYN (up to `MAX_RETRIES`).
7. Send ACK (flags = `FLAG_ACK`, seq = client_isn + 1, ack = server_isn + 1).
8. Print "Handshake complete."
 */
int main()
{
	struct sockaddr_in server_addr;
	struct sockaddr_in local_addr;
	socklen_t server_addr_len = sizeof(server_addr);

	SOCKET socket_client = socket(AF_INET, SOCK_DGRAM, 0);
	if (socket_client < 0)
	{
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(REMOTE_SERVER_PORT);
	if (inet_pton(AF_INET, REMOTE_SERVER_IP, &server_addr.sin_addr) < 0)
	{
		perror("server dest ip set failed");
		exit(EXIT_FAILURE);
	}
	struct timeval timeout = {TIMEOUT_SEC, TIMEOUT_USEC};
	if (setsockopt(socket_client, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
	{
		perror("setsockopt failed");
		exit(EXIT_FAILURE);
	}

	if (connect(socket_client, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("connection failed");
		exit(EXIT_FAILURE);
	}
	printf("Success: Connected to server at %s:%d\n", REMOTE_SERVER_IP, REMOTE_SERVER_PORT);

	Packet packetSYN = make_packet();
	// ISN
	srand((unsigned)time(NULL) ^ getpid());
	uint32_t initialSequenceNumber = rand();
	printf("ISN: %d", initialSequenceNumber);
	packetSYN.header.sequenceNumber = initialSequenceNumber;
	packetSYN.header.acknowledgmentNumber = 0;
	packetSYN.header.synchronizeSequence = 1;
	packetSYN.header.payloadLength = 0;
	char *serializedPacketSYN = packet_serialize(packetSYN);

	char *bufferRawServerPacketSYN;
	bufferRawServerPacketSYN = malloc(HEADER_SIZE);
	int result;
	uint32_t retries = 0;
	Packet serverPacketSYN;
	do
	{
		if (sendto(socket_client, serializedPacketSYN, strlen(serializedPacketSYN), 0, &server_addr, &server_addr_len) < 0)
		{
			close(socket_client);
			perror("SYN failed");
			exit(EXIT_FAILURE);
		}
		printf("sent cient SYN");
		if (recvfrom(socket_client, bufferRawServerPacketSYN, HEADER_SIZE, 0, &server_addr, &server_addr_len) < 0)
		{
			perror("timeout or recv failed, retransmit?");
			continue;
		}
		serverPacketSYN = packet_deserialize(bufferRawServerPacketSYN);
		if (!serverPacketSYN.header.synchronizeSequence || !serverPacketSYN.header.acknowledgmentValid)
		{
			perror("sequenceNumber and acknowledgmentValid flags both not 1, retransmit?");
			continue;
		}
		if (serverPacketSYN.header.acknowledgmentNumber != (initialSequenceNumber + 1))
		{
			perror("sequenceNumber and acknowledgmentValid flags both not 1, retransmit?");
			continue;
		}
		printf("recv Server SYN");
		break;
	} while (++retries <= MAX_RETRIES);
	if (retries > MAX_RETRIES)
	{
		perror("MAX_RETRIES, closed connection");
		close(socket_client);
		exit(EXIT_FAILURE);
	}
	Packet packetACK = make_packet();
	packetSYN.header.sequenceNumber = initialSequenceNumber + 1;
	packetSYN.header.acknowledgmentNumber = serverPacketSYN.header.synchronizeSequence + 1;
	packetSYN.header.acknowledgmentValid = 1;
	packetSYN.header.payloadLength = 0;
	char *serializedPacketACK = packet_serialize(packetACK);
	if (sendto(socket_client, serializedPacketACK, strlen(serializedPacketACK), 0, &server_addr, &server_addr_len) < 0)
	{
		close(socket_client);
		perror("ACK failed");
		exit(EXIT_FAILURE);
	}
	printf("Handshake complete.");
	close(socket_client);
}