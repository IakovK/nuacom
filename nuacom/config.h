#pragma once
void GetConfig();
struct LineInfo
{
	std::string extension;
	std::string userName;
	std::string password;
};

using linesCollection = std::map<int, LineInfo>;
linesCollection GetLinesInfo();
