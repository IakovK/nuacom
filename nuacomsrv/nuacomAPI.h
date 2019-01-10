#pragma once
namespace nuacom
{
	bool MakeCall(const std::string &session_token, const std::string &src_number, const std::string &dst_number, std::string &status_message);
	bool GetSessionToken(const std::string &name, const std::string &pass, std::string &session_token);
	bool EndCall(const std::string &session_token, const std::string &channel_id, std::string &status_message);
	bool GetExtension(const std::string &session_token, std::string &Extension);
}
