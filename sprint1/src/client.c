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
	uint32_t client_ISN;
	int socket_client = socket(AF_INET, SOCK_DGRAM, 0);
	if (socket_client < 0)
	{
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}
	if (!createConnection(socket_client, clientConfig, &server_addr, &client_ISN))
	{
		printf("Handshake failed.\n");
		exit(EXIT_FAILURE);
	};
	printf("Handshake complete.\n");
	close(socket_client);
}