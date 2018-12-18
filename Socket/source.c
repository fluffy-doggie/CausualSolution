//
//#include <WinSock2.h>
//#include <winerror.h>
//
//#include <stdio.h>
//
//#pragma comment(lib, "ws2_32.lib")
//
//#define BUFFER_SZ (4096)
//
//int main(int argc, char *argv[])
//{
//	int errcode;
//	WSADATA wsaData;
//	if (errcode = WSAStartup(MAKEWORD(2, 2), &wsaData))
//	{
//		return errcode;
//	}
//
//	SOCKET sServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
//	if (INVALID_SOCKET == sServer)
//	{
//		WSACleanup();
//		return INVALID_SOCKET;
//	}
//
//	SOCKADDR_IN addrServer;
//	addrServer.sin_family = AF_INET;
//	addrServer.sin_port = htons(8000);
//	addrServer.sin_addr.s_addr = INADDR_ANY;
//
//	if (SOCKET_ERROR == bind(sServer, (SOCKADDR *)&addrServer, sizeof(addrServer)))
//	{
//		closesocket(sServer);
//		WSACleanup();
//		return SOCKET_ERROR;
//	}
//
//	if (SOCKET_ERROR == listen(sServer, 1))
//	{
//		closesocket(sServer);
//		WSACleanup();
//		return SOCKET_ERROR;
//	}
//	while (1)
//	{
//		SOCKADDR_IN addrClient;
//		int addrClientLength = sizeof(addrClient);
//		SOCKET sClient = accept(sServer, (SOCKADDR *)&addrClient, &addrClientLength);
//		if (INVALID_SOCKET == sClient)
//		{
//			closesocket(sServer);
//			WSACleanup();
//			return INVALID_SOCKET;
//		}
//
//		char buffer[BUFFER_SZ];
//		memset(buffer, 0, BUFFER_SZ);
//
//		if (SOCKET_ERROR == recv(sClient, buffer, 512, 0))
//		{
//			closesocket(sServer);
//			closesocket(sClient);
//			WSACleanup();
//			return SOCKET_ERROR;
//		}
//
//		printf("%s", buffer);
//
//		const char *msg = "{\"msg\":\"Hello World\"}";
//
//		memset(buffer, 0, BUFFER_SZ);
//		strcpy_s(buffer, BUFFER_SZ, msg);
//		if (SOCKET_ERROR == send(sClient, buffer, strlen(buffer), 0))
//		{
//			closesocket(sServer);
//			closesocket(sClient);
//			WSACleanup();
//			return SOCKET_ERROR;
//		}
//
//		closesocket(sClient);
//	}
//
//	closesocket(sServer);
//	WSACleanup();
//	return 0;
//}