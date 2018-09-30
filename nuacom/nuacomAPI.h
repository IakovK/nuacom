#pragma once
bool MakeCall(const std::string &session_token, const std::string &src_number, const std::wstring &dst_number, std::string &status_message);
bool GetSessionToken(const std::string &name, const std::string &pass, std::string &session_token);
bool EndCall(const std::string &session_token, const std::string &channel_id, std::string &status_message);
