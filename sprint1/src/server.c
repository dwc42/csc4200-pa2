#include "../include/protocol.h"

typedef struct ServerConfig
{
	uint16_t port;
	char *logfilePath;
} ServerConfig;
ServerConfig parseServerArgs(int argc, char *argv[])
{
	ServerConfig serverConfig;
	serverConfig.logfilePath = NULL;
	serverConfig.port = 0;
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-p") == 0 && i + 1 < argc)
		{
			serverConfig.port = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-l") == 0 && i + 1 < argc)
		{
			serverConfig.logfilePath = argv[++i];
		}
	}
	return serverConfig;
}
typedef void OnConnectionCallback(int server_socket, ServerConfig serverConfig, struct sockaddr_in *server_addr, struct sockaddr_in *client_addr);
bool startListening(int server_socket, ServerConfig serverConfig, struct sockaddr_in *server_addr, OnConnectionCallback callback)
{
	if (server_socket < 0)
	{
		perror("socket creation failed");
		return false;
	}
	// configure server address
	memset(server_addr, 0, sizeof(struct sockaddr_in));
	server_addr->sin_family = AF_INET;
	server_addr->sin_addr.s_addr = INADDR_ANY;
	server_addr->sin_port = htons(REMOTE_SERVER_PORT);
	if (bind(server_socket, (struct sockaddr *)server_addr, sizeof(struct sockaddr_in)) < 0)
	{
		perror("bind failed");
		return false;
	};
	printf("Server listening on port %d...\n", serverConfig.port);
	while (1)
	{
		struct sockaddr_in *client_addr;
		// resets timeout to 0
		struct timeval blocking_timeout = {0, 0};
		if (setsockopt(server_socket, SOL_SOCKET, SO_RCVTIMEO, &blocking_timeout, sizeof(blocking_timeout)) < 0)
		{
			perror("setsockopt failed");
			continue;
		}
		socklen_t client_addr_len = sizeof(struct sockaddr_in);
		char bufferClientRawPacketSYN[HEADER_SIZE];
		if (recvfrom(server_socket, bufferClientRawPacketSYN, HEADER_SIZE, 0, (struct sockaddr *)client_addr, &client_addr_len) < 0)
		{
			perror("receive syc failed");
			continue;
		}

		Packet clientPacketSYN = packet_deserialize(bufferClientRawPacketSYN);
		log_packet(clientPacketSYN, serverConfig.logfilePath, Receive);

		srand((unsigned)time(NULL) ^ getpid());
		uint32_t initialSequenceNumber = rand();
		printf("Client ISN: %u, Server ISN: %u\n", clientPacketSYN.header.sequenceNumber, initialSequenceNumber);
		if (!clientPacketSYN.header.synchronizeSequence)
		{

			perror("packet.header.synchronizeSequence not 1");
		}
		Packet packetSYN = make_packet();
		packetSYN.header.sequenceNumber = initialSequenceNumber;
		packetSYN.header.acknowledgmentNumber = clientPacketSYN.header.sequenceNumber + 1;
		packetSYN.header.synchronizeSequence = 1;
		packetSYN.header.acknowledgmentValid = 1;
		packetSYN.header.payloadLength = 0;
		char *serializedPacketSYN = packet_serialize(packetSYN);

		uint32_t retries = 0;
		Packet clientPacketACK;
		char bufferClientRawPacketACK[HEADER_SIZE];
		do
		{
			if (sendto(server_socket, serializedPacketSYN, HEADER_SIZE, 0, (struct sockaddr *)client_addr, client_addr_len) < 0)
			{
				perror("send SYN failed");
				continue;
			};
			log_packet(packetSYN, serverConfig.logfilePath, Send);
			struct timeval timeout = {TIMEOUT_SEC, TIMEOUT_USEC};
			if (setsockopt(server_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
			{
				perror("setsockopt failed");
				continue;
			}
			if (recvfrom(server_socket, bufferClientRawPacketACK, HEADER_SIZE, 0, (struct sockaddr *)client_addr, &client_addr_len) < 0)
			{
				perror("timeout or recv failed, retransmit?");
				continue;
			}
			clientPacketACK = packet_deserialize(bufferClientRawPacketACK);
			log_packet(clientPacketACK, serverConfig.logfilePath, Receive);
			if (!clientPacketACK.header.acknowledgmentValid || clientPacketACK.header.synchronizeSequence)
			{
				printf("synchronizeSequence not 1 or acknowledgmentValid not 0 flags, retransmit?\n");
				continue;
			}
			if (clientPacketACK.header.acknowledgmentNumber != (initialSequenceNumber + 1))
			{
				printf("acknowledgmentNumber != initialSequenceNumber+1, retransmit?\n");
				continue;
			}
			printf("recv Client ACK\n");
			break;
		} while (++retries < MAX_RETRIES);
		if (retries >= MAX_RETRIES)
		{
			perror("MAX_RETRIES, closed connection");
			// exit(EXIT_FAILURE);
			continue;
		}
		callback(server_socket, serverConfig, server_addr, client_addr);
	}
	return true; // should never happen
}
void onConnectionCallback(int server_socket, ServerConfig serverConfig, struct sockaddr_in *server_addr, struct sockaddr_in *client_addr)
{
	(void)server_socket;
	(void)serverConfig;
	(void)server_addr;
	(void)client_addr;
	printf("Handshake complete.\n");
}
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
int main(int argc, char *argv[])
{

	ServerConfig serverConfig = parseServerArgs(argc, argv);
	if (serverConfig.logfilePath == NULL)
	{
		printf("serverConfig.logfilePath == NULL");
		exit(EXIT_FAILURE);
	}
	else if (serverConfig.port < 1024)
	{
		printf("serverConfig.port < 1024");
		exit(EXIT_FAILURE);
	}

	int server_socket;
	struct sockaddr_in server_addr;
	server_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (!startListening(server_socket, serverConfig, &server_addr, onConnectionCallback))
	{
		printf("failed to startListening\n");
		exit(EXIT_FAILURE);
	};
	// 1. create local socket
}