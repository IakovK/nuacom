#pragma once

#define ENCRYPT_DATA 0
#define DECRYPT_DATA 1
#define SEND_STRING 2
struct RPCHeader
{
	DWORD dwTotalSize;
	DWORD dwNeededSize;
	DWORD dwUsedSize;
	DWORD dwCommand;
	DWORD dwDataSize;
	char data[1];
};

struct stringData
{
	DWORD_PTR callback;
	DWORD dwDataSize;
	char data[1];
};

