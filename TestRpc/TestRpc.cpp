#include <stdio.h>
#include <string>
#include "../RPCClient/RPCWrapper.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

#if !defined(_M_IA64) && !defined(_M_AMD64)
#include "..\common\Win32\NuacomCallback.h"
#endif
#if defined(_M_AMD64)
#include "..\common\x64\NuacomCallback.h"
#endif

#include "..\nuacomsrv\WebsocketAPI.h"

#include "sioclient\src\secure-socketio\internal\ssio_packet.h"
namespace ssio
{
	message::ptr from_json(rapidjson::Value const& value, std::vector<std::shared_ptr<const std::string>> const& buffers);
}

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

class Callbacks
{
public:
	void ProcessMessage(IMessageHolder *pmh);
	void ProcessObjectMessage(const std::map<std::string, IMessageHolder*> &msg);
	void ProcessCallMessage(const std::string &msgId, const std::map<std::string, IMessageHolder*> &msg);
	std::string localChannel;
	std::map<std::string, std::string> msgData;
};

void Callbacks::ProcessMessage(IMessageHolder *pmh)
{
	if (pmh->get_flag() != IMessageHolder::flag::flag_object)
		return;
	auto l = pmh->get_object_length();
	std::map<std::string, IMessageHolder*> data;
	for (int j = 0; j < l; j++)
	{
		char *ptr;
		int length;
		pmh->get_key(j, (void**)&ptr, &length);
		std::string s(ptr, length);
		auto obj = pmh->get_data(j);
		data[s] = obj;
	}
	ProcessObjectMessage(data);
}

void Callbacks::ProcessObjectMessage(const std::map<std::string, IMessageHolder*> &msg)
{
	for (auto &v : msg)
	{
		auto msgId = v.first;
		if (v.second->get_flag() != IMessageHolder::flag::flag_object)
			continue;
		auto pmh = v.second;
		auto l = pmh->get_object_length();
		std::map<std::string, IMessageHolder*> data;
		for (int j = 0; j < l; j++)
		{
			char *ptr;
			int length;
			pmh->get_key(j, (void**)&ptr, &length);
			std::string s(ptr, length);
			auto obj = pmh->get_data(j);
			data[s] = obj;
		}
		ProcessCallMessage(msgId, data);
	}
}

void Callbacks::ProcessCallMessage(const std::string &msgId, const std::map<std::string, IMessageHolder*> &msg)
{
	{
		msgData.clear();
		for (auto &v : msg)
		{
			auto f = v.second->get_flag();
			if (f == IMessageHolder::flag::flag_string)
			{
				auto pmh = v.second;
				char *ptr;
				int length;
				pmh->get_string((void**)&ptr, &length);
				std::string s(ptr, length);
				msgData[v.first] = s;
			}
		}
	}

	try
	{
		auto channel = msgData["local_channel"];
		if (channel.size() > 4)
		{
			if (channel.substr(0, 4) == "SIP/")
				localChannel = channel.substr(4);
		}
	}
	catch (const std::exception&)
	{
	}
	printf("Callbacks::ProcessCallMessage: localChannel = %s\n", localChannel.c_str());
}

void SendString(
	/* [in] */ handle_t Binding,
	/* [in] */ PCALLBACK_HANDLE_TYPE hCallback,
	/* [string][in] */ unsigned char *msg)
{
	Callbacks *callbacks = (Callbacks*)hCallback;
	rapidjson::Document doc;
	doc.Parse<0>((char*)msg);
	ssio::message::ptr m = ssio::from_json(doc, std::vector<std::shared_ptr<const std::string> >());
	MessageHolder mh(m);
	callbacks->ProcessMessage(&mh);
}

RPC_STATUS InitRpc(unsigned char *pszEndpoint)
{
	RPC_STATUS status;
	unsigned char * pszProtocolSequence = (unsigned char *)"ncalrpc";

	status = RpcServerUseProtseqEpA(pszProtocolSequence, RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
		pszEndpoint, NULL);
	if (status)
	{
		return status;
	}

	status = RpcServerRegisterIf(NuacomCallback_v1_0_s_ifspec, NULL, NULL);
	if (status)
	{
		return status;
	}

	status = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, TRUE);
	return status;
}

int main()
{
	unsigned char * pszProtocolSequence = (unsigned char *)"ncalrpc";
	unsigned char * pszEndpoint = (unsigned char *)"NuacomService";
	unsigned char * pszCallbackEndpoint = (unsigned char *)"NuacomServiceCallback";
	printf("TestRpc: calling InitRpc\n");
	RPC_STATUS status = InitRpc(pszCallbackEndpoint);
	if (status != RPC_S_OK)
	{
		printf("TestRpc: InitRpc failed. status = %d\n", status);
		return 0;
	}
	CRPCWrapper rpcw;
	Callbacks cb;
	printf("TestRpc: calling rpcw.Init\n");
	status = rpcw.Init(pszProtocolSequence, pszEndpoint);
	if (status != RPC_S_OK)
	{
		printf("TestRpc: rpcw.Init failed. status = %d\n", status);
		return 0;
	}
	printf("TestRpc: calling rpcw.ConnectToServer. &cb = %p\n", &cb);
	status = rpcw.ConnectToServer(pszCallbackEndpoint, &cb);
	if (status != RPC_S_OK)
	{
		printf("TestRpc: rpcw.ConnectToServer failed. status = %d(%08x)\n", status, status);
		return 0;
	}
	Sleep(1000);
	char statusMessage[255];
	BOOL bSuccess;
	printf("TestRpc: calling rpcw.MakeCall\n");
	status = rpcw.MakeCall((char*)"0079160858919", statusMessage, 255, &bSuccess);
	if (status != RPC_S_OK)
	{
		printf("TestRpc: rpcw.MakeCall failed. status = %d\n", status);
		return 0;
	}
	printf("TestRpc: Calling Hangup. Presss Enter to continue\n");
	gets_s(statusMessage);
	status = rpcw.Hangup((char*)cb.localChannel.c_str());
	if (status != RPC_S_OK)
	{
		printf("TestRpc: rpcw.Hangup failed. status = %d", status);
		return 0;
	}
	printf("TestRpc: Calling Disconnect. Presss Enter to continue\n");
	gets_s(statusMessage);
	status = rpcw.Disconnect();
	if (status != RPC_S_OK)
	{
		printf("TestRpc: rpcw.Disconnectfailed. status = %d", status);
		return 0;
	}
	printf("TestRpc: Stopping. Presss Enter to continue\n");
	gets_s(statusMessage);
	printf("TestRpc: calling RpcMgmtStopServerListening\n");
	status = RpcMgmtStopServerListening(0);
	printf("TestRpc: RpcMgmtStopServerListening returned: status = %d\n", status);
	printf("TestRpc: calling RpcMgmtWaitServerListen\n");
	status = RpcMgmtWaitServerListen();
	printf("TestRpc: RpcMgmtWaitServerListen returned: status = %d\n", status);
}

void __RPC_USER PCONTEXT_HANDLE_TYPE_rundown(PCONTEXT_HANDLE_TYPE hConn)
{
	printf("PCONTEXT_HANDLE_TYPE_rundown: hConn = %p\n", hConn);
}
