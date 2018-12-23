// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (C) Microsoft Corporation.  All Rights Reserved.
//
// Module:
//      iocpserver.cpp
//
// Abstract:
//      This program is a Winsock echo server program that uses I/O Completion Ports 
//      (IOCP) to receive data from and echo data back to a sending client. The server 
//      program supports multiple clients connecting via TCP/IP and sending arbitrary 
//      sized data buffers which the server then echoes back to the client.  For 
//      convenience a simple client program, iocpclient was developed to connect 
//      and continually send data to the server to stress it.
//
//      Direct IOCP support was added to Winsock 2 and is fully implemented on the NT 
//      platform.  IOCPs provide a model for developing very high performance and very 
//      scalable server programs.
//
//      The basic idea is that this server continuously accepts connection requests from 
//      a client program.  When this happens, the accepted socket descriptor is added to 
//      the existing IOCP and an initial receive (WSARecv) is posted on that socket.  When 
//      the client then sends data on that socket, a completion packet will be delivered 
//      and handled by one of the server's worker threads.  The worker thread echoes the 
//      data back to the sender by posting a send (WSASend) containing all the data just 
//      received.  When sending the data back to the client completes, another completion
//      packet will be delivered and again handled by one of the server's worker threads.  
//      Assuming all the data that needed to be sent was actually sent, another receive 
//      (WSARecv) is once again posted and the scenario repeats itself until the client 
//      stops sending data.
//
//      When using IOCPs it is important to remember that the worker threads must be able
//      to distinguish between I/O that occurs on multiple handles in the IOCP as well as 
//      multiple I/O requests initiated on a single handle.  The per handle data 
//      (PER_SOCKET_CONTEXT) is associated with the handle as the CompletionKey when the 
//      handle is added to the IOCP using CreateIoCompletionPort.  The per IO operation 
//      data (PER_IO_CONTEXT) is associated with a specific handle during an I/O 
//      operation as part of the overlapped structure passed to either WSARecv or 
//      WSASend.  Please notice that the first member of the PER_IO_CONTEXT structure is 
//      a WSAOVERLAPPED structure (compatible with the Win32 OVERLAPPED structure).  
//
//      When the worker thread unblocks from GetQueuedCompletionStatus, the key 
//      associated with the handle when the handle was added to the IOCP is returned as 
//      well as the overlapped structure associated when this particular I/O operation 
//      was initiated.
//      
//      This program cleans up all resources and shuts down when CTRL-C is pressed.  
//      This will cause the main thread to break out of the accept loop and close all open 
//      sockets and free all context data.  The worker threads get unblocked by posting  
//      special I/O packets with a NULL CompletionKey to the IOCP.  The worker 
//      threads check for a NULL CompletionKey and exits if it encounters one. If CTRL-BRK 
//      is pressed instead, cleanup process is same as above but instead of exit the process, 
//      the program loops back to restart the server.

//      Another point worth noting is that the Win32 API CreateThread() does not 
//      initialize the C Runtime and therefore, C runtime functions such as 
//      printf() have been avoid or rewritten (see myprintf()) to use just Win32 APIs.
//
//  Usage:
//      Start the server and wait for connections on port 6001
//          iocpserver -e:6001
//
//  Build:
//      Use the headers and libs from the April98 Platform SDK or later.
//      Link with ws2_32.lib
//      
//
//

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define xmalloc(s)	HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(s))
#define xfree(p)	HeapFree(GetProcessHeap(),0,(p))

#include <WinSock2.h>
#include <MSWSock.h>
#include <WS2tcpip.h>
#include <strsafe.h>

#include "server.h"

char *g_Port = DEFAULT_PORT;	// 端口
BOOL g_bEndServer = FALSE;	// 
BOOL g_bRestart = TRUE;		// 
BOOL g_bVerbose = FALSE;	// 详细信息
HANDLE g_hIOCP = INVALID_HANDLE_VALUE;		// IOCP句柄
SOCKET g_sdListen = INVALID_SOCKET;			// 监听套接字
HANDLE g_ThreadHandles[MAX_WORKER_THREAD];	// 线程
WSAEVENT g_hCleanupEvent[1];				// 清理事件
PPER_SOCKET_CONTEXT g_pCtxtListenSocket = NULL;	// 监听套接字环境
PPER_SOCKET_CONTEXT g_pCtxtList = NULL;			// 套接字环境链表

CRITICAL_SECTION g_CriticalSection;	// 临界区

int myprintf(const TCHAR *lpFormat, ...)
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

	if (SUCCEEDED(hRet))
	//if (dwRet >= dwLen || GetLastError() == 0)
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

	// 控制台事件处理回调
	if (!SetConsoleCtrlHandler(CtrlHandler, TRUE))
	{
		myprintf(TEXT("SetConsoleCtrlHandler() failed to install console handler: %d\n"),
			GetLastError());
		return;
	}
	
	GetSystemInfo(&systemInfo);
	dwThreadCount = systemInfo.dwNumberOfProcessors * 2;

	if (WSA_INVALID_EVENT == (g_hCleanupEvent[0] = WSACreateEvent()))
	{
		myprintf(TEXT("WSACreateEvent() failed: %d\n"), WSAGetLastError());
		return;
	}

	if ((nRet = WSAStartup(0x202, &wsaData)) != 0) {
		myprintf(TEXT("WSAStartup() failed: %d\n"), nRet);
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
		myprintf(TEXT("InitializeCriticalSection raised an exception.\n"));
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
			// 创建IO完成端口并将其与指定的文件句柄关联，或创建未与文件句柄关联的IO完成端口，以便以后进行关联
			g_hIOCP = CreateIoCompletionPort(
				// 
				// 打开的文件句柄或INVALID_HANDLE_VALUE
				// 句柄对象必须支持重叠I/O
				//
				// 如果提供了句柄，则必须打开它的重叠IO选项。
				// 例如，在使用CreateFile函数获取句柄时，必须指定FILE_FLAG_OVERLAPPED标志。
				//
				// 如果指定了INVALID_HANDLE_VALUE，则该函数会创建一个I / O完成端口，而不会将其与文件句柄相关联。
				// 在这种情况下，ExistingCompletionPort参数必须为NULL并且忽略CompletionKey参数。
				INVALID_HANDLE_VALUE, 
				//
				// 现有I / O完成端口的句柄或NULL。
				// 如果此参数指定现有I / O完成端口，则该函数将其与FileHandle参数指定的句柄相关联。 
				// 如果成功，该函数返回现有I / O完成端口的句柄; 它不会创建新的I / O完成端口。
				//
				// 如果此参数为NULL，则该函数将创建新的I / O完成端口，如果FileHandle参数有效，则将其与新的I / O完成端口关联。 
				// 否则，不会发生文件句柄关联。 如果成功，该函数将返回新I / O完成端口的句柄。
				NULL, 
				//
				// 每个句柄用户定义的完成密钥，包含在指定文件句柄的每个I / O完成数据包中。
				0, 
				//
				// 操作系统可以允许同时处理I / O完成端口的I / O完成数据包的最大线程数。
				// 如果ExistingCompletionPort参数不为NULL，则忽略此参数。
				//
				// 如果此参数为零，则系统允许与系统中的处理器一样多的并发运行线程。
				0);

			if (g_hIOCP == NULL)
			{
				myprintf(TEXT("CreateIoCompletionPort() failed to create I/O completion port: %d\n"),
					GetLastError());
				__leave;
			}

			for (DWORD dwCPU = 0; dwCPU < dwThreadCount; ++dwCPU)
			{
				HANDLE hThread;
				DWORD dwThreadId;
				// 创建线程
				hThread = CreateThread(NULL, 0, WorkerThread, g_hIOCP, 0, &dwThreadId);
				if (hThread == NULL)
				{
					myprintf(TEXT("CreateThread() failed to create worker thread: %d\n"),
						GetLastError());
					__leave;
				}
				g_ThreadHandles[dwCPU] = hThread;
				hThread = INVALID_HANDLE_VALUE;
			}

			// 创建监听套接字
			if (!CreateListenSocket())
				__leave;

			// 创捷接收套接字
			if (!CreateAcceptSocket(TRUE))
				__leave;

			// 等待清理事件
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
					// 将I / O完成数据包发布到I / O完成端口。
					PostQueuedCompletionStatus(g_hIOCP, 0, 0, NULL);
				}
			}

			// 等待一个或所有指定对象处于信号状态或调用超时。
			if (WAIT_OBJECT_0 != WaitForMultipleObjects(dwThreadCount, g_ThreadHandles, TRUE, 1000))
				myprintf(TEXT("WaitForMultipleObjects() failed: %d\n"), GetLastError());
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
			myprintf(TEXT("\niocpserverex is restarting...\n"));
		}
		else
		{
			myprintf(TEXT("\niocpserverex is exiting...\n"));
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
				myprintf(TEXT("Usage:\n  socket [-e:port] [-v] [-?]\n"));
				myprintf(TEXT("  -e:port\tSpecify echoing port number\n"));
				myprintf(TEXT("  -v\t\tVerbose\n"));
				myprintf(TEXT("  -?\t\tDisplay this help\n"));
				bRet = FALSE;
				break;

			default:
				myprintf(TEXT("Unknown options flag %s\n"), argv[i]);
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
			myprintf(TEXT("CtrlHandler: closing listening socket\n"));

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
		myprintf(TEXT("WSASocket(sdSocket) failed: %d\n"), WSAGetLastError());
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
		myprintf(TEXT("setsockopt(SNDBUF) failed: %d\n"), WSAGetLastError());
		return (sdSocket);
	}

	//
	// linger value （不要设置）
	// 设置过短的linger可能会导致一些数据丢失
	// 如果需要防止一些恶意连接，只连接不发送数据
	// 服务器可以设置一个timer，等到一定时间之后再设置linger value为舍弃
	// 
	/*
	LINGER lingerStruct;
	lingerStruct.l_onoff = 1;
	lingerStruct.l_linger = 0;
	nRet = setsockopt(sdSocket, SOL_SOCKET, SO_LINGER,
		(char *)&lingerStruct, sizeof(lingerStruct));
	if (nRet == SOCKET_ERROR)
	{
		myprintf(TEXT("setsockopt(SO_LINGER) failed: %d\n"), WSAGetLastError());
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

	// getaddrinfo函数提供独立于协议的从ANSI主机名到地址的转换。
	if (getaddrinfo(
		// 指向以NULL结尾的ANSI字符串的指针，该字符串包含主机（节点）名称或数字主机地址字符串。
		// 对于Internet协议，数字主机地址字符串是点分十进制IPv4地址或IPv6十六进制地址。
		NULL, 
		// 指向以NULL结尾的ANSI字符串的指针，该字符串包含表示为字符串的服务名称或端口号。 
		// 服务名称是端口号的字符串别名。 
		// 例如，“http”是由Internet工程任务组（IETF）定义的端口80的别名，作为Web服务器用于HTTP协议的默认端口。 
		// 可用服务名称：%WINDIR%\system32\drivers\etc\services
		g_Port, 
		// 指向addrinfo结构的指针，该结构提供有关调用者支持的套接字类型的提示。
		// pHints参数指向的addrinfo结构的ai_addrlen，ai_canonname，ai_addr和ai_next成员必须为零或NULL。
		// 否则，GetAddrInfoEx函数将失败并返回WSANO_RECOVERY。
		&hints, 
		// 指向包含有关主机的响应信息的一个或多个addrinfo结构的链接列表的指针 。
		&addrlocal) != 0)
	{
		myprintf(TEXT("getaddrinfo() failed with error %d\n"), WSAGetLastError());
		return (FALSE);
	}

	if (addrlocal == NULL)
	{
		myprintf(TEXT("getaddrinfo() failed to resolve/convert the interface\n"));
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
		myprintf(TEXT("bind() failed: %d\n"), WSAGetLastError());
		freeaddrinfo(addrlocal);
		return (FALSE);
	}

	nRet = listen(g_sdListen, 5);
	if (nRet == SOCKET_ERROR)
	{
		myprintf(TEXT("listen() failed: %d\n"), WSAGetLastError());
		freeaddrinfo(addrlocal);
		return (FALSE);
	}
	
	freeaddrinfo(addrlocal);

	return (TRUE);
}

// 创建接收套接字
BOOL CreateAcceptSocket(BOOL fUpdateIOCP)
{
	int nRet = 0;
	DWORD dwRecvNumBytes = 0;
	DWORD bytes = 0;

	GUID acceptex_guid = WSAID_ACCEPTEX;

	if (fUpdateIOCP)
	{
		// 创建监听套接字的上下文
		g_pCtxtListenSocket = UpdateCompletionPort(g_sdListen, ClientIoAccept, FALSE);
		if (g_pCtxtListenSocket == NULL)
		{
			myprintf(TEXT("failed to update listen socket to IOCP\n"));
			return (FALSE);
		}

		// 获取AcceptEx的函数地址

		// WSAIoctl函数控制套接字的模式。
		nRet = WSAIoctl(
			g_sdListen,
			// 
			// 执行的操作控制代码。
			SIO_GET_EXTENSION_FUNCTION_POINTER,
			//
			// 指向输入缓冲区的指针。
			&acceptex_guid,
			// 
			// 输入缓冲区的大小（以字节为单位）。
			sizeof(acceptex_guid),
			//
			// 指向输出缓冲区的指针。
			&g_pCtxtListenSocket->fnAcceptEx,
			// 
			// 输出缓冲区的大小（以字节为单位）。
			sizeof(g_pCtxtListenSocket->fnAcceptEx),
			//
			// 指向实际输出字节数的指针。
			&bytes,
			//
			// 指向WSAOVERLAPPED结构的指针（对于非重叠套接字，将被忽略）。
			NULL,
			//
			// 操作完成时调用的完成例程指针（对于非重叠套接字，将被忽略）。
			NULL
		);
		if (nRet == SOCKET_ERROR)
		{
			myprintf(TEXT("failed to load AcceptEx: %d\n"), WSAGetLastError());
			return (FALSE);
		}
	}

	g_pCtxtListenSocket->pIOContext->SocketAccept = CreateSocket();
	if (g_pCtxtListenSocket->pIOContext->SocketAccept == INVALID_SOCKET)
	{
		myprintf(TEXT("failed to create new accept socket\n"));
		return (FALSE);
	}

	// AcceptEx函数接受新连接，返回本地和远程地址，并接收客户端应用程序发送的第一个数据块。
	nRet = g_pCtxtListenSocket->fnAcceptEx(
		// 标识已使用listen函数调用的套接字的描述符。
		// 服务器应用程序等待连接此套接字的尝试。
		g_sdListen, 
		// 标识要接受传入连接的套接字的描述符。非绑定或已连接
		g_pCtxtListenSocket->pIOContext->SocketAccept,
		// 指向缓冲区的指针，该缓冲区接收在新连接上发送的第一个数据块，服务器的本地地址以及客户端的远程地址。 
		// 接收数据从偏移零开始写入缓冲区的第一部分，而地址写入缓冲区的后半部分。 
		// 必须指定此参数。
		(LPVOID)(g_pCtxtListenSocket->pIOContext->Buffer),
		// lpOutputBuffer中将用于缓冲区开头的实际接收数据的字节数。 
		// 此大小不应包括服务器本地地址的大小，也不应包括客户端的远程地址; 它们被附加到输出缓冲区。 
		// 如果dwReceiveDataLength为零，则接受连接将不会导致接收操作。 
		// 而是连接一旦到达，AcceptEx就会完成，不会等待任何数据。
		MAX_BUFF_SIZE - (2 * (sizeof(SOCKADDR_STORAGE) + 16)),
		// 为本地地址信息保留的字节数。
		// 此值必须至少比使用的传输协议的最大地址长度长16个字节。
		sizeof(SOCKADDR_STORAGE) + 16, 
		// 为远程地址信息保留的字节数。
		// 此值必须至少比使用的传输协议的最大地址长度长16个字节。不能为零。
		sizeof(SOCKADDR_STORAGE) + 16,
		// 指向DWORD的指针，该DWORD接收接收的字节数。 
		// 仅当操作同步完成时才设置此参数。 
		// 如果它返回ERROR_IO_PENDING并在稍后完成，则永远不会设置此变量，您必须获取从完成通知机制读取的字节数。
		&dwRecvNumBytes,
		// OVERLAPPED结构，用于处理请求。必须指定此参数;它不能为NULL。
		(LPOVERLAPPED) &(g_pCtxtListenSocket->pIOContext->Overlapped));

	if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError()))
	{
		myprintf(TEXT("AcceptEx() failed: %d\n"), WSAGetLastError());
		return (FALSE);
	}

	return (TRUE);
}

// 工作线程
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
		// 
		// 尝试从指定的I / O完成端口出列I / O完成数据包。 
		// 如果没有排队的完成数据包，该函数将等待与完成端口关联的待处理I / O操作完成。 
		// 要一次出列多个I / O完成数据包，请使用GetQueuedCompletionStatusEx函数。
		bSuccess = GetQueuedCompletionStatus(
			// 
			// 完成端口的句柄。
			// 要创建完成端口，请使用CreateIoCompletionPort函数。
			hIOCP,
			// 
			// 指向变量的指针，该变量接收已完成的I / O操作期间传输的字节数。
			&dwIoSize,
			//
			// 指向变量的指针，该变量接收与I / O操作已完成的文件句柄关联的完成键值。
			// 完成密钥是在调用CreateIoCompletionPort时指定的每文件密钥。
			(PDWORD_PTR)&lpPerSocketContext,
			// 
			// 指向变量的指针，该变量接受在启动完成的I/O操作时指定的OVERLAPPED结构的地址
			//
			// 即使您已将函数传递给与完成端口关联的文件句柄和有效的OVERLAPPED接口，应用程序也可以阻止完成端口通知。
			// 这是通过位OVERLAPPED结构的hEvent成员指定有效的事件句柄并设置其低位来完成的。
			// 设置了低位的有效事件句柄使I/O完毕不会排队到完成端口。
			(LPOVERLAPPED *)&lpOverlapped,
			// 
			// 调用者愿意等待完成数据包出现在完成端口的毫秒数。 
			// 如果在指定时间内未出现完成包，则该函数超时，返回FALSE，并将* lpOverlapped设置为NULL。
			// 
			// 如果dwMilliseconds是INFINITE，则该函数永远不会超时。
			// 如果dwMilliseconds为零并且没有出队的I / O操作，则该函数将立即超时。
			INFINITE
		);

		if (!bSuccess)
			myprintf(TEXT("GetQueuedCompletionStatus() failed: %d\n"), GetLastError());

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
				// 该SO_CONDITIONAL_ACCEPT套接字选项被设计为允许应用程序决定是否要接受一个监听套接字传入的连接。
				SO_UPDATE_ACCEPT_CONTEXT,
				(char *)&g_sdListen,
				sizeof(g_sdListen)
				);

			if (nRet == SOCKET_ERROR)
			{
				myprintf(TEXT("setsocket(SO_UPDATE_ACCEPT_CONTEXT) failed to update accept socket\n"));
				WSASetEvent(g_hCleanupEvent[0]);
				return (0);
			}

			// 刷新完成端口
			lpAcceptSocketContext = UpdateCompletionPort(
				lpPerSocketContext->pIOContext->SocketAccept,
				ClientIoAccept, TRUE);

			if (lpAcceptSocketContext == NULL)
			{
				myprintf(TEXT("failed to update accept socket to IOCP\n"));
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
					// 
					// 指向WSABUF结构数组的指针 。
					// 每个 WSABUF结构都包含一个指向缓冲区的指针和缓冲区的长度（以字节为单位）。
					// 对于Winsock应用程序，一旦调用了 WSASend函数，系统就拥有这些缓冲区，应用程序可能无法访问它们。
					// 此数组必须在发送操作期间保持有效。
					&lpAcceptSocketContext->pIOContext->wsabuf, 1, // lpBuffers数组中的WSABUF结构数 。
					// 
					// 指向以字节为单位的数字的指针，如果I/O操作立即完成，将会被发送。
					// 如果lpOverlapped参数不为NULL，则对此参数使用NULL以避免可能的错误结果。
					// 仅当lpOverlapped参数不为NULL时，此参数才可以为NULL。
					&dwSendNumBytes,
					// 用于修改WSASend函数调用行为的标志。
					0,
					//
					// 指向WSAOVERLAPPED结构体的指针。
					// 对于非重叠套接字，将忽略此参数。
					&(lpAcceptSocketContext->pIOContext->Overlapped),
					// 
					// 发送操作完成时调用的指向完成例程的指针。
					// 对于非重叠套接字，将忽略此参数。
					NULL);

				if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError()))
				{
					myprintf(TEXT("WSASend() failed: %d\n"), WSAGetLastError());
					CloseClient(lpAcceptSocketContext, FALSE);
				}
				else if (g_bVerbose)
				{
					myprintf(TEXT("WorkerThread %d: Socket(%d) AcceptEx completed (%d bytes), Send posted\n"),
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
					myprintf(TEXT("WSARecv() failed: %d\n"), WSAGetLastError());
					CloseClient(lpAcceptSocketContext, FALSE);
				}
			}

			if (!CreateAcceptSocket(FALSE))
			{
				myprintf(TEXT("Please shut down and reboot the server.\n"));
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
				myprintf(TEXT("WSASend() failed: %d\n"), WSAGetLastError());
				CloseClient(lpPerSocketContext, FALSE);
			}
			else if (g_bVerbose)
			{
				myprintf(TEXT("WorkerThread %d: Socket(%d) Recv completed (%d bytes), Send posted\n"),
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
					myprintf(TEXT("WSASend() failed: %d\n"), WSAGetLastError());
					CloseClient(lpPerSocketContext, FALSE);
				}
				else if (g_bVerbose)
				{
					myprintf(TEXT("WorkerThread %d: Socket(%d) Send partially completed (%d bytes), Recv posted\n"),
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
					myprintf(TEXT("WSARecv() failed: %d\n"), WSAGetLastError());
					CloseClient(lpPerSocketContext, FALSE);
				}
				else if (g_bVerbose) {
					myprintf(TEXT("WorkerThread %d: Socket(%d) Send completed (%d bytes), Recv posted\n"),
						GetCurrentThreadId(), lpPerSocketContext->Socket, dwIoSize);
				}
			}
			break;

		}
	}
	return (0);
}

PPER_SOCKET_CONTEXT UpdateCompletionPort(SOCKET sd, IO_OPERATION ClientIo, BOOL bAddToList)
{
	PPER_SOCKET_CONTEXT lpPerSocketContext;

	lpPerSocketContext = CtxtAllocate(sd, ClientIo);
	if (lpPerSocketContext == NULL)
	{
		return (NULL);
	}

	// 将套接字关联到现有的IO完成端口
	g_hIOCP = CreateIoCompletionPort((HANDLE)sd, g_hIOCP, (DWORD_PTR)lpPerSocketContext, 0);
	if (g_hIOCP == NULL)
	{
		myprintf(TEXT("CreateIoCompletionPort() failed: %d\n"), GetLastError());
		if (lpPerSocketContext->pIOContext)
			xfree(lpPerSocketContext->pIOContext);
		xfree(lpPerSocketContext);
		return (NULL);
	}

	if (bAddToList) CtxtListAddTo(lpPerSocketContext);

	if (g_bVerbose)
		myprintf(TEXT("UpdateCompletionPort: Socket(%d) added to IOCP\n"), lpPerSocketContext->Socket);

	return (lpPerSocketContext);
}

VOID CloseClient(PPER_SOCKET_CONTEXT lpPerSocketContext, BOOL bGraceful)
{
	__try
	{
		// 进入临界区
		EnterCriticalSection(&g_CriticalSection);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		myprintf(TEXT("EnterCriticalSection raised an exception.\n"));
		return;
	}

	if (lpPerSocketContext)
	{
		if (g_bVerbose)
			myprintf(TEXT("CloseClient: Socket(%d) connection closing (graceful=%s)\n"),
				lpPerSocketContext->Socket, (bGraceful ? "TRUE" : "FALSE"));

		if (!bGraceful)
		{
			//
			// force the subsequent closesocket to be abortative
			//
			LINGER lingerStruct;

			lingerStruct.l_onoff = 1;
			lingerStruct.l_linger = 0;
			setsockopt(lpPerSocketContext->Socket, SOL_SOCKET, SO_LINGER,
				(char *)&lingerStruct, sizeof(lingerStruct));
		}
		if (lpPerSocketContext->pIOContext->SocketAccept != INVALID_SOCKET)
		{
			closesocket(lpPerSocketContext->pIOContext->SocketAccept);
			lpPerSocketContext->pIOContext->SocketAccept = INVALID_SOCKET;
		}

		closesocket(lpPerSocketContext->Socket);
		lpPerSocketContext->Socket = INVALID_SOCKET;
		CtxtListDeleteFrom(lpPerSocketContext);
		lpPerSocketContext = NULL;
	}
	else
	{
		myprintf(TEXT("CloseClient: lpPerSocketContext is NULL\n"));
	}

	LeaveCriticalSection(&g_CriticalSection);

	return;
}

PPER_SOCKET_CONTEXT CtxtAllocate(SOCKET sd, IO_OPERATION ClientIO)
{
	PPER_SOCKET_CONTEXT lpPerSocketContext;

	__try
	{
		EnterCriticalSection(&g_CriticalSection);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		myprintf(TEXT("EnterCriticalSection raised an exception.\n"));
		return NULL;
	}

	lpPerSocketContext = (PPER_SOCKET_CONTEXT)xmalloc(sizeof(PER_SOCKET_CONTEXT));
	if (lpPerSocketContext)
	{
		lpPerSocketContext->pIOContext = (PPER_IO_CONTEXT)xmalloc(sizeof(PER_IO_CONTEXT));
		if (lpPerSocketContext->pIOContext)
		{
			lpPerSocketContext->Socket = sd;
			lpPerSocketContext->pCtxtBack = NULL;
			lpPerSocketContext->pCtxtForward = NULL;

			lpPerSocketContext->pIOContext->Overlapped.Internal = 0;
			lpPerSocketContext->pIOContext->Overlapped.InternalHigh = 0;
			lpPerSocketContext->pIOContext->Overlapped.Offset = 0;
			lpPerSocketContext->pIOContext->Overlapped.OffsetHigh = 0;
			lpPerSocketContext->pIOContext->Overlapped.hEvent = NULL;
			lpPerSocketContext->pIOContext->IOOperation = ClientIO;
			lpPerSocketContext->pIOContext->pIOContextForward = NULL;
			lpPerSocketContext->pIOContext->nTotalBytes = 0;
			lpPerSocketContext->pIOContext->nSentBytes = 0;
			lpPerSocketContext->pIOContext->wsabuf.buf = lpPerSocketContext->pIOContext->Buffer;
			lpPerSocketContext->pIOContext->wsabuf.len = sizeof(lpPerSocketContext->pIOContext->Buffer);
			lpPerSocketContext->pIOContext->SocketAccept = INVALID_SOCKET;

			ZeroMemory(lpPerSocketContext->pIOContext->wsabuf.buf, lpPerSocketContext->pIOContext->wsabuf.len);
		}
		else
		{
			xfree(lpPerSocketContext);
			myprintf(TEXT("HealAlloc() PER_IO_CONTEXT failed: %d\n"), GetLastError());
		}
	}
	else
	{
		myprintf(TEXT("HeapAlloc() PER_SOCKET_CONTEXT failed: %d\n"), GetLastError());
		return (NULL);
	}

	LeaveCriticalSection(&g_CriticalSection);

	return (lpPerSocketContext);
}

VOID CtxtListAddTo(PPER_SOCKET_CONTEXT lpPerSocketContext)
{
	PPER_SOCKET_CONTEXT pTemp;

	__try
	{
		EnterCriticalSection(&g_CriticalSection);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		myprintf(TEXT("EnterCriticalSection raised an exception\n"));
		return;
	}

	if (g_pCtxtList == NULL)
	{
		// add the first node
		lpPerSocketContext->pCtxtBack = NULL;
		lpPerSocketContext->pCtxtForward = NULL;
		g_pCtxtList = lpPerSocketContext;
	}
	else
	{
		// add the node to head of list
		pTemp = g_pCtxtList;

		g_pCtxtList = lpPerSocketContext;
		lpPerSocketContext->pCtxtBack = pTemp;
		lpPerSocketContext->pCtxtForward = NULL;

		pTemp->pCtxtForward = lpPerSocketContext;
	}

	LeaveCriticalSection(&g_CriticalSection);

	return;
}

void CtxtListDeleteFrom(PPER_SOCKET_CONTEXT lpPerSocketContext)
{
	PPER_SOCKET_CONTEXT pBack;
	PPER_SOCKET_CONTEXT pForward;
	PPER_IO_CONTEXT pNextIO = NULL;
	PPER_IO_CONTEXT pTempIO = NULL;

	__try
	{
		EnterCriticalSection(&g_CriticalSection);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		myprintf(TEXT("EnterCriticalSection raised an exception.\n"));
		return;
	}

	if (lpPerSocketContext)
	{
		pBack = lpPerSocketContext->pCtxtBack;
		pForward = lpPerSocketContext->pCtxtForward;

		if (pBack == NULL && pForward == NULL)
		{
			// only node
			g_pCtxtList = NULL;
		}
		else if (pBack == NULL && pForward != NULL)
		{
			// start node
			pForward->pCtxtBack = NULL;
			g_pCtxtList = pForward;
		}
		else if (pBack != NULL && pForward == NULL)
		{
			// end node
			pBack->pCtxtForward = NULL;
		}
		else if (pBack && pForward)
		{
			pBack->pCtxtForward = pForward;
			pForward->pCtxtBack = pBack;
		}

		// free all i/o context structures per socket
		pTempIO = (PPER_IO_CONTEXT)(lpPerSocketContext->pIOContext);
		do
		{
			pNextIO = (PPER_IO_CONTEXT)(pTempIO->pIOContextForward);
			if (pTempIO)
			{
				if (g_bEndServer)
					while (!HasOverlappedIoCompleted((LPOVERLAPPED)pTempIO)) Sleep(0);
				xfree(pTempIO);
				pTempIO = NULL;
			}
			pTempIO = pNextIO;
		} while (pNextIO);

		xfree(lpPerSocketContext);
		lpPerSocketContext = NULL;
	}
	else
	{
		myprintf(TEXT("CtxtListDeleteFrom: lpPerSocketContext is NULL\n"));
	}

	LeaveCriticalSection(&g_CriticalSection);

	return;
}

VOID CtxtListFree()
{
	PPER_SOCKET_CONTEXT pTemp1, pTemp2;

	__try
	{
		EnterCriticalSection(&g_CriticalSection);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		myprintf(TEXT("EnterCriticalSection raised an exception.\n"));
		return;
	}

	pTemp1 = g_pCtxtList;
	while (pTemp1)
	{
		pTemp2 = pTemp1->pCtxtBack;
		CloseClient(pTemp1, FALSE);
		pTemp1 = pTemp2;
	}

	LeaveCriticalSection(&g_CriticalSection);

	return;
}
