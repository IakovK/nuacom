#include <iostream>
#include <cpprest/ws_client.h>

using namespace std;
using namespace web;
using namespace web::websockets::client;
using namespace utility;

pplx::task<void> ConnectToWebSocketAsync(const std::string &session_token)
{
	std::string url = "wss://call-events.nuacom.ie/extension_calls?client-security-token=" + session_token;
	//std::string url = "ws://atlas-suse/socket.io/?client-security-token=" + session_token;
	//std::string url = "wss://echo.websocket.org/";
	websocket_client_config config;
	const auto &name = utf8string("call-events.nuacom.ie");
	config.set_validate_certificates(false);
	websocket_client client(config);

	printf("ConnectToWebSocketAsync: calling client.connect. url = %s\n", url.c_str());
	try
	{
		client.connect(uri(conversions::to_string_t(url))).then([=](pplx::task<void> previous)
		{
			try
			{
				previous.get();
				printf("ConnectToWebSocketAsync: client.connect succeeded\n");
			}
			catch (const websocket_exception& ex)
			{
				printf("ConnectToWebSocketAsync: client.connect failed: %s\n", ex.what());
			}
			catch (const std::exception& ex)
			{
				printf("ConnectToWebSocketAsync: client.connect failed: %s\n", ex.what());
			}
		}).wait();
	}
	catch (const std::exception& ex)
	{
		printf("ConnectToWebSocketAsync: exception in client.connect: %s\n", ex.what());
	}
	printf("ConnectToWebSocketAsync: after client.connect\n");

	return pplx::task<void>([] {});
#if 0
	websocket_outgoing_message out_msg;
	out_msg.set_utf8_message("test");
	client.send(out_msg).wait();

	client.receive().then([](websocket_incoming_message in_msg) {
		return in_msg.extract_string();
	}).then([](string body) {
		cout << body << endl; // test
	}).wait();

	client.close().wait();

	return 0;
#endif
}

void ConnectToWebSocket(const std::string &session_token)
{
	ConnectToWebSocketAsync(session_token).wait();
}

void TestConnect(int ac, char *av[])
{
	if (ac < 3)
	{
		printf("usage: wstest.exe connect session_token\n");
		return;
	}
	std::string session_token = av[2];
	ConnectToWebSocket(session_token);
}

int main(int ac, char *av[])
{
	if (ac < 2)
	{
		printf("usage: wstest.exe <command>\n");
		printf("commands: connect\n");
		return 0;
	}
	std::string command = av[1];
	if (command == "connect")
		TestConnect(ac, av);
	else
	{
		printf("invalid command\n");
		printf("commands: connect\n");
	}
	return 0;
}
