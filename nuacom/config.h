#pragma once
#if DBG
void GetDebugLevel();
#endif
struct LineInfo
{
	std::string extension;
	std::string userName;
	std::string password;
};

using linesCollection = std::map<int, LineInfo>;
linesCollection GetLinesInfo();
