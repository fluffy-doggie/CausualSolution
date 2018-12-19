#pragma once

#include <MSWSock.h>

#define DEFAULT_PORT		"5001"	// 默认端口
#define MAX_BUFF_SIZE		8192	// 默认buffer - 8k
#define MAX_WORKER_THREAD	16		// 最大工作线程 16

typedef enum _IO_OPERATION {
	ClientIoAccept,
	ClientIoRead,
	ClientIoWrite
} IO_OPERATION, *PIO_OPERATION;

typedef struct _PER_IO_CONTEXT {
	WSAOVERLAPPED			Overlapped;
	char					Buffer[MAX_BUFF_SIZE];
	WSABUF					wsabuf;
	int						nTotalBytes;
	int						nSentBytes;
	IO_OPERATION			IOOperation;
	SOCKET					SocketAccept;

	struct _PER_IO_CONTEXT	*pIOContextForward;	// 链表
} PER_IO_CONTEXT, *PPER_IO_CONTEXT;

typedef struct _PER_SOCKET_CONTEXT {
	SOCKET						Socket;
	LPFN_ACCEPTEX				fnAcceptEx;


	PPER_IO_CONTEXT				pIOContext;
	struct _PER_SOCKET_CONTEXT	*pCtxtBack;
	struct _PER_SOCKET_CONTEXT	*pCtxtForward;	// 双向链表
} PER_SOCKET_CONTEXT, *PPER_SOCKET_CONTEXT;

BOOL ValidOptions(int argc, char *argv[]);

BOOL WINAPI CtrlHandler(
	DWORD dwEvent
);

BOOL CreateListenSocket(void);

BOOL CreateAcceptSocket(
	BOOL fUpdateIOCP
);

DWORD WINAPI WorkerThread(
	LPVOID WorkContext
);

PPER_SOCKET_CONTEXT UpdateCompletionPort(
	SOCKET s,
	IO_OPERATION ClientIo,
	BOOL bAddToList
);

VOID CloseClient(
	PPER_SOCKET_CONTEXT lpPerSocketContext,
	BOOL BGraceful
);

PPER_SOCKET_CONTEXT CtxtAllocate(
	SOCKET s,
	IO_OPERATION ClientIO
);

VOID CtxtListFree();

VOID CtxtListAddTo(
	PPER_SOCKET_CONTEXT lpPerSocketContext
);

VOID CtxtListDeleteFrom(
	PPER_SOCKET_CONTEXT lpPerSocketContext
);