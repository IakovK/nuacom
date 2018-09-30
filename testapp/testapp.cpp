#include <Windows.h>
#include <stdio.h>
#include <tapi.h>
#include <intrin.h>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#pragma comment(lib, "tapi32")

void TestInstall(int ac, char *av[])
{
	printf("TestInstall\n");
	if (ac < 3)
	{
		printf("usage: testapp.exe install <file name>\n");
		return;
	}
	std::string fileName = av[2];
	printf("installing %s\n", fileName.c_str());
	DWORD providerId = 0;
	printf("calling lineAddProvider\n");
	LONG retVal = lineAddProvider("c:\\windows\\system32\\nuacom.tsp", NULL, &providerId);
	printf("lineAddProvider returned: retVal = %08x, providerId = %d\n", retVal, providerId);
}

void TestUninstall(int ac, char *av[])
{
	printf("TestUninstall\n");
	if (ac < 3)
	{
		printf("usage: testapp.exe uninstall <provider id>\n");
		return;
	}
	std::string idStr = av[2];
	std::stringstream s(idStr);
	int providerId;
	s >> providerId;
	printf("uninstalling %d\n", providerId);
	printf("calling lineRemoveProvider\n");
	LONG retVal = lineRemoveProvider(providerId, NULL);
	printf("lineRemoveProvider returned: retVal = %08x\n", retVal);
}

class DialTester
{
	bool m_bInited;
	HLINEAPP m_appHandle;
	DWORD m_numDevs;
	LINEINITIALIZEEXPARAMS m_params;
public:
	DialTester()
		:m_bInited(false)
		, m_params{ 0 }
		, m_numDevs{ 0 }
	{
		DWORD apiVersion = TAPI_CURRENT_VERSION;
		m_params.dwTotalSize = sizeof(LINEINITIALIZEEXPARAMS);
		m_params.dwOptions = LINEINITIALIZEEXOPTION_USEEVENT;
		auto n = lineInitializeEx(&m_appHandle, NULL, NULL, "nuacom test app", &m_numDevs, &apiVersion, &m_params);
		if (n == 0)
		{
			m_bInited = true;
		}
	}
	~DialTester()
	{
		if (m_bInited)
		{
			auto n = lineShutdown(m_appHandle);
		}
	}
	struct provDesc
	{
		DWORD providerId;
		std::string providerName;
	};
	struct lineDesc
	{
		DWORD lineId;
		DWORD dwAPIVersion;
		std::string lineName;
	};
	using ProvidersList = std::vector<provDesc>;
	using LinesList = std::vector<lineDesc>;
	ProvidersList GetProvidersList()
	{
		ProvidersList retVal;
		LINEPROVIDERLIST lpl;
		lpl.dwTotalSize = sizeof lpl;
		auto n = lineGetProviderList(TAPI_CURRENT_VERSION, &lpl);
		if (n != 0)
		{
			printf("GetProvidersList: lineGetProviderList(1) failed: %08x\n", n);
			return retVal;
		}
		std::vector<unsigned char> v(lpl.dwNeededSize);
		LPLINEPROVIDERLIST lpl2 = (LPLINEPROVIDERLIST)v.data();
		lpl2->dwTotalSize = lpl.dwNeededSize;
		n = lineGetProviderList(TAPI_CURRENT_VERSION, lpl2);
		if (n != 0)
		{
			printf("GetProvidersList: lineGetProviderList(2) failed: %08x\n", n);
			return retVal;
		}
		LPLINEPROVIDERENTRY lpeptr = (LPLINEPROVIDERENTRY)(v.data() + lpl2->dwProviderListOffset);
		for (int j = 0; j < lpl2->dwNumProviders; j++)
		{
			provDesc pd;
			pd.providerId = lpeptr[j].dwPermanentProviderID;
			pd.providerName = std::string((char*)(v.data() + lpeptr[j].dwProviderFilenameOffset), lpeptr[j].dwProviderFilenameSize);
			retVal.push_back(pd);
		}
		return retVal;
	}
	bool GetLineInfo(int lineNum, lineDesc &ld)
	{
		LINEEXTENSIONID extId;
		auto n = lineNegotiateAPIVersion(m_appHandle, lineNum,
			TAPI_CURRENT_VERSION, TAPI_CURRENT_VERSION,
			&(ld.dwAPIVersion), &extId);
		if (n != 0)
		{
			printf("GetLineInfo: lineNum = %d, lineNegotiateAPIVersion failed: %08x\n", lineNum, n);
			return false;
		}
		LINEDEVCAPS ldc{ 0 };
		ldc.dwTotalSize = sizeof(LINEDEVCAPS);
		n = lineGetDevCaps(m_appHandle, lineNum, ld.dwAPIVersion, 0, &ldc);
		if (n != 0)
		{
			printf("GetLineInfo: lineNum = %d, lineGetDevCaps(1) failed: %08x\n", lineNum, n);
			return false;
		}
		std::vector<unsigned char> v(ldc.dwNeededSize);
		LPLINEDEVCAPS ldc2 = (LPLINEDEVCAPS)v.data();
		ldc2->dwTotalSize = ldc.dwNeededSize;
		n = lineGetDevCaps(m_appHandle, lineNum, ld.dwAPIVersion, 0, ldc2);
		if (n != 0)
		{
			printf("GetLineInfo: lineNum = %d, lineGetDevCaps(2) failed: %08x\n", lineNum, n);
			return false;
		}
		std::string lineName((char*)v.data() + ldc2->dwLineNameOffset, ldc2->dwLineNameSize);
		ld.lineId = lineNum;
		ld.lineName = lineName;
		return true;
	}
	LinesList GetLinesList()
	{
		LinesList retVal;
		for (int j = 0; j < m_numDevs; j++)
		{
			lineDesc ld;
			if (GetLineInfo(j, ld))
				retVal.push_back(ld);
		}
		return retVal;
	}
	void dial(int deviceId, const char *number)
	{
		printf("dial: deviceId = %d, number = %s\n", deviceId, number);
		LinesList l = GetLinesList();
		auto it = std::find_if(std::begin(l), std::end(l), [=](const lineDesc& ld)
		{
			return ld.lineId == deviceId;
		});
		if (it == std::end(l))
		{
			printf("dial: linedesc not found\n");
			return;
		}
		HLINE lineHandle;
		auto n = lineOpen(m_appHandle, deviceId, &lineHandle, it->dwAPIVersion, 0, (DWORD_PTR)this,
			LINECALLPRIVILEGE_OWNER, LINEMEDIAMODE_UNKNOWN, NULL);
		printf("dial: lineOpen returned: %08x\n", n);
		if (n != 0)
			return;

		HCALL callHandle;
		n = lineMakeCall(lineHandle, &callHandle, number, 0, NULL);
		printf("dial: lineMakeCall returned: %08x\n", n);
		if (n > 0)
			RunMessageLoop();
		n = lineClose(lineHandle);
		printf("dial: lineClose returned: %08x\n", n);
	}
	void answer(int deviceId)
	{
		printf("answer: deviceId = %d\n", deviceId);
		LinesList l = GetLinesList();
		auto it = std::find_if(std::begin(l), std::end(l), [=](const lineDesc& ld)
		{
			return ld.lineId == deviceId;
		});
		if (it == std::end(l))
		{
			printf("answer: linedesc not found\n");
			return;
		}
		HLINE lineHandle;
		auto n = lineOpen(m_appHandle, deviceId, &lineHandle, it->dwAPIVersion, 0, (DWORD_PTR)this,
			LINECALLPRIVILEGE_OWNER, LINEMEDIAMODE_UNKNOWN, NULL);
		printf("answer: lineOpen returned: %08x\n", n);
		if (n != 0)
			return;

		RunMessageLoop();
		n = lineClose(lineHandle);
		printf("answer: lineClose returned: %08x\n", n);
	}
	void RunMessageLoop()
	{
		printf("RunMessageLoop: entry\n");
		LINEMESSAGE msg;
		while (true)
		{
			int n = lineGetMessage(m_appHandle, &msg, 10 * 1000);
			printf("RunMessageLoop: lineGetMessage returned: %08x\n", n);
			if (n < 0)
				return;
			printf("RunMessageLoop: msg.dwMessageID = %s\n", MessageIdToText(msg.dwMessageID));
			switch (msg.dwMessageID)
			{
			case LINE_REPLY:
				printf("RunMessageLoop: request id = %08x, result code = %08x\n", msg.dwParam1, msg.dwParam2);
				break;
			}
		}
		printf("RunMessageLoop: exit\n");
	}
	const char *MessageIdToText(int dwMessageID)
	{
		static char s[100];
#define CASE_TEXT(n) case n: return #n;
		switch (dwMessageID)
		{
		default:
			sprintf(s, "Unknown: %d", dwMessageID);
			return s;

CASE_TEXT(LINE_CALLINFO)
CASE_TEXT(LINE_CALLSTATE)
CASE_TEXT(LINE_CLOSE)
CASE_TEXT(LINE_DEVSPECIFIC)
CASE_TEXT(LINE_DEVSPECIFICFEATURE)
CASE_TEXT(LINE_GATHERDIGITS)
CASE_TEXT(LINE_GENERATE)
CASE_TEXT(LINE_LINEDEVSTATE)
CASE_TEXT(LINE_MONITORDIGITS)
CASE_TEXT(LINE_MONITORMEDIA)
CASE_TEXT(LINE_MONITORTONE)
CASE_TEXT(LINE_REPLY)
CASE_TEXT(LINE_REQUEST)
CASE_TEXT(PHONE_BUTTON)
CASE_TEXT(PHONE_CLOSE)
CASE_TEXT(PHONE_DEVSPECIFIC)
CASE_TEXT(PHONE_REPLY)
CASE_TEXT(PHONE_STATE)
CASE_TEXT(LINE_CREATE)
CASE_TEXT(PHONE_CREATE)
		}
#undef CASE_TEXT
	}
};

void TestDial(int ac, char *av[])
{
	printf("TestDial\n");
	if (ac < 4)
	{
		printf("usage: testapp.exe dial <device id> <number>\n");
		return;
	}
	std::string idStr = av[2];
	std::string numToDial = av[3];
	std::stringstream s(idStr);
	int providerId;
	s >> providerId;
	DialTester dt;
	dt.dial(providerId, numToDial.c_str());
	printf("TestDial exit\n");
}

void TestAnswer(int ac, char *av[])
{
	printf("TestAnswer\n");
	if (ac < 4)
	{
		printf("usage: testapp.exe answer <device id>\n");
		return;
	}
	std::string idStr = av[2];
	std::stringstream s(idStr);
	int providerId;
	s >> providerId;
	DialTester dt;
	dt.answer(providerId);
	printf("TestAnswer exit\n");
}

void TestGetProvidersList(int ac, char *av[])
{
	printf("TestGetProvidersList\n");
	DialTester dt;
	DialTester::ProvidersList p = dt.GetProvidersList();
	printf("Id\tName\n");
	for (int j = 0; j < p.size(); j++)
	{
		printf("%d\t%s\n", p[j].providerId, p[j].providerName.c_str());
	}
}

void TestGetLinesList(int ac, char *av[])
{
	printf("TestGetLinesList\n");
	DialTester dt;
	DialTester::LinesList l = dt.GetLinesList();
	printf("l.size() = %zd\n", l.size());
	printf("lineId\tAPIVersion\tName\n");
	for (int j = 0; j < l.size(); j++)
	{
		printf("%d\t%08x\t%s\n", l[j].lineId, l[j].dwAPIVersion, l[j].lineName.c_str());
	}
}

int main(int ac, char *av[])
{
	if (ac < 2)
	{
		printf("usage: testapp.exe <command>\n");
		printf("commands: install, uninstall, dial, answer, getprovlist, getlineslist\n");
		return 0;
	}
	std::string command = av[1];
	if (command == "install")
		TestInstall(ac, av);
	else if(command == "uninstall")
		TestUninstall(ac, av);
	else if (command == "dial")
		TestDial(ac, av);
	else if (command == "answer")
		TestAnswer(ac, av);
	else if (command == "getprovlist")
		TestGetProvidersList(ac, av);
	else if (command == "getlineslist")
		TestGetLinesList(ac, av);
	else
	{
		printf("invalid command\n");
		printf("commands: install, uninstall, dial, answer, getprovlist, getlineslist\n");
	}
	return 0;
}
