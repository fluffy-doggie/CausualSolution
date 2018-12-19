
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define xmalloc(s)	HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(s))
#define xfree(p)	HeapFree(GetProcessHeap(),0,(p))

#include <WinSock2.h>
#include <MSWSock.h>
#include <WS2tcpip.h>
#include <strsafe.h>

#include "socket.h"

char *g_Port = DEFAULT_PORT;	// 端口
BOOL g_bEndServer = FALSE;
BOOL g_bRestart = TRUE;
BOOL g_bVerbose = FALSE;
HANDLE g_hIOCP = INVALID_HANDLE_VALUE;
SOCKET g_sdListen = INVALID_SOCKET;			// 监听socket
HANDLE g_ThreadHandles[MAX_WORKER_THREAD];	// 线程
WSAEVENT g_hCleanupEvent[1];
PPER_SOCKET_CONTEXT g_pCtxtListenSocket = NULL;
PPER_SOCKET_CONTEXT g_pCtxtList = NULL;

CRITICAL_SECTION g_CriticalSection;	// 临界区

int myprintf(const char *lpFormat, ...)
{
	// 该函数专用缓存区 - 1k / 2k
	static TCHAR s_buffer[1024];

	DWORD dwLen = 0;
	DWORD dwRet = 0;
	va_list arglist;
	HANDLE hOut = NULL;
	HRESULT hRet;

	ZeroMemory(s_buffer, sizeof(s_buffer));

	va_start(arglist, lpFormat);
	
	dwLen = lstrlen(lpFormat);
	hRet = StringCchVPrintf(s_buffer, sizeof(s_buffer), lpFormat, arglist);
	
	if (dwRet >= dwLen || GetLastError() == 0)
	{	// equals: dwLen == 0 or no recently error

		hOut = GetStdHandle(STD_OUTPUT_HANDLE);
		if (hOut != INVALID_HANDLE_VALUE)
		{
			// console handle | buffer | size to write | size be written | is reversed
			WriteConsole(hOut, s_buffer, lstrlen(s_buffer), (LPDWORD)&dwLen, NULL);
		}
	}

	return dwLen;
}

void __cdecl main(int argc, char *argv[])
{
	//int ret = myprintf(TEXT("Hello World"));
	SYSTEM_INFO systemInfo;
	WSADATA wsaData;
	DWORD dwThreadCount = 0;
	int nRet = 0;

	g_ThreadHandles[0] = (HANDLE)WSA_INVALID_EVENT;

	for (int i = 0; i < MAX_WORKER_THREAD; ++i)
	{
		g_ThreadHandles[i] = INVALID_HANDLE_VALUE;
	}

	if (!ValidOptions(argc, argv))
		return;

	if (!SetConsoleCtrlHandler(CtrlHandler, TRUE))
	{
		myprintf("SetConsoleCtrlHandler() failed to install console handler: %d\n",
			GetLastError());
		return;
	}
	
	GetSystemInfo(&systemInfo);
	dwThreadCount = systemInfo.dwNumberOfProcessors * 2;

	if (WSA_INVALID_EVENT == (g_hCleanupEvent[0] = WSACreateEvent()))
	{
		myprintf("WSACreateEvent() failed: %d\n", WSAGetLastError());
		return;
	}

	if ((nRet = WSAStartup(0x202, &wsaData)) != 0) {
		myprintf("WSAStartup() failed: %d\n", nRet);
		SetConsoleCtrlHandler(CtrlHandler, FALSE);
		if (g_hCleanupEvent[0] != WSA_INVALID_EVENT)
		{
			WSACloseEvent(g_hCleanupEvent[0]);
			g_hCleanupEvent[0] = WSA_INVALID_EVENT;
		}
		return;
	}

	__try // SEH异常模型处理
	{
		// 初始化临界区
		InitializeCriticalSection(&g_CriticalSection);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		myprintf("InitializeCriticalSection raised an exception.\n");
		SetConsoleCtrlHandler(CtrlHandler, FALSE);
		if (g_hCleanupEvent[0] != WSA_INVALID_EVENT)
		{
			WSACloseEvent(g_hCleanupEvent[0]);
			g_hCleanupEvent[0] = WSA_INVALID_EVENT;
		}
		return;
	}

	while (g_bRestart)
	{
		g_bRestart = FALSE;
		g_bEndServer = FALSE;
		WSAResetEvent(g_hCleanupEvent[0]);

		__try
		{
			g_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
			if (g_hIOCP == NULL)
			{
				myprintf("CreateIoCompletionPort() failed to create I/O completion port: %d\n",
					GetLastError());
				__leave;
			}

			for (DWORD dwCPU = 0; dwCPU < dwThreadCount; ++dwCPU)
			{
				HANDLE hThread;
				DWORD dwThreadId;

				hThread = CreateThread(NULL, 0, WorkerThread, g_hIOCP, 0, &dwThreadId);
				if (hThread == NULL)
				{
					myprintf("CreateThread() failed to create worker thread: %d\n",
						GetLastError());
					__leave;
				}
				g_ThreadHandles[dwCPU] = hThread;
				hThread = INVALID_HANDLE_VALUE;
			}

			if (!CreateListenSocket())
				__leave;

			if (!CreateAcceptSocket(TRUE))
				__leave;

			WSAWaitForMultipleEvents(1, g_hCleanupEvent, TRUE, WSA_INFINITE, FALSE);
		}

		__finally
		{
			g_bEndServer = TRUE;

			// cause worker threads to exit
			if (g_hIOCP)
			{
				for (DWORD i = 0; i < dwThreadCount; ++i)
				{
					PostQueuedCompletionStatus(g_hIOCP, 0, 0, NULL);
				}
			}

			// make sure worker threads exits
			if (WAIT_OBJECT_0 != WaitForMultipleObjects(dwThreadCount, g_ThreadHandles, TRUE, 1000))
				myprintf("WaitForMultipleObjects() failed: %d\n", GetLastError());
			else 
				for (DWORD i = 0; i < dwThreadCount; ++i)
				{
					if (g_ThreadHandles[i] != INVALID_HANDLE_VALUE)
						CloseHandle(g_ThreadHandles[i]);
					g_ThreadHandles[i] = INVALID_HANDLE_VALUE;
				}

			if (g_sdListen != INVALID_SOCKET)
			{
				closesocket(g_sdListen);
				g_sdListen = INVALID_SOCKET;
			}

			if (g_pCtxtListenSocket)
			{
				while (!HasOverlappedIoCompleted((LPOVERLAPPED)&g_pCtxtListenSocket->pIOContext->Overlapped))
					Sleep(0);

				if (g_pCtxtListenSocket->pIOContext->SocketAccept != INVALID_SOCKET)
					closesocket(g_pCtxtListenSocket->pIOContext->SocketAccept);
				g_pCtxtListenSocket->pIOContext->SocketAccept = INVALID_SOCKET;

				if (g_pCtxtListenSocket->pIOContext)
					xfree(g_pCtxtListenSocket->pIOContext);

				if (g_pCtxtListenSocket)
					xfree(g_pCtxtListenSocket);
				g_pCtxtListenSocket = NULL;
			}

			CtxtListFree();

			if (g_hIOCP)
			{
				CloseHandle(g_hIOCP);
				g_hIOCP = NULL;
			}
		}

		if (g_bRestart)
		{
			myprintf("\niocpserverex is restarting...\n");
		}
		else
		{
			myprintf("\niocpserverex is exiting...\n");
		}
	}

	DeleteCriticalSection(&g_CriticalSection);
	if (g_hCleanupEvent[0] != WSA_INVALID_EVENT)
	{
		WSACloseEvent(g_hCleanupEvent[0]);
		g_hCleanupEvent[0] = WSA_INVALID_EVENT;
	}

	WSACleanup();
	SetConsoleCtrlHandler(CtrlHandler, FALSE);
}

// 验证启动选项
BOOL ValidOptions(int argc, char *argv[])
{
	BOOL bRet = TRUE;

	for (int i = 1; i < argc; ++i)
	{
		if ((argv[i][0] == '-') || (argv[i][0] == '/'))
		{
			switch (tolower(argv[i][1]))
			{
			case 'e':
				if (strlen(argv[i]) > 3)
					g_Port = &argv[i][3];
				break;

			case 'v':
				g_bVerbose = TRUE;
				break;

			case '?':
				myprintf("Usage:\n  iocpserver [-p:port] [-v] [-?]\n");
				myprintf("  -e:port\tSpecify echoing port number\n");
				myprintf("  -v\t\tVerbose\n");
				myprintf("  -?\t\tDisplay this help\n");
				bRet = FALSE;
				break;

			default:
				myprintf("Unknown options flag %s\n", argv[i]);
				bRet = FALSE;
				break;
			}
		}
	}

	return (bRet);
}

// 控制台事件响应 ctrl+c | ctrl+break ...
BOOL WINAPI CtrlHandler(DWORD dwEvent)
{
	switch (dwEvent)
	{
	case CTRL_BREAK_EVENT:
		g_bRestart = TRUE;
	case CTRL_C_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
	case CTRL_CLOSE_EVENT:
		if (g_bVerbose)
			myprintf("CtrlHandler: closing listening socket\n");

		g_bEndServer = TRUE;

		WSASetEvent(g_hCleanupEvent[0]);
		break;

	default:

		return (FALSE);
	}

	return (TRUE);
}

SOCKET CreateSocket(void)
{
	int nRet = 0;
	int nZero = 0;
	SOCKET sdSocket = INVALID_SOCKET;

	sdSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (sdSocket == INVALID_SOCKET)
	{
		myprintf("WSASocket(sdSocket) failed: %d\n", WSAGetLastError());
		return (sdSocket);
	}

	// 禁用发送区缓存，SO_SNDBUF: 0
	// winsock直接将数据从此程序的缓冲区发出，省下一次内存拷贝
	// 将会每次直接发送一个TCP/IP包，尽管它没有被填满
	// 禁用发送区缓存比禁用接收区缓存后果要小得多

	nZero = 0;
	nRet = setsockopt(sdSocket, SOL_SOCKET, SO_SNDBUF, (char *)&nZero, sizeof(nZero));
	if (nRet == SOCKET_ERROR)
	{
		myprintf("setsockopt(SNDBUF) failed: %d\n", WSAGetLastError());
		return (sdSocket);
	}

	LINGER lingerStruct;

	//
	// linger value （不要设置）
	// 设置过短的linger可能会导致一些数据丢失
	// 如果需要防止一些恶意连接，只连接不发送数据
	// 服务器可以设置一个timer，等到一定时间之后再设置linger value为舍弃
	// 
/*
	lingerStruct.l_onoff = 1;
	lingerStruct.l_linger = 0;
	nRet = setsockopt(sdSocket, SOL_SOCKET, SO_LINGER,
		(char *)&lingerStruct, sizeof(lingerStruct));
	if (nRet == SOCKET_ERROR)
	{
		myprintf("setsockopt(SO_LINGER) failed: %d\n", WSAGetLastError());
		return (sdSocket);
	}
*/
	return (sdSocket);
}

BOOL CreateListenSocket(void)
{
	int nRet = 0;
	LINGER lingerStruct;
	struct addrinfo hints;
	struct addrinfo *addrlocal = NULL;
	ZeroMemory(&hints, sizeof(hints));

	lingerStruct.l_onoff = 1;	// 0: 立即关闭，非0: 等待一段时间
	lingerStruct.l_linger = 0;	// 等待时间，单位：s

	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_IP;

	if (getaddrinfo(NULL, g_Port, &hints, &addrlocal) != 0)
	{
		myprintf("getaddrinfo() failed with error %d\n", WSAGetLastError());
		return (FALSE);
	}

	if (addrlocal == NULL)
	{
		myprintf("getaddrinfo() failed to resolve/convert the interface\n");
		return (FALSE);
	}

	g_sdListen = CreateSocket();
	if (g_sdListen == INVALID_SOCKET)
	{
		freeaddrinfo(addrlocal);
		return (FALSE);
	}

	nRet = bind(g_sdListen, addrlocal->ai_addr, (int)addrlocal->ai_addrlen);
	if (nRet == SOCKET_ERROR)
	{
		myprintf("bind() failed: %d\n", WSAGetLastError());
		freeaddrinfo(addrlocal);
		return (FALSE);
	}

	nRet = listen(g_sdListen, 5);
	if (nRet == SOCKET_ERROR)
	{
		myprintf("listen() failed: %d\n", WSAGetLastError());
		freeaddrinfo(addrlocal);
		return (FALSE);
	}
	
	freeaddrinfo(addrlocal);

	return (TRUE);
}

BOOL CreateAcceptSocket(BOOL fUpdateIOCP)
{
	int nRet = 0;
	DWORD dwRecvNumBytes = 0;
	DWORD bytes = 0;

	GUID acceptex_guid = WSAID_ACCEPTEX;

	if (fUpdateIOCP)
	{
		g_pCtxtListenSocket = UpdateCompletionPort(g_sdListen, ClientIoAccept, FALSE);
		if (g_pCtxtListenSocket == NULL)
		{
			myprintf("failed to update listen socket to IOCP\n");
			return (FALSE);
		}

		nRet = WSAIoctl(
			g_sdListen,
			SIO_GET_EXTENSION_FUNCTION_POINTER,
			&acceptex_guid,
			sizeof(acceptex_guid),
			&g_pCtxtListenSocket->fnAcceptEx,
			sizeof(g_pCtxtListenSocket->fnAcceptEx),
			&bytes,
			NULL,
			NULL
		);
		if (nRet == SOCKET_ERROR)
		{
			myprintf("failed to load AcceptEx: %d\n", WSAGetLastError());
			return (FALSE);
		}
	}

	g_pCtxtListenSocket->pIOContext->SocketAccept = CreateSocket();
	if (g_pCtxtListenSocket->pIOContext->SocketAccept == INVALID_SOCKET)
	{
		myprintf("failed to create new accept socket\n");
		return (FALSE);
	}

	nRet = g_pCtxtListenSocket->fnAcceptEx(g_sdListen, g_pCtxtListenSocket->pIOContext->SocketAccept,
		(LPVOID)(g_pCtxtListenSocket->pIOContext->Buffer),
		MAX_BUFF_SIZE - (2 * (sizeof(SOCKADDR_STORAGE) + 16)),
		sizeof(SOCKADDR_STORAGE) + 16, sizeof(SOCKADDR_STORAGE) + 16,
		&dwRecvNumBytes,
		(LPOVERLAPPED) &(g_pCtxtListenSocket->pIOContext->Overlapped));

	if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError()))
	{
		myprintf("AcceptEx() failed: %d\n", WSAGetLastError());
		return (FALSE);
	}

	return (TRUE);
}

DWORD WINAPI WorkerThread(LPVOID WorkThreadContext)
{
	HANDLE hIOCP = (HANDLE)WorkThreadContext;
	BOOL bSuccess = FALSE;
	int nRet = 0;
	LPWSAOVERLAPPED lpOverlapped = NULL;
	PPER_SOCKET_CONTEXT lpPerSocketContext = NULL;
	PPER_SOCKET_CONTEXT lpAcceptSocketContext = NULL;
	PPER_IO_CONTEXT lpIOContext = NULL;
	WSABUF buffRecv;
	WSABUF buffSend;
	DWORD dwRecvNumBytes = 0;
	DWORD dwSendNumBytes = 0;
	DWORD dwFlags = 0;
	DWORD dwIoSize = 0;
	HRESULT hRet;

	while (TRUE)
	{
		bSuccess = GetQueuedCompletionStatus(
			hIOCP,
			&dwIoSize,
			(PDWORD_PTR)&lpPerSocketContext,
			(LPOVERLAPPED *)&lpOverlapped,
			INFINITE
		);

		if (!bSuccess)
			myprintf("GetQueuedCompletionStatus() failed: %d\n", GetLastError());

		if (lpPerSocketContext == NULL)
		{
			// ctrl-c handler
			return (0);
		}

		if (g_bEndServer)
		{
			return (0);
		}

		lpIOContext = (PPER_IO_CONTEXT)lpOverlapped;

		if (lpIOContext->IOOperation != ClientIoAccept)
		{
			if (!bSuccess || (bSuccess && (0 == dwIoSize)))
			{
				// client connection dropped
				CloseClient(lpPerSocketContext, FALSE);
			}
		}

		switch (lpIOContext->IOOperation)
		{
		case ClientIoAccept:
			nRet = setsockopt(
				lpPerSocketContext->pIOContext->SocketAccept,
				SOL_SOCKET,
				SO_UPDATE_ACCEPT_CONTEXT,
				(char *)&g_sdListen,
				sizeof(g_sdListen)
				);

			if (nRet == SOCKET_ERROR)
			{
				myprintf("setsocket(SO_UPDATE_ACCEPT_CONTEXT) failed to update accept socket\n");
				WSASetEvent(g_hCleanupEvent[0]);
				return (0);
			}

			lpAcceptSocketContext = UpdateCompletionPort(
				lpPerSocketContext->pIOContext->SocketAccept,
				ClientIoAccept, TRUE);

			if (lpAcceptSocketContext == NULL)
			{
				myprintf("failed to update accept socket to IOCP\n");
				WSASetEvent(g_hCleanupEvent[0]);
				return (0);
			}

			if (dwIoSize)
			{
				lpAcceptSocketContext->pIOContext->IOOperation = ClientIoWrite;
				lpAcceptSocketContext->pIOContext->nTotalBytes = dwIoSize;
				lpAcceptSocketContext->pIOContext->nSentBytes = 0;
				lpAcceptSocketContext->pIOContext->wsabuf.len = dwIoSize;
				hRet = StringCbCopyN(lpAcceptSocketContext->pIOContext->Buffer,
					MAX_BUFF_SIZE,
					lpPerSocketContext->pIOContext->Buffer,
					sizeof(lpPerSocketContext->pIOContext->Buffer));
				lpAcceptSocketContext->pIOContext->wsabuf.buf = lpAcceptSocketContext->pIOContext->Buffer;

				nRet = WSASend(
					lpPerSocketContext->pIOContext->SocketAccept,
					&lpAcceptSocketContext->pIOContext->wsabuf, 1,
					&dwSendNumBytes,
					0,
					&(lpAcceptSocketContext->pIOContext->Overlapped), NULL);

				if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError()))
				{
					myprintf("WSASend() failed: %d\n", WSAGetLastError());
					CloseClient(lpAcceptSocketContext, FALSE);
				}
				else if (g_bVerbose)
				{
					myprintf("WorkerThread %d: Socket(%d) AcceptEx completed (%d bytes), Send posted\n",
						GetCurrentThreadId(), lpPerSocketContext->Socket, dwIoSize);
				}
			}
			else
			{
				lpAcceptSocketContext->pIOContext->IOOperation = ClientIoRead;
				dwRecvNumBytes = 0;
				dwFlags = 0;
				buffRecv.buf = lpAcceptSocketContext->pIOContext->Buffer;
				buffRecv.len = MAX_BUFF_SIZE;
				nRet = WSARecv(
					lpAcceptSocketContext->Socket,
					&buffRecv, 1,
					&dwRecvNumBytes,
					&dwFlags,
					&lpAcceptSocketContext->pIOContext->Overlapped, NULL);

				if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError()))
				{
					myprintf("WSARecv() failed: %d\n", WSAGetLastError());
					CloseClient(lpAcceptSocketContext, FALSE);
				}
			}

			if (!CreateAcceptSocket(FALSE))
			{
				myprintf("Please shut down and reboot the server.\n");
				WSASetEvent(g_hCleanupEvent[0]);
				return (0);
			}
			break;

		case ClientIoRead:
			lpIOContext->IOOperation = ClientIoWrite;
			lpIOContext->nTotalBytes = dwIoSize;
			lpIOContext->nSentBytes = 0;
			lpIOContext->wsabuf.len = dwIoSize;
			dwFlags = 0;
			nRet = WSASend(
				lpPerSocketContext->Socket,
				&lpIOContext->wsabuf, 1, &dwSendNumBytes,
				dwFlags,
				&(lpIOContext->Overlapped), NULL);

			if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError()))
			{
				myprintf("WSASend() failed: %d\n", WSAGetLastError());
				CloseClient(lpPerSocketContext, FALSE);
			}
			else if (g_bVerbose)
			{
				myprintf("WorkerThread %d: Socket(%d) Recv completed (%d bytes), Send posted\n",
					GetCurrentThreadId(), lpPerSocketContext->Socket, dwIoSize);
			}
			break;

		case ClientIoWrite:

			lpIOContext->IOOperation = ClientIoWrite;
			lpIOContext->nSentBytes += dwIoSize;
			dwFlags = 0;
			if (lpIOContext->nSentBytes < lpIOContext->nTotalBytes)
			{
				// 上一个send未能发送所有数据
				buffSend.buf = lpIOContext->Buffer + lpIOContext->nSentBytes;
				buffSend.len = lpIOContext->nTotalBytes - lpIOContext->nSentBytes;
				nRet = WSASend(
					lpPerSocketContext->Socket,
					&buffSend, 1, &dwSendNumBytes,
					dwFlags,
					&(lpIOContext->Overlapped), NULL);
				
				if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError()))
				{
					myprintf("WSASend() failed: %d\h", WSAGetLastError());
					CloseClient(lpPerSocketContext, FALSE);
				}
				else if (g_bVerbose)
				{
					myprintf("WorkerThread %d: Socket(%d) Send partially completed (%d bytes), Recv posted\n",
						GetCurrentThreadId(), lpPerSocketContext->Socket, dwIoSize);
				}
			}
			else
			{
				// 上一个写操作已经完成，发出下一个读操作请求
				lpIOContext->IOOperation = ClientIoRead;
				dwRecvNumBytes = 0;
				dwFlags = 0;
				buffRecv.buf = lpIOContext->Buffer;
				buffRecv.len = MAX_BUFF_SIZE;
				nRet = WSARecv(
					lpPerSocketContext->Socket,
					&buffRecv, 1, &dwRecvNumBytes,
					&dwFlags,
					&lpIOContext->Overlapped, NULL);

				if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError())) {
					myprintf("WSARecv() failed: %d\n", WSAGetLastError());
					CloseClient(lpPerSocketContext, FALSE);
				}
				else if (g_bVerbose) {
					myprintf("WorkerThread %d: Socket(%d) Send completed (%d bytes), Recv posted\n",
						GetCurrentThreadId(), lpPerSocketContext->Socket, dwIoSize);
				}
			}
			break;

		}
	}
	return (0);
}
