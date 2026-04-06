#include "../include/protocol.h"
typedef struct ClientConfig
{
	char *serverIp;
	uint16_t port;
	char *logfilePath;
	char *filePath;
} ClientConfig;
ClientConfig parseClientArgs(int argc, char *argv[])
{
	ClientConfig clientConfig;
	clientConfig.logfilePath = NULL;
	clientConfig.filePath = NULL;
	clientConfig.serverIp = NULL;
	clientConfig.port = 0;
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-p") == 0 && i + 1 < argc)
		{
			clientConfig.port = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-l") == 0 && i + 1 < argc)
		{
			clientConfig.logfilePath = argv[++i];
		}
		else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc)
		{
			clientConfig.serverIp = argv[++i];
		}
		else if (strcmp(argv[i], "-f") == 0 && i + 1 < argc)
		{
			clientConfig.filePath = argv[++i];
		}
	}
	return clientConfig;
}
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
int main(int argc, char *argv[])
{
	ClientConfig clientConfig = parseClientArgs(argc, argv);
	if (clientConfig.logfilePath == NULL)
	{
		printf("clientConfig.logfilePath == NULL");
		exit(EXIT_FAILURE);
	}
	else if (clientConfig.port < 1024)
	{
		printf("clientConfig.port < 1024");
		exit(EXIT_FAILURE);
	}
	else if (clientConfig.serverIp == NULL)
	{
		printf("clientConfig.serverIp == NULL");
		exit(EXIT_FAILURE);
	}
	else if (clientConfig.filePath == NULL)
	{
		printf("clientConfig.filePath == NULL");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in server_addr;
	// struct sockaddr_in local_addr;
	socklen_t server_addr_len = sizeof(server_addr);
	int socket_client = socket(AF_INET, SOCK_DGRAM, 0);
	if (socket_client < 0)
	{
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(clientConfig.port);
	if (inet_pton(AF_INET, clientConfig.serverIp, &server_addr.sin_addr) <= 0)
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
	printf("Success: Connected to server at %s:%d\n", clientConfig.serverIp, clientConfig.port);

	Packet packetSYN = make_packet();
	// ISN
	srand((unsigned)time(NULL) ^ getpid());
	uint32_t initialSequenceNumber = rand();
	printf("ISN: %d\n", initialSequenceNumber);
	packetSYN.header.sequenceNumber = initialSequenceNumber;
	packetSYN.header.acknowledgmentNumber = 0;
	packetSYN.header.synchronizeSequence = 1;
	packetSYN.header.payloadLength = 0;
	char *serializedPacketSYN = packet_serialize(packetSYN);

	char bufferRawServerPacketSYN[HEADER_SIZE];

	uint32_t retries = 0;
	Packet serverPacketSYN;
	do
	{
		if (sendto(socket_client, serializedPacketSYN, HEADER_SIZE, 0, (struct sockaddr *)&server_addr, server_addr_len) < 0)
		{
			close(socket_client);
			perror("SYN failed");
			exit(EXIT_FAILURE);
		}
		log_packet(packetSYN, clientConfig.logfilePath, Send);
		printf("sent cient SYN\n");
		if (recvfrom(socket_client, bufferRawServerPacketSYN, HEADER_SIZE, 0, (struct sockaddr *)&server_addr, &server_addr_len) < 0)
		{
			printf("timeout or recv failed, retransmit?\n");
			continue;
		}
		serverPacketSYN = packet_deserialize(bufferRawServerPacketSYN);
		log_packet(serverPacketSYN, clientConfig.logfilePath, Receive);
		if (!serverPacketSYN.header.synchronizeSequence || !serverPacketSYN.header.acknowledgmentValid)
		{
			printf("synchronizeSequence and acknowledgmentValid flags both not 1, retransmit?\n");
			continue;
		}
		if (serverPacketSYN.header.acknowledgmentNumber != (initialSequenceNumber + 1))
		{
			printf("acknowledgmentNumber != initialSequenceNumber + 1, retransmit?\n");
			continue;
		}
		printf("recv Server SYN\n");
		break;
	} while (++retries < MAX_RETRIES);
	if (retries >= MAX_RETRIES)
	{
		perror("MAX_RETRIES, closed connection");
		close(socket_client);
		exit(EXIT_FAILURE);
	}
	Packet packetACK = make_packet();
	packetACK.header.sequenceNumber = initialSequenceNumber + 1;
	packetACK.header.acknowledgmentNumber = serverPacketSYN.header.sequenceNumber + 1;
	packetACK.header.acknowledgmentValid = 1;
	packetACK.header.payloadLength = 0;
	char *serializedPacketACK = packet_serialize(packetACK);
	if (sendto(socket_client, serializedPacketACK, HEADER_SIZE, 0, (struct sockaddr *)&server_addr, server_addr_len) < 0)
	{
		close(socket_client);
		perror("ACK failed");
		exit(EXIT_FAILURE);
	}
	log_packet(packetACK, clientConfig.logfilePath, Send);
	printf("Handshake complete.\n");

	close(socket_client);
}