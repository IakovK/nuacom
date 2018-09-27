#include <Windows.h>
#include <stdio.h>
#include <string>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <curl/curl.h>
#include "md5.h"

size_t write_data(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	std::string *result = (std::string *)userdata;
	result->append(ptr, nmemb);
	return nmemb;
}

#ifdef CPPREST
pplx::task<std::tuple<bool, string_t>> GetSessionTokenAsync(const std::string &name, const std::string &pass)
{
	std::string hash = md5(pass.c_str());
	string_t baseurl = U("https://api.nuacom.ie/login?");
	string_t query;
	query += U("email=");
	query += conversions::to_string_t(name);
	query += U("&pass=");
	query += conversions::to_string_t(hash);
	string_t fullurl = baseurl + query;
	http_request req(methods::GET);
	http_client client(fullurl);

	return client.request(req).then([=](http_response response)
	{
		return response.extract_json(true);
	}).then([=](json::value jsv)
	{
		if (jsv.has_field(U("session_token")))
		{
			return std::tuple<bool, string_t>(true, jsv[U("session_token")].as_string());
		}
		else
		{
			return std::tuple<bool, string_t>(false, jsv.serialize());
		}
	}).then([=](concurrency::task<std::tuple<bool, string_t>> previous)
	{
		try
		{
			return previous.get();
		}
		catch (const std::exception& ex)
		{
			return std::tuple<bool, string_t>(false, conversions::to_string_t(ex.what()));
		}
	});
}
#endif

bool GetSessionToken(const std::string &name, const std::string &pass, std::string &session_token)
{
#ifdef CPPREST
	auto r = GetSessionTokenAsync(name, pass).get();
	session_token = conversions::to_utf8string(std::get<1>(r));
	return std::get<0>(r);
#else
	std::string hash = md5(pass.c_str());
	std::string baseurl = "https://api.nuacom.ie/login?";
	std::string query;
	query += "email=";
	query += name;
	query += "&pass=";
	query += hash;
	std::string fullurl = baseurl + query;
	// send request
	CURL *hnd = curl_easy_init();

	curl_easy_setopt(hnd, CURLOPT_URL, fullurl.c_str());

	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, "Cache-Control: no-cache");
	headers = curl_slist_append(headers, "Content-Type: application/json");
	curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);

	curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, write_data);
	std::string result;
	curl_easy_setopt(hnd, CURLOPT_WRITEDATA, &result);
	curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYPEER, 0);
	curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYHOST, 0);
	char errMsgBuf[CURL_ERROR_SIZE];
	errMsgBuf[0] = 0;
	curl_easy_setopt(hnd, CURLOPT_ERRORBUFFER, errMsgBuf);

	CURLcode ret = curl_easy_perform(hnd);
	printf("GetSessionToken: curl_easy_perform returned: %d\n", ret);
	curl_easy_cleanup(hnd);
	if (ret == CURLE_OK)
	{
		rapidjson::Document d;
		rapidjson::Document::AllocatorType& a = d.GetAllocator();
		d.SetObject();
		d.Parse(result.c_str());
		if (d.HasMember("session_token"))
		{
			if (d["session_token"].IsString())
			{
				session_token = d["session_token"].GetString();
				return true;
			}
			else
			{
				session_token = result;
				return false;
			}
		}
		else
		{
			session_token = errMsgBuf;
			return false;
		}
	}
	else
	{
		session_token = result;
		return false;
	}
#endif
}

bool MakeCall(const std::string &session_token, const std::string &src_number, const std::string &dst_number, std::string &status_message)
{
	// create json
	rapidjson::Document d;
	rapidjson::Document::AllocatorType& a = d.GetAllocator();
	d.SetObject();
	rapidjson::Value v;
	v.SetString("call");
	d.AddMember("action", v, a);
	v.SetString(src_number.c_str(), a);
	d.AddMember("call_from_number", v, a);
	v.SetString(dst_number.c_str(), a);
	d.AddMember("call_to_number", v, a);
	rapidjson::StringBuffer sb;
	rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
	d.Accept(writer);
	std::string payload = sb.GetString();
	printf("MakeCall: payload = %s\n", payload.c_str());

	// send request
	CURL *hnd = curl_easy_init();

	curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "POST");
	curl_easy_setopt(hnd, CURLOPT_URL, "https://api.nuacom.ie/v1/call_back");

	struct curl_slist *headers = NULL;
	std::string securityHeader = "Client-Security-Token: " + session_token;
	headers = curl_slist_append(headers, "Cache-Control: no-cache");
	headers = curl_slist_append(headers, "Content-Type: application/json");
	headers = curl_slist_append(headers, securityHeader.c_str());
	curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);

	curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, payload.c_str());
	curl_easy_setopt(hnd, CURLOPT_POSTFIELDSIZE, payload.size());
	curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, write_data);
	std::string result;
	curl_easy_setopt(hnd, CURLOPT_WRITEDATA, &result);
	curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYPEER, 0);
	curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYHOST, 0);
	char errMsgBuf[CURL_ERROR_SIZE];
	errMsgBuf[0] = 0;
	curl_easy_setopt(hnd, CURLOPT_ERRORBUFFER, errMsgBuf);

	CURLcode ret = curl_easy_perform(hnd);
	printf("MakeCall: curl_easy_perform returned: %d\n", ret);
	curl_easy_cleanup(hnd);
	if (ret == CURLE_OK)
	{
		printf("MakeCall: result is: %s\n", result.c_str());
		rapidjson::Document d;
		rapidjson::Document::AllocatorType& a = d.GetAllocator();
		d.SetObject();
		d.Parse(result.c_str());
		if (d.HasMember("status"))
		{
			if (d["status"].IsString())
			{
				status_message = d["status"].GetString();
				return true;
			}
			else
			{
				status_message = result;
				return false;
			}
		}
		else
		{
			status_message = errMsgBuf;
			return false;
		}
	}
	else
	{
		status_message = result;
		return false;
	}
}

void TestLogin(int ac, char *av[])
{
	if (ac < 4)
	{
		printf("usage: apitest.exe login email password\n");
		return;
	}
	std::string name = av[2];
	std::string pass = av[3];
	std::string session_token;
	printf("TestLogin: name = %s, pass = %s\n", name.c_str(), pass.c_str());
	bool b = GetSessionToken(name, pass, session_token);
	if (b)
	{
		printf("TestLogin: GetSessionToken returned %s\n", session_token.c_str());
	}
	else
	{
		printf("TestLogin: GetSessionToken returned false: %s\n", session_token.c_str());
	}
}

void TestLogout(int ac, char *av[])
{
	printf("TestLogout\n");
#if 0
	if (ac < 3)
	{
		printf("usage: apitest.exe login <file name>\n");
		return;
	}
#endif
}

void TestDial(int ac, char *av[])
{
	if (ac < 5)
	{
		printf("usage: apitest.exe dial session_token src_number dst_number\n");
		return;
	}
	std::string session_token = av[2];
	std::string src_number = av[3];
	std::string dst_number = av[4];
	std::string status_message;
	//apitest.exe dial 993fef08e1bdff3252e898b5bc4c7aaa 10 353876543219
	printf("TestDial: session_token = %s, src_number = %s, dst_number = %s\n", session_token.c_str(), src_number.c_str(), dst_number.c_str());
	bool b = MakeCall(session_token, src_number, dst_number, status_message);
	if (b)
	{
		printf("TestDial: MakeCall returned %s\n", status_message.c_str());
	}
	else
	{
		printf("TestDial: MakeCall returned false: %s\n", status_message.c_str());
	}
}

int main(int ac, char *av[])
{
	if (ac < 2)
	{
		printf("usage: apitest.exe <command>\n");
		printf("commands: login, logout, dial\n");
		return 0;
	}
	std::string command = av[1];
	if (command == "login")
		TestLogin(ac, av);
	else if (command == "logout")
		TestLogout(ac, av);
	else if (command == "dial")
		TestDial(ac, av);
	else
	{
		printf("invalid command\n");
		printf("commands: login, logout, dial\n");
	}
	return 0;
}
