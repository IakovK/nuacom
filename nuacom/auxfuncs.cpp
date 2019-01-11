#include "stdafx.h"
#include "nuacom.h"

LONG
TSPIAPI
TSPI_lineAccept(
	DRV_REQUESTID       dwRequestID,
	HDRVCALL            hdCall,
	LPCSTR              lpsUserUserInfo,
	DWORD               dwSize
)
{
	DBGOUT((3, "TSPI_lineAccept"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

LONG
TSPIAPI
TSPI_lineAddToConference(
	DRV_REQUESTID       dwRequestID,
	HDRVCALL            hdConfCall,
	HDRVCALL            hdConsultCall
)
{
	DBGOUT((3, "TSPI_lineAddToConference"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

LONG
TSPIAPI
TSPI_lineBlindTransfer(
	DRV_REQUESTID       dwRequestID,
	HDRVCALL            hdCall,
	LPCWSTR             lpszDestAddress,
	DWORD               dwCountryCode)
{
	DBGOUT((3, "TSPI_lineAddToConference"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

LONG
TSPIAPI
TSPI_lineCompleteCall(
	DRV_REQUESTID       dwRequestID,
	HDRVCALL            hdCall,
	LPDWORD             lpdwCompletionID,
	DWORD               dwCompletionMode,
	DWORD               dwMessageID
)
{
	DBGOUT((3, "TSPI_lineCompleteCall"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

LONG
TSPIAPI
TSPI_lineCompleteTransfer(
	DRV_REQUESTID       dwRequestID,
	HDRVCALL            hdCall,
	HDRVCALL            hdConsultCall,
	HTAPICALL           htConfCall,
	LPHDRVCALL          lphdConfCall,
	DWORD               dwTransferMode
)
{
	DBGOUT((3, "TSPI_lineCompleteTransfer"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

LONG
TSPIAPI
TSPI_lineConditionalMediaDetection(
	HDRVLINE            hdLine,
	DWORD               dwMediaModes,
	LPLINECALLPARAMS    const lpCallParams
)
{
	DBGOUT((3, "TSPI_lineConditionalMediaDetection"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

LONG
TSPIAPI
TSPI_lineDevSpecificFeature(
	DRV_REQUESTID       dwRequestID,
	HDRVLINE            hdLine,
	DWORD               dwFeature,
	LPVOID              lpParams,
	DWORD               dwSize
)
{
	DBGOUT((3, "TSPI_lineDevSpecificFeature"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

LONG
TSPIAPI
TSPI_lineDropOnClose(                                           // TSPI v1.4
	HDRVCALL            hdCall
)
{
	DBGOUT((3, "TSPI_lineDropOnClose"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

LONG
TSPIAPI
TSPI_lineDropNoOwner(                                           // TSPI v1.4
	HDRVCALL            hdCall
)
{
	DBGOUT((3, "TSPI_lineDropNoOwner"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

LONG
TSPIAPI
TSPI_lineForward(
	DRV_REQUESTID       dwRequestID,
	HDRVLINE            hdLine,
	DWORD               bAllAddresses,
	DWORD               dwAddressID,
	LPLINEFORWARDLIST   const lpForwardList,
	DWORD               dwNumRingsNoAnswer,
	HTAPICALL           htConsultCall,
	LPHDRVCALL          lphdConsultCall,
	LPLINECALLPARAMS    const lpCallParams
)
{
	DBGOUT((3, "TSPI_lineForward"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

LONG
TSPIAPI
TSPI_lineGatherDigits(
	HDRVCALL            hdCall,
	DWORD               dwEndToEndID,
	DWORD               dwDigitModes,
	_Out_writes_opt_(dwNumDigits) LPWSTR lpsDigits,
	DWORD               dwNumDigits,
	LPCWSTR             lpszTerminationDigits,
	DWORD               dwFirstDigitTimeout,
	DWORD               dwInterDigitTimeout
)
{
	DBGOUT((3, "TSPI_lineGatherDigits"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

LONG
TSPIAPI
TSPI_lineGenerateDigits(
	HDRVCALL            hdCall,
	DWORD               dwEndToEndID,
	DWORD               dwDigitMode,
	LPCWSTR             lpszDigits,
	DWORD               dwDuration
)
{
	DBGOUT((3, "TSPI_lineGenerateDigits"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

LONG
TSPIAPI
TSPI_lineGenerateTone(
	HDRVCALL            hdCall,
	DWORD               dwEndToEndID,
	DWORD               dwToneMode,
	DWORD               dwDuration,
	DWORD               dwNumTones,
	LPLINEGENERATETONE  const lpTones
)
{
	DBGOUT((3, "TSPI_lineGenerateTone"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

LONG
TSPIAPI
TSPI_lineGetCallAddressID(
	HDRVCALL            hdCall,
	LPDWORD             lpdwAddressID
)
{
	DBGOUT((3, "TSPI_lineGetCallAddressID"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

#if 0
LONG
TSPIAPI
TSPI_lineGetExtensionID(
	DWORD               dwDeviceID,
	DWORD               dwTSPIVersion,
	LPLINEEXTENSIONID   lpExtensionID
)
{
	DBGOUT((3, "TSPI_lineGetExtensionID"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}
#endif

LONG
TSPIAPI
TSPI_lineHold(
	DRV_REQUESTID       dwRequestID,
	HDRVCALL            hdCall
)
{
	DBGOUT((3, "TSPI_lineHold"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

LONG
TSPIAPI
TSPI_lineMonitorDigits(
	HDRVCALL            hdCall,
	DWORD               dwDigitModes
)
{
	DBGOUT((3, "TSPI_lineMonitorDigits"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

LONG
TSPIAPI
TSPI_lineMonitorMedia(
	HDRVCALL            hdCall,
	DWORD               dwMediaModes
)
{
	DBGOUT((3, "TSPI_lineMonitorMedia"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

LONG
TSPIAPI
TSPI_lineMonitorTones(
	HDRVCALL            hdCall,
	DWORD               dwToneListID,
	LPLINEMONITORTONE   const lpToneList,
	DWORD               dwNumEntries
)
{
	DBGOUT((3, "TSPI_lineMonitorTones"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

LONG
TSPIAPI
TSPI_linePark(
	DRV_REQUESTID       dwRequestID,
	HDRVCALL            hdCall,
	DWORD               dwParkMode,
	LPCWSTR             lpszDirAddress,
	LPVARSTRING         lpNonDirAddress
)
{
	DBGOUT((3, "TSPI_linePark"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

LONG
TSPIAPI
TSPI_linePickup(
	DRV_REQUESTID       dwRequestID,
	HDRVLINE            hdLine,
	DWORD               dwAddressID,
	HTAPICALL           htCall,
	LPHDRVCALL          lphdCall,
	LPCWSTR             lpszDestAddress,
	LPCWSTR             lpszGroupID
)
{
	DBGOUT((3, "TSPI_linePickup"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

LONG
TSPIAPI
TSPI_linePrepareAddToConference(
	DRV_REQUESTID       dwRequestID,
	HDRVCALL            hdConfCall,
	HTAPICALL           htConsultCall,
	LPHDRVCALL          lphdConsultCall,
	LPLINECALLPARAMS    const lpCallParams
)
{
	DBGOUT((3, "TSPI_linePrepareAddToConference"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

LONG
TSPIAPI
TSPI_lineRedirect(
	DRV_REQUESTID       dwRequestID,
	HDRVCALL            hdCall,
	LPCWSTR             lpszDestAddress,
	DWORD               dwCountryCode
)
{
	DBGOUT((3, "TSPI_lineRedirect"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

LONG
TSPIAPI
TSPI_lineReleaseUserUserInfo(                                   // TSPI v1.4
	DRV_REQUESTID       dwRequestID,
	HDRVCALL            hdCall
)
{
	DBGOUT((3, "TSPI_lineReleaseUserUserInfo"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

LONG
TSPIAPI
TSPI_lineRemoveFromConference(
	DRV_REQUESTID       dwRequestID,
	HDRVCALL            hdCall
)
{
	DBGOUT((3, "TSPI_lineRemoveFromConference"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

LONG
TSPIAPI
TSPI_lineSecureCall(
	DRV_REQUESTID       dwRequestID,
	HDRVCALL            hdCall
)
{
	DBGOUT((3, "TSPI_lineSecureCall"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

LONG
TSPIAPI
TSPI_lineSelectExtVersion(
	HDRVLINE            hdLine,
	DWORD               dwExtVersion
)
{
	DBGOUT((3, "TSPI_lineSelectExtVersion"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

LONG
TSPIAPI
TSPI_lineSendUserUserInfo(
	DRV_REQUESTID       dwRequestID,
	HDRVCALL            hdCall,
	LPCSTR              lpsUserUserInfo,
	DWORD               dwSize
)
{
	DBGOUT((3, "TSPI_lineSendUserUserInfo"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

LONG
TSPIAPI
TSPI_lineSetCallData(                                           // TSPI v2.0
	DRV_REQUESTID       dwRequestID,
	HDRVCALL            hdCall,
	LPVOID              lpCallData,
	DWORD               dwSize
)
{
	DBGOUT((3, "TSPI_lineSetCallData"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

LONG
TSPIAPI
TSPI_lineSetCallParams(
	DRV_REQUESTID       dwRequestID,
	HDRVCALL            hdCall,
	DWORD               dwBearerMode,
	DWORD               dwMinRate,
	DWORD               dwMaxRate,
	LPLINEDIALPARAMS    const lpDialParams
)
{
	DBGOUT((3, "TSPI_lineSetCallParams"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

LONG
TSPIAPI
TSPI_lineSetCallQualityOfService(                               // TSPI v2.0
	DRV_REQUESTID       dwRequestID,
	HDRVCALL            hdCall,
	LPVOID              lpSendingFlowspec,
	DWORD               dwSendingFlowspecSize,
	LPVOID              lpReceivingFlowspec,
	DWORD               dwReceivingFlowspecSize
)
{
	DBGOUT((3, "TSPI_lineSetCallQualityOfService"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

LONG
TSPIAPI
TSPI_lineSetCallTreatment(                                      // TSPI v2.0
	DRV_REQUESTID       dwRequestID,
	HDRVCALL            hdCall,
	DWORD               dwTreatment
)
{
	DBGOUT((3, "TSPI_lineSetCallTreatment"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

LONG
TSPIAPI
TSPI_lineSetCurrentLocation(                                    // TSPI v1.4
	DWORD               dwLocation
)
{
	DBGOUT((3, "TSPI_lineSetCurrentLocation"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

LONG
TSPIAPI
TSPI_lineSetLineDevStatus(                                      // TSPI v2.0
	DRV_REQUESTID       dwRequestID,
	HDRVLINE            hdLine,
	DWORD               dwStatusToChange,
	DWORD               fStatus
)
{
	DBGOUT((3, "TSPI_lineSetLineDevStatus"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

LONG
TSPIAPI
TSPI_lineSetMediaControl(
	HDRVLINE                    hdLine,
	DWORD                       dwAddressID,
	HDRVCALL                    hdCall,
	DWORD                       dwSelect,
	LPLINEMEDIACONTROLDIGIT     const lpDigitList,
	DWORD                       dwDigitNumEntries,
	LPLINEMEDIACONTROLMEDIA     const lpMediaList,
	DWORD                       dwMediaNumEntries,
	LPLINEMEDIACONTROLTONE      const lpToneList,
	DWORD                       dwToneNumEntries,
	LPLINEMEDIACONTROLCALLSTATE const lpCallStateList,
	DWORD                       dwCallStateNumEntries
)
{
	DBGOUT((3, "TSPI_lineSetMediaControl"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

LONG
TSPIAPI
TSPI_lineSetMediaMode(
	HDRVCALL            hdCall,
	DWORD               dwMediaMode
)
{
	DBGOUT((3, "TSPI_lineSetMediaMode"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

LONG
TSPIAPI
TSPI_lineSetTerminal(
	DRV_REQUESTID       dwRequestID,
	HDRVLINE            hdLine,
	DWORD               dwAddressID,
	HDRVCALL            hdCall,
	DWORD               dwSelect,
	DWORD               dwTerminalModes,
	DWORD               dwTerminalID,
	DWORD               bEnable
)
{
	DBGOUT((3, "TSPI_lineSetTerminal"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

LONG
TSPIAPI
TSPI_lineSetupConference(
	DRV_REQUESTID       dwRequestID,
	HDRVCALL            hdCall,
	HDRVLINE            hdLine,
	HTAPICALL           htConfCall,
	LPHDRVCALL          lphdConfCall,
	HTAPICALL           htConsultCall,
	LPHDRVCALL          lphdConsultCall,
	DWORD               dwNumParties,
	LPLINECALLPARAMS    const lpCallParams
)
{
	DBGOUT((3, "TSPI_lineSetupConference"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

LONG
TSPIAPI
TSPI_lineSetupTransfer(
	DRV_REQUESTID       dwRequestID,
	HDRVCALL            hdCall,
	HTAPICALL           htConsultCall,
	LPHDRVCALL          lphdConsultCall,
	LPLINECALLPARAMS    const lpCallParams
)
{
	DBGOUT((3, "TSPI_lineSetupTransfer"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

LONG
TSPIAPI
TSPI_lineSwapHold(
	DRV_REQUESTID       dwRequestID,
	HDRVCALL            hdActiveCall,
	HDRVCALL            hdHeldCall
)
{
	DBGOUT((3, "TSPI_lineSwapHold"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

LONG
TSPIAPI
TSPI_lineUncompleteCall(
	DRV_REQUESTID       dwRequestID,
	HDRVLINE            hdLine,
	DWORD               dwCompletionID
)
{
	DBGOUT((3, "TSPI_lineUncompleteCall"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

LONG
TSPIAPI
TSPI_lineUnhold(
	DRV_REQUESTID       dwRequestID,
	HDRVCALL            hdCall
)
{
	DBGOUT((3, "TSPI_lineUnhold"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}

LONG
TSPIAPI
TSPI_lineUnpark(
	DRV_REQUESTID       dwRequestID,
	HDRVLINE            hdLine,
	DWORD               dwAddressID,
	HTAPICALL           htCall,
	LPHDRVCALL          lphdCall,
	LPCWSTR             lpszDestAddress
)
{
	DBGOUT((3, "TSPI_lineUnpark"));
	LONG        lResult = LINEERR_OPERATIONFAILED;

	return lResult;
}
