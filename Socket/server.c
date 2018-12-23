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

char *g_Port = DEFAULT_PORT;	// �˿�
BOOL g_bEndServer = FALSE;	// 
BOOL g_bRestart = TRUE;		// 
BOOL g_bVerbose = FALSE;	// ��ϸ��Ϣ
HANDLE g_hIOCP = INVALID_HANDLE_VALUE;		// IOCP���
SOCKET g_sdListen = INVALID_SOCKET;			// �����׽���
HANDLE g_ThreadHandles[MAX_WORKER_THREAD];	// �߳�
WSAEVENT g_hCleanupEvent[1];				// �����¼�
PPER_SOCKET_CONTEXT g_pCtxtListenSocket = NULL;	// �����׽��ֻ���
PPER_SOCKET_CONTEXT g_pCtxtList = NULL;			// �׽��ֻ�������

CRITICAL_SECTION g_CriticalSection;	// �ٽ���

int myprintf(const TCHAR *lpFormat, ...)
{
	// �ú���ר�û����� - 1k / 2k
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

	// ����̨�¼�����ص�
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

	__try // SEH�쳣ģ�ʹ���
	{
		// ��ʼ���ٽ���
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
			// ����IO��ɶ˿ڲ�������ָ�����ļ�����������򴴽�δ���ļ����������IO��ɶ˿ڣ��Ա��Ժ���й���
			g_hIOCP = CreateIoCompletionPort(
				// 
				// �򿪵��ļ������INVALID_HANDLE_VALUE
				// ����������֧���ص�I/O
				//
				// ����ṩ�˾���������������ص�IOѡ�
				// ���磬��ʹ��CreateFile������ȡ���ʱ������ָ��FILE_FLAG_OVERLAPPED��־��
				//
				// ���ָ����INVALID_HANDLE_VALUE����ú����ᴴ��һ��I / O��ɶ˿ڣ������Ὣ�����ļ�����������
				// ����������£�ExistingCompletionPort��������ΪNULL���Һ���CompletionKey������
				INVALID_HANDLE_VALUE, 
				//
				// ����I / O��ɶ˿ڵľ����NULL��
				// ����˲���ָ������I / O��ɶ˿ڣ���ú���������FileHandle����ָ���ľ��������� 
				// ����ɹ����ú�����������I / O��ɶ˿ڵľ��; �����ᴴ���µ�I / O��ɶ˿ڡ�
				//
				// ����˲���ΪNULL����ú����������µ�I / O��ɶ˿ڣ����FileHandle������Ч���������µ�I / O��ɶ˿ڹ����� 
				// ���򣬲��ᷢ���ļ���������� ����ɹ����ú�����������I / O��ɶ˿ڵľ����
				NULL, 
				//
				// ÿ������û�����������Կ��������ָ���ļ������ÿ��I / O������ݰ��С�
				0, 
				//
				// ����ϵͳ��������ͬʱ����I / O��ɶ˿ڵ�I / O������ݰ�������߳�����
				// ���ExistingCompletionPort������ΪNULL������Դ˲�����
				//
				// ����˲���Ϊ�㣬��ϵͳ������ϵͳ�еĴ�����һ����Ĳ��������̡߳�
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
				// �����߳�
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

			// ���������׽���
			if (!CreateListenSocket())
				__leave;

			// ���ݽ����׽���
			if (!CreateAcceptSocket(TRUE))
				__leave;

			// �ȴ������¼�
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
					// ��I / O������ݰ�������I / O��ɶ˿ڡ�
					PostQueuedCompletionStatus(g_hIOCP, 0, 0, NULL);
				}
			}

			// �ȴ�һ��������ָ���������ź�״̬����ó�ʱ��
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

// ��֤����ѡ��
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

// ����̨�¼���Ӧ ctrl+c | ctrl+break ...
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

	// ���÷��������棬SO_SNDBUF: 0
	// winsockֱ�ӽ����ݴӴ˳���Ļ�����������ʡ��һ���ڴ濽��
	// ����ÿ��ֱ�ӷ���һ��TCP/IP����������û�б�����
	// ���÷���������Ƚ��ý�����������ҪС�ö�

	nZero = 0;
	nRet = setsockopt(sdSocket, SOL_SOCKET, SO_SNDBUF, (char *)&nZero, sizeof(nZero));
	if (nRet == SOCKET_ERROR)
	{
		myprintf(TEXT("setsockopt(SNDBUF) failed: %d\n"), WSAGetLastError());
		return (sdSocket);
	}

	//
	// linger value ����Ҫ���ã�
	// ���ù��̵�linger���ܻᵼ��һЩ���ݶ�ʧ
	// �����Ҫ��ֹһЩ�������ӣ�ֻ���Ӳ���������
	// ��������������һ��timer���ȵ�һ��ʱ��֮��������linger valueΪ����
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

	lingerStruct.l_onoff = 1;	// 0: �����رգ���0: �ȴ�һ��ʱ��
	lingerStruct.l_linger = 0;	// �ȴ�ʱ�䣬��λ��s

	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_IP;

	// getaddrinfo�����ṩ������Э��Ĵ�ANSI����������ַ��ת����
	if (getaddrinfo(
		// ָ����NULL��β��ANSI�ַ�����ָ�룬���ַ��������������ڵ㣩���ƻ�����������ַ�ַ�����
		// ����InternetЭ�飬����������ַ�ַ����ǵ��ʮ����IPv4��ַ��IPv6ʮ�����Ƶ�ַ��
		NULL, 
		// ָ����NULL��β��ANSI�ַ�����ָ�룬���ַ���������ʾΪ�ַ����ķ������ƻ�˿ںš� 
		// ���������Ƕ˿ںŵ��ַ��������� 
		// ���磬��http������Internet���������飨IETF������Ķ˿�80�ı�������ΪWeb����������HTTPЭ���Ĭ�϶˿ڡ� 
		// ���÷������ƣ�%WINDIR%\system32\drivers\etc\services
		g_Port, 
		// ָ��addrinfo�ṹ��ָ�룬�ýṹ�ṩ�йص�����֧�ֵ��׽������͵���ʾ��
		// pHints����ָ���addrinfo�ṹ��ai_addrlen��ai_canonname��ai_addr��ai_next��Ա����Ϊ���NULL��
		// ����GetAddrInfoEx������ʧ�ܲ�����WSANO_RECOVERY��
		&hints, 
		// ָ������й���������Ӧ��Ϣ��һ������addrinfo�ṹ�������б��ָ�� ��
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

// ���������׽���
BOOL CreateAcceptSocket(BOOL fUpdateIOCP)
{
	int nRet = 0;
	DWORD dwRecvNumBytes = 0;
	DWORD bytes = 0;

	GUID acceptex_guid = WSAID_ACCEPTEX;

	if (fUpdateIOCP)
	{
		// ���������׽��ֵ�������
		g_pCtxtListenSocket = UpdateCompletionPort(g_sdListen, ClientIoAccept, FALSE);
		if (g_pCtxtListenSocket == NULL)
		{
			myprintf(TEXT("failed to update listen socket to IOCP\n"));
			return (FALSE);
		}

		// ��ȡAcceptEx�ĺ�����ַ

		// WSAIoctl���������׽��ֵ�ģʽ��
		nRet = WSAIoctl(
			g_sdListen,
			// 
			// ִ�еĲ������ƴ��롣
			SIO_GET_EXTENSION_FUNCTION_POINTER,
			//
			// ָ�����뻺������ָ�롣
			&acceptex_guid,
			// 
			// ���뻺�����Ĵ�С�����ֽ�Ϊ��λ����
			sizeof(acceptex_guid),
			//
			// ָ�������������ָ�롣
			&g_pCtxtListenSocket->fnAcceptEx,
			// 
			// ����������Ĵ�С�����ֽ�Ϊ��λ����
			sizeof(g_pCtxtListenSocket->fnAcceptEx),
			//
			// ָ��ʵ������ֽ�����ָ�롣
			&bytes,
			//
			// ָ��WSAOVERLAPPED�ṹ��ָ�루���ڷ��ص��׽��֣��������ԣ���
			NULL,
			//
			// �������ʱ���õ��������ָ�루���ڷ��ص��׽��֣��������ԣ���
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

	// AcceptEx�������������ӣ����ر��غ�Զ�̵�ַ�������տͻ���Ӧ�ó����͵ĵ�һ�����ݿ顣
	nRet = g_pCtxtListenSocket->fnAcceptEx(
		// ��ʶ��ʹ��listen�������õ��׽��ֵ���������
		// ������Ӧ�ó���ȴ����Ӵ��׽��ֵĳ��ԡ�
		g_sdListen, 
		// ��ʶҪ���ܴ������ӵ��׽��ֵ����������ǰ󶨻�������
		g_pCtxtListenSocket->pIOContext->SocketAccept,
		// ָ�򻺳�����ָ�룬�û������������������Ϸ��͵ĵ�һ�����ݿ飬�������ı��ص�ַ�Լ��ͻ��˵�Զ�̵�ַ�� 
		// �������ݴ�ƫ���㿪ʼд�뻺�����ĵ�һ���֣�����ַд�뻺�����ĺ�벿�֡� 
		// ����ָ���˲�����
		(LPVOID)(g_pCtxtListenSocket->pIOContext->Buffer),
		// lpOutputBuffer�н����ڻ�������ͷ��ʵ�ʽ������ݵ��ֽ����� 
		// �˴�С��Ӧ�������������ص�ַ�Ĵ�С��Ҳ��Ӧ�����ͻ��˵�Զ�̵�ַ; ���Ǳ����ӵ������������ 
		// ���dwReceiveDataLengthΪ�㣬��������ӽ����ᵼ�½��ղ����� 
		// ��������һ�����AcceptEx�ͻ���ɣ�����ȴ��κ����ݡ�
		MAX_BUFF_SIZE - (2 * (sizeof(SOCKADDR_STORAGE) + 16)),
		// Ϊ���ص�ַ��Ϣ�������ֽ�����
		// ��ֵ�������ٱ�ʹ�õĴ���Э�������ַ���ȳ�16���ֽڡ�
		sizeof(SOCKADDR_STORAGE) + 16, 
		// ΪԶ�̵�ַ��Ϣ�������ֽ�����
		// ��ֵ�������ٱ�ʹ�õĴ���Э�������ַ���ȳ�16���ֽڡ�����Ϊ�㡣
		sizeof(SOCKADDR_STORAGE) + 16,
		// ָ��DWORD��ָ�룬��DWORD���ս��յ��ֽ����� 
		// ��������ͬ�����ʱ�����ô˲����� 
		// ���������ERROR_IO_PENDING�����Ժ���ɣ�����Զ�������ô˱������������ȡ�����֪ͨ���ƶ�ȡ���ֽ�����
		&dwRecvNumBytes,
		// OVERLAPPED�ṹ�����ڴ������󡣱���ָ���˲���;������ΪNULL��
		(LPOVERLAPPED) &(g_pCtxtListenSocket->pIOContext->Overlapped));

	if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError()))
	{
		myprintf(TEXT("AcceptEx() failed: %d\n"), WSAGetLastError());
		return (FALSE);
	}

	return (TRUE);
}

// �����߳�
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
		// ���Դ�ָ����I / O��ɶ˿ڳ���I / O������ݰ��� 
		// ���û���Ŷӵ�������ݰ����ú������ȴ�����ɶ˿ڹ����Ĵ�����I / O������ɡ� 
		// Ҫһ�γ��ж��I / O������ݰ�����ʹ��GetQueuedCompletionStatusEx������
		bSuccess = GetQueuedCompletionStatus(
			// 
			// ��ɶ˿ڵľ����
			// Ҫ������ɶ˿ڣ���ʹ��CreateIoCompletionPort������
			hIOCP,
			// 
			// ָ�������ָ�룬�ñ�����������ɵ�I / O�����ڼ䴫����ֽ�����
			&dwIoSize,
			//
			// ָ�������ָ�룬�ñ���������I / O��������ɵ��ļ������������ɼ�ֵ��
			// �����Կ���ڵ���CreateIoCompletionPortʱָ����ÿ�ļ���Կ��
			(PDWORD_PTR)&lpPerSocketContext,
			// 
			// ָ�������ָ�룬�ñ���������������ɵ�I/O����ʱָ����OVERLAPPED�ṹ�ĵ�ַ
			//
			// ��ʹ���ѽ��������ݸ�����ɶ˿ڹ������ļ��������Ч��OVERLAPPED�ӿڣ�Ӧ�ó���Ҳ������ֹ��ɶ˿�֪ͨ��
			// ����ͨ��λOVERLAPPED�ṹ��hEvent��Աָ����Ч���¼�������������λ����ɵġ�
			// �����˵�λ����Ч�¼����ʹI/O��ϲ����Ŷӵ���ɶ˿ڡ�
			(LPOVERLAPPED *)&lpOverlapped,
			// 
			// ������Ը��ȴ�������ݰ���������ɶ˿ڵĺ������� 
			// �����ָ��ʱ����δ������ɰ�����ú�����ʱ������FALSE������* lpOverlapped����ΪNULL��
			// 
			// ���dwMilliseconds��INFINITE����ú�����Զ���ᳬʱ��
			// ���dwMillisecondsΪ�㲢��û�г��ӵ�I / O��������ú�����������ʱ��
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
				// ��SO_CONDITIONAL_ACCEPT�׽���ѡ����Ϊ����Ӧ�ó�������Ƿ�Ҫ����һ�������׽��ִ�������ӡ�
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

			// ˢ����ɶ˿�
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
					// ָ��WSABUF�ṹ�����ָ�� ��
					// ÿ�� WSABUF�ṹ������һ��ָ�򻺳�����ָ��ͻ������ĳ��ȣ����ֽ�Ϊ��λ����
					// ����WinsockӦ�ó���һ�������� WSASend������ϵͳ��ӵ����Щ��������Ӧ�ó�������޷��������ǡ�
					// ����������ڷ��Ͳ����ڼ䱣����Ч��
					&lpAcceptSocketContext->pIOContext->wsabuf, 1, // lpBuffers�����е�WSABUF�ṹ�� ��
					// 
					// ָ�����ֽ�Ϊ��λ�����ֵ�ָ�룬���I/O����������ɣ����ᱻ���͡�
					// ���lpOverlapped������ΪNULL����Դ˲���ʹ��NULL�Ա�����ܵĴ�������
					// ����lpOverlapped������ΪNULLʱ���˲����ſ���ΪNULL��
					&dwSendNumBytes,
					// �����޸�WSASend����������Ϊ�ı�־��
					0,
					//
					// ָ��WSAOVERLAPPED�ṹ���ָ�롣
					// ���ڷ��ص��׽��֣������Դ˲�����
					&(lpAcceptSocketContext->pIOContext->Overlapped),
					// 
					// ���Ͳ������ʱ���õ�ָ��������̵�ָ�롣
					// ���ڷ��ص��׽��֣������Դ˲�����
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
				// ��һ��sendδ�ܷ�����������
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
				// ��һ��д�����Ѿ���ɣ�������һ������������
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

	// ���׽��ֹ��������е�IO��ɶ˿�
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
		// �����ٽ���
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
