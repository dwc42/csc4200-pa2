#include "../include/protocol.h"
/*
 * protocol.c
 * CSC4200 — Program 2: TCP-Like Reliable Protocol over UDP
 *
 * This header defines the packet structure and constants for the
 * custom reliability protocol you will implement.
 *
 * DO NOT change field names, sizes, or the HEADER_SIZE constant.
 * Your serialization and deserialization must match this layout exactly.
 *
 * Packet Wire Format (all fields big-endian / network byte order):
 *
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                     Sequence Number  (32 bits)                |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                  Acknowledgment Number (32 bits)              |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                   Not Used (29 bits)                    |A|S|F|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                    Payload Length (32 bits)                   |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                    Payload (variable)                         |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * Flag bits (low 3 bits of the flags field):
 *   Bit 0 (F) — FIN  : No more data from sender; initiate teardown
 *   Bit 1 (S) — SYN  : Synchronize sequence numbers (handshake)
 *   Bit 2 (A) — ACK  : Acknowledgment Number field is valid
 */
Packet make_packet()
{
	Packet packet;
	memset(&packet.header, 0, sizeof(Packet));
	return packet;
}
char *packet_serialize(Packet packet)
{
	char *serializedPacket;
	//+1 for null termination
	uint64_t serializedPacketSize = HEADER_SIZE + packet.header.payloadLength + 1;
	serializedPacket = malloc(serializedPacketSize);

	uint8_t i = 0;
	uint32_t bufferNetworkInt; // store int to htonl for the serializedPacket
	bufferNetworkInt = htonl(packet.header.sequenceNumber);
	// i++ to count at runtime;
	memcpy(serializedPacket + sizeof(uint32_t) * i++, &bufferNetworkInt, sizeof(uint32_t));
	bufferNetworkInt = htonl(packet.header.acknowledgmentNumber);
	memcpy(serializedPacket + sizeof(uint32_t) * i++, &bufferNetworkInt, sizeof(uint32_t));
	bufferNetworkInt = htonl((packet.header.unused << 3) | (packet.header.acknowledgmentValid << 2) | (packet.header.synchronizeSequence << 1) | packet.header.noMoreData);
	memcpy(serializedPacket + sizeof(uint32_t) * i++, &bufferNetworkInt, sizeof(uint32_t));
	bufferNetworkInt = htonl(packet.header.payloadLength);
	memcpy(serializedPacket + sizeof(uint32_t) * i++, &bufferNetworkInt, sizeof(uint32_t));
	printf("finished packet_serialize of header");
	memcpy(serializedPacket + HEADER_SIZE, packet.payload, packet.header.payloadLength);
	serializedPacket[HEADER_SIZE + packet.header.payloadLength] = '\0';
	printf("finished packet_serialize");
	return serializedPacket;
}
Packet packet_deserialize(char *serializedPacket)
{
	printf("run");
	Packet packet = make_packet();
	uint8_t i = 0;
	uint32_t bufferInt;
	memcpy(&bufferInt, serializedPacket + sizeof(uint32_t) * i++, sizeof(uint32_t));
	packet.header.sequenceNumber = ntohl(bufferInt);
	memcpy(&bufferInt, serializedPacket + sizeof(uint32_t) * i++, sizeof(uint32_t));
	packet.header.acknowledgmentNumber = ntohl(bufferInt);
	memcpy(&bufferInt, serializedPacket + sizeof(uint32_t) * i++, sizeof(uint32_t));
	bufferInt = ntohl(bufferInt);
	uint8_t offset = 3;
	packet.header.unused = bufferInt >> (--offset);
	packet.header.acknowledgmentValid = bufferInt >> (--offset) & 0x1u;
	packet.header.synchronizeSequence = bufferInt >> (--offset) & 0x1u;
	packet.header.noMoreData = bufferInt >> (--offset) & 0x1u;
	memcpy(&bufferInt, serializedPacket + sizeof(uint32_t) * i++, sizeof(uint32_t));
	packet.header.payloadLength = ntohl(bufferInt);

	printf("payloadLength: %d,\n", packet.header.payloadLength);
	packet.payload = malloc(packet.header.payloadLength + 1);
	memcpy(packet.payload, serializedPacket + HEADER_SIZE, packet.header.payloadLength);
	packet.payload[packet.header.payloadLength] = '\0';
	return packet;
}

void printPacket(Packet packet)
{
	printf("Packet: (\n");
	printf("  acknowledgmentNumber: %d,\n", packet.header.acknowledgmentNumber);
	printf("  synchronizeSequence: %d,\n", packet.header.sequenceNumber);
	printf("  unused: %d,\n", packet.header.unused);
	printf("  acknowledgmentValid: %d,\n", packet.header.acknowledgmentValid);
	printf("  synchronizeSequence: %d,\n", packet.header.synchronizeSequence);
	printf("  noMoreData: %d,\n", packet.header.noMoreData);
	printf("  payloadLength: %d,\n", packet.header.payloadLength);
	printf("  payload: %s,\n", packet.payload);
	printf(")\n");
}

// int main()
// {

// 	Packet packet = make_packet();
// 	packet.payload = "Test Message";
// 	packet.header.acknowledgmentNumber = 389983;
// 	packet.header.sequenceNumber = 389983;
// 	packet.header.acknowledgmentValid = 1;
// 	packet.header.synchronizeSequence = 1;
// 	packet.header.noMoreData = 1;
// 	packet.header.payloadLength = strlen(packet.payload);
// 	printf("\n\nPacket Start: \n");
// 	printPacket(packet);
// 	char *serializedPacket = packet_serialize(packet);
// 	printf("%s\n", serializedPacket);
// 	Packet packet2 = packet_deserialize(serializedPacket);
// 	printf("\n\nPacket Back: \n");
// 	printPacket(packet2);

// 	free(serializedPacket);
// 	free(packet.payload);
// 	free(packet2.payload);
// }
