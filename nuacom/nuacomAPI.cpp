#include "stdafx.h"
#include "nuacom.h"
#include "nuacomAPI.h"
#include "md5.h"

#include <cvt/wstring>
#include <codecvt>

#pragma comment(lib, "libcurl.dll.a")
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <curl/curl.h>

size_t write_data(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	try
	{
		std::string *result = (std::string *)userdata;
		result->append(ptr, nmemb);
		return nmemb;
	}
	catch (const std::exception&)
	{
		return 0;
	}
}

bool MakeCall(const std::string &session_token, const std::string &src_number, const std::wstring &dst_number, std::string &status_message)
{
	stdext::cvt::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
	std::string dst_string = myconv.to_bytes(dst_number);
	// create json
	rapidjson::Document d;
	rapidjson::Document::AllocatorType& a = d.GetAllocator();
	d.SetObject();
	rapidjson::Value v;
	v.SetString("call");
	d.AddMember("action", v, a);
	v.SetString(src_number.c_str(), a);
	d.AddMember("call_from_number", v, a);
	v.SetString(dst_string.c_str(), a);
	d.AddMember("call_to_number", v, a);
	rapidjson::StringBuffer sb;
	rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
	d.Accept(writer);
	std::string payload = sb.GetString();

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
	DBGOUT((3, "MakeCall: curl_easy_perform returned: %d\n", ret));
	curl_easy_cleanup(hnd);
	if (ret == CURLE_OK)
	{
		DBGOUT((3, "MakeCall: result is: %s\n", result.c_str()));
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

bool GetSessionToken(const std::string &name, const std::string &pass, std::string &session_token)
{
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
}

