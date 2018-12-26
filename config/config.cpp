// config.cpp : Defines the entry point for the application.
//

#include "header.h"
#include "config.h"

int APIENTRY WinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

	std::string provName = "nuacom.tsp";

	LINEPROVIDERLIST lpl;
	lpl.dwTotalSize = sizeof lpl;
	auto n = lineGetProviderList(TAPI_CURRENT_VERSION, &lpl);
	if (n != 0)
	{
		MessageBox(NULL, "TAPI provider isn't installed", "nuacom config utility", MB_OK| MB_ICONERROR);
		return -1;
	}
	std::vector<unsigned char> v(lpl.dwNeededSize);
	LPLINEPROVIDERLIST lpl2 = (LPLINEPROVIDERLIST)v.data();
	lpl2->dwTotalSize = lpl.dwNeededSize;
	n = lineGetProviderList(TAPI_CURRENT_VERSION, lpl2);
	if (n != 0)
	{
		MessageBox(NULL, "TAPI provider isn't installed", "nuacom config utility", MB_OK | MB_ICONERROR);
		return -1;
	}
	LPLINEPROVIDERENTRY lpeptr = (LPLINEPROVIDERENTRY)(v.data() + lpl2->dwProviderListOffset);
	for (int j = 0; j < lpl2->dwNumProviders; j++)
	{
		int providerId = lpeptr[j].dwPermanentProviderID;
		std::string providerName = std::string((char*)(v.data() + lpeptr[j].dwProviderFilenameOffset), lpeptr[j].dwProviderFilenameSize - 1);
		auto n = providerName.rfind(provName);
		if (n != -1)
		{
			int l1 = n + provName.size();
			int l2 = providerName.size();
			if (l1 == l2)
			{
				n = lineConfigProvider(NULL, providerId);
				if (n != 0)
					MessageBox(NULL, "TAPI provider isn't installed", "nuacom config utility", MB_OK | MB_ICONERROR);
				return n;
			}
		}
	}

	MessageBox(NULL, "TAPI provider isn't installed", "nuacom config utility", MB_OK | MB_ICONERROR);
	return 0;
}
