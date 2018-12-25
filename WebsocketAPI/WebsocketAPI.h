#pragma once
#include "header.h"

class IMessageHolder
{
public:
	enum flag
	{
		flag_integer,
		flag_double,
		flag_string,
		flag_binary,
		flag_array,
		flag_object,
		flag_boolean,
		flag_null
	};
	virtual flag get_flag() const = 0;
	virtual int64_t get_int() const = 0;
	virtual double get_double() const = 0;
	virtual void get_string(void **ptr, int *length) = 0;
	virtual void get_binary(void **ptr, int *length) = 0;
	virtual int get_array_length() const = 0;
	virtual IMessageHolder *get_array_elt(int index) const = 0;
	virtual int get_object_length() const = 0;
	virtual void get_key(int index, void **ptr, int *length) const = 0;
	virtual IMessageHolder *get_data(int index) const = 0;
	virtual bool get_bool() const = 0;
};

class ICallbacks
{
public:
	virtual void OnOpenListener() = 0;
	virtual void OnFailListener() = 0;
	virtual void ProcessMessage(IMessageHolder *pmh) = 0;
};

extern "C" void WSAPI ConnectToWebsocket(const char *securityToken, ICallbacks *callbacks, void **handle);
extern "C" void WSAPI CloseWebsocket(void *handle);
extern "C" void WSAPI CloseWebsocketAsync(void *handle);
