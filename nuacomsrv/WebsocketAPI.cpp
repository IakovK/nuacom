// WebsocketAPI.cpp : Defines the exported functions for the DLL application.
//

#include "header.h"

#include "sioclient\src\secure-socketio\ssio_client.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <process.h>
#include "WebsocketAPI.h"

#if 0
void PrintMessage(const ssio::message &m);

std::string ToString(ssio::message::flag flag)
{
#define CASE_STRING(x) case x: return #x;
	switch (flag)
	{
		CASE_STRING(ssio::message::flag_integer)
			CASE_STRING(ssio::message::flag_double)
			CASE_STRING(ssio::message::flag_string)
			CASE_STRING(ssio::message::flag_binary)
			CASE_STRING(ssio::message::flag_array)
			CASE_STRING(ssio::message::flag_object)
			CASE_STRING(ssio::message::flag_boolean)
			CASE_STRING(ssio::message::flag_null)
	default:
		{
			std::stringstream s;
			s << "unknown: " << (int)flag;
			return s.str();
		}
	}
}

void PrintIntMessage(const ssio::int_message &m)
{
	std::cout << m.get_int() << "\n";
}

void PrintDoubleMessage(const ssio::double_message &m)
{
	std::cout << m.get_double() << "\n";
}

void PrintStringMessage(const ssio::string_message &m)
{
	std::cout << m.get_string() << "\n";
}

void PrintBinaryMessage(const ssio::binary_message &m)
{
	std::cout << "binary: size = " << m.get_binary()->size() << "\n";
}

void PrintArrayMessage(const ssio::array_message &m)
{
	std::cout << "array: size = " << m.size() << "\n";
}

void PrintObjectMessage(const ssio::object_message &m)
{
	auto &values = m.get_map();
	for (auto &v : values)
	{
		std::cout << v.first << ": ";
		PrintMessage(*v.second);
	}
}

void PrintBooleanMessage(const ssio::bool_message &m)
{
	std::cout << m.get_bool() << "\n";
}

void PrintNullMessage(const ssio::null_message &m)
{
	std::cout << "null message" << "\n";
}

void PrintUnknownMessage(const ssio::message &m)
{
	std::cout << "unknown message" << "\n";
}

void PrintMessage(const ssio::message &m)
{
	auto f = m.get_flag();
	switch (f)
	{
	case ssio::message::flag_integer:
		PrintIntMessage(static_cast<const ssio::int_message&>(m));
		break;
	case ssio::message::flag_double:
		PrintDoubleMessage(static_cast<const ssio::double_message&>(m));
		break;
	case ssio::message::flag_string:
		PrintStringMessage(static_cast<const ssio::string_message&>(m));
		break;
	case ssio::message::flag_binary:
		PrintBinaryMessage(static_cast<const ssio::binary_message&>(m));
		break;
	case ssio::message::flag_array:
		PrintArrayMessage(static_cast<const ssio::array_message&>(m));
		break;
	case ssio::message::flag_object:
		PrintObjectMessage(static_cast<const ssio::object_message&>(m));
		break;
	case ssio::message::flag_boolean:
		PrintBooleanMessage(static_cast<const ssio::bool_message&>(m));
		break;
	default:
		break;
	}
}
#endif

struct context
{
	ssio::client h;
	std::thread dingThread;
	std::thread reconnectThread;
	bool terminating;
};

class MessageHolder : public IMessageHolder
{
	ssio::message::ptr _m;
	mutable std::vector<MessageHolder> children;
	mutable std::vector<std::string> keys;
	mutable std::vector<MessageHolder> values;
	mutable bool keys_retrieved;
public:
	MessageHolder(ssio::message::ptr m)
		:_m(m)
		, keys_retrieved(false)
	{
	}
	virtual flag get_flag() const
	{
		return (flag)(int)_m->get_flag();
	}
	virtual int64_t get_int() const
	{
		return _m->get_int();
	}
	virtual double get_double() const
	{
		return _m->get_double();
	}
	virtual void get_string(void **ptr, int *length)
	{
		auto &s = _m->get_string();
		*((const char**)ptr) = s.data();
		*length = s.length();
	}
	virtual void get_binary(void **ptr, int *length)
	{
		auto &s = _m->get_binary();
		*((const char**)ptr) = s->data();
		*length = s->length();
	}
	virtual int get_array_length() const
	{
		auto v = _m->get_vector();
		return v.size();
	}
	virtual IMessageHolder *get_array_elt(int index) const
	{
		auto v = _m->get_vector();
		MessageHolder mh(v[index]);
		children.push_back(mh);
		auto &v1 = *children.rbegin();
		return &v1;
	}
	virtual int get_object_length() const
	{
		auto v = _m->get_map();
		return v.size();
	}
	virtual void get_key(int index, void **ptr, int *length) const
	{
		if (!keys_retrieved)
		{
			auto v = _m->get_map();
			for (auto &e : v)
			{
				keys.push_back(e.first);
				values.push_back(MessageHolder(e.second));
			}
		}
		keys_retrieved = true;
		auto &s = keys[index];
		*((const char**)ptr) = s.data();
		*length = s.length();
	}
	IMessageHolder *get_data(int index) const
	{
		if (!keys_retrieved)
		{
			auto v = _m->get_map();
			for (auto &e : v)
			{
				keys.push_back(e.first);
				values.push_back(MessageHolder(e.second));
			}
		}
		keys_retrieved = true;
		auto &h = values[index];
		return &h;
	}
	virtual bool get_bool() const
	{
		return _m->get_bool();
	}
	virtual ssio::message::ptr get_message()
	{
		return _m;
	}
};

extern "C" void WSAPI ConnectToWebsocket(const char *securityToken, ICallbacks *callbacks, void **handle)
{
	OutputDebugString("ConnectToWebsocket: entry\n");
	context *c = new context();
	c->terminating = false;

	std::map<std::string, std::string> query;
	query["client-security-token"] = securityToken;
	c->h.set_open_listener([=]()
	{
		callbacks->OnOpenListener();
	});

	c->h.set_fail_listener([=]()
	{
		callbacks->OnFailListener();
	});

	c->h.set_close_listener([=](ssio::client::close_reason const& reason)
	{
		OutputDebugString("ConnectToWebsocket: close_listener callback\n");
		if (c->reconnectThread.joinable())
			c->reconnectThread.join();
		c->reconnectThread = std::thread([=]()
		{
			if (!c->terminating)
			{
				c->h.connect("wss://call-events.nuacom.ie/extension_calls", query);
			}
		});
	});

	c->h.socket("extension_calls")->on("room_Calls", [=](ssio::event& ev)
	{
		auto &ml = ev.get_messages();
		for (int j = 0; j < ml.size(); j++)
		{
			auto &m = ml[j];
			MessageHolder mh(m);
			callbacks->ProcessMessage(&mh);
			//PrintMessage(*m);
		}
	});
	OutputDebugString("ConnectToWebsocket: calling c->h.connect\n");
	c->h.connect("wss://call-events.nuacom.ie/extension_calls", query);
	OutputDebugString("ConnectToWebsocket: calling c->h.connect done\n");
	c->dingThread = std::thread([=]()
	{
		OutputDebugString("ConnectToWebsocket: c->dingThread entry\n");
		while (!c->terminating)
		{
			using namespace std::chrono_literals;
			std::this_thread::sleep_for(6s);
			c->h.socket("extension_calls")->emit("ding", { "{hello: \"server\"}" });
		}
		OutputDebugString("ConnectToWebsocket: c->dingThread exit\n");
	});
	*handle = c;
	OutputDebugString("ConnectToWebsocket: exit\n");
}

extern "C" void WSAPI CloseWebsocket(void *handle)
{
	context *c = (context*)handle;
	c->terminating = true;
	OutputDebugString("CloseWebsocket: waiting for dingThread\n");
	if (c->dingThread.joinable())
		c->dingThread.join();
	if (c->reconnectThread.joinable())
		c->reconnectThread.join();
	OutputDebugString("CloseWebsocket: exit\n");
	delete c;
}

extern "C" void WSAPI CloseWebsocketAsync(void *handle)
{
	_beginthread(CloseWebsocket, 0, handle);
}

