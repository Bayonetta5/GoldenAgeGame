// GoldenAgeGameServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

//winsock server only! hooray!

void InitWinsock()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
}

int main()
{
	int iResult;
	u_long iMode = 1;
	SOCKET socketS;

	InitWinsock();
	struct sockaddr_in local;
	struct sockaddr_in from;
	int fromlen = sizeof(from);
	local.sin_family = AF_INET;
	local.sin_port = htons(1234);
	local.sin_addr.s_addr = INADDR_ANY;

	socketS = socket(AF_INET, SOCK_DGRAM, 0);
	iResult = ioctlsocket(socketS, FIONBIO, &iMode);
	if (iResult != NO_ERROR)
		printf("ioctlsocket failed with error: %ld\n", iResult);
	bind(socketS, (sockaddr*)&local, sizeof(local));
	while (1)
	{
		char buffer[1024];
		ZeroMemory(buffer, sizeof(buffer));
		if (recvfrom(socketS, buffer, sizeof(buffer), 0, (sockaddr*)&from, &fromlen) != SOCKET_ERROR)
		{
			printf("Received message from %s: %s\n", inet_ntoa(from.sin_addr), buffer);
			sendto(socketS, buffer, sizeof(buffer), 0, (sockaddr*)&from, fromlen);
		}
	}
	closesocket(socketS);

	return 0;
}