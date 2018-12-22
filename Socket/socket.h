#pragma once

#include <MSWSock.h>

#define DEFAULT_PORT		"5001"	// Ĭ�϶˿�
#define MAX_BUFF_SIZE		8192	// Ĭ��buffer - 8k
#define MAX_WORKER_THREAD	16		// ������߳� 16

typedef enum _IO_OPERATION {		// ��������ö��
	ClientIoAccept,
	ClientIoRead,
	ClientIoWrite
} IO_OPERATION, *PIO_OPERATION;

typedef struct _PER_IO_CONTEXT {					// IO������
	WSAOVERLAPPED			Overlapped;				// Overlapped�ṹ-�׵�ַ
	char					Buffer[MAX_BUFF_SIZE];	// ����
	WSABUF					wsabuf;					// wsabuf��
	int						nTotalBytes;			// ���ֽ���
	int						nSentBytes;				// �ѷ����ֽ���
	IO_OPERATION			IOOperation;			//
	SOCKET					SocketAccept;			// ?

	struct _PER_IO_CONTEXT	*pIOContextForward;	// ����
} PER_IO_CONTEXT, *PPER_IO_CONTEXT;

typedef struct _PER_SOCKET_CONTEXT {			// Socket������
	SOCKET						Socket;			// Socket
	LPFN_ACCEPTEX				fnAcceptEx;		// ����ָ��


	PPER_IO_CONTEXT				pIOContext;		// IO������
	struct _PER_SOCKET_CONTEXT	*pCtxtBack;		// ��ָ��
	struct _PER_SOCKET_CONTEXT	*pCtxtForward;	// ǰָ��
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