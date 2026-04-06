#include "../include/protocol.h"

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