// aspi32.cpp

//
#include <windows.h>
#include "aspi32.h"
//

//
int CAspi32::GetHACount( void )
{
	dwAspiState = GetASPI32SupportInfo();

	if( HIBYTE( LOWORD( dwAspiState )) == SS_COMP ){
		nHACount = LOBYTE( LOWORD( dwAspiState ));
	} else {
	//	AfxMessageBox( "ASPI for Win32 is not initialized.", MB_OK, 0 );
		nHACount = 0;
	}

	return( nHACount );
}

//
BOOL CAspi32::GetHAInquiry( int nHANo, LPSTR lpszInq1, LPSTR lpszInq2 )
{
	SRB_HAInquiry hainq;

	hainq.SRB_Cmd		= SC_HA_INQUIRY;
	hainq.SRB_HaId		= nHANo;
	hainq.SRB_Flags		= 0;
	hainq.SRB_Hdr_Rsvd	= 0;
	if( SendASPI32Command( &hainq ) == SS_COMP ){
		CopyMemory( &lpszInq1[0], hainq.HA_Identifier, 16 );
		lpszInq1[16] = 0x00;
		CopyMemory( &lpszInq2[0], hainq.HA_ManagerId, 16 );
		lpszInq2[16] = 0x00;
		return( TRUE );
	}

	return( FALSE );
}

//
int CAspi32::GetHAScsiID( int nHANo )
{
	SRB_HAInquiry hainq;

	hainq.SRB_Cmd		= SC_HA_INQUIRY;
	hainq.SRB_HaId		= nHANo;
	hainq.SRB_Flags		= 0;
	hainq.SRB_Hdr_Rsvd	= 0;
	if( SendASPI32Command( &hainq ) == SS_COMP ){
		return( hainq.HA_SCSI_ID );
	}

	return( -1 );
}

//
BOOL CAspi32::GetScsiType( int nHANo, int nID, int nLUN )
{
	SRB_GDEVBlock gdtype;

	gdtype.SRB_Cmd		= SC_GET_DEV_TYPE;
	gdtype.SRB_HaId		= nHANo;
	gdtype.SRB_Flags	= 0;
	gdtype.SRB_Hdr_Rsvd	= 0;
	gdtype.SRB_Target	= nID;
	gdtype.SRB_Lun		= nLUN;
	if( SendASPI32Command( &gdtype ) == SS_COMP ){
		return( gdtype.SRB_DeviceType );
	}

	return( -1 );
}

//
BOOL CAspi32::TestUnitReady( int nHANo, int nID, int nLUN )
{
	SRB_ExecSCSICmd cmd;
	HANDLE hAspiEvent;
	DWORD dwAspiState;
	DWORD dwEventState;

	hAspiEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
	if( hAspiEvent == NULL ){
		return( FALSE );
	}

	cmd.SRB_Cmd			= SC_EXEC_SCSI_CMD;
	cmd.SRB_HaId		= nHANo;
	cmd.SRB_Flags		= SRB_DIR_IN | SRB_EVENT_NOTIFY;
	cmd.SRB_Hdr_Rsvd	= 0;
	cmd.SRB_Target		= nID;
	cmd.SRB_Lun			= nLUN;
	cmd.SRB_BufLen		= 0;
	cmd.SRB_BufPointer	= NULL;
	cmd.SRB_SenseLen	= SENSE_LEN;
	cmd.SRB_CDBLen		= 6;
	cmd.SRB_PostProc	= hAspiEvent;
	cmd.CDBByte[0]		= SCSI_TST_U_RDY;
	cmd.CDBByte[1]		= ( nLUN << 5 ) & 0xE0;
	cmd.CDBByte[2]		= 0;
	cmd.CDBByte[3]		= 0;
	cmd.CDBByte[4]		= 0;
	cmd.CDBByte[5]		= 0;

	dwAspiState = SendASPI32Command( &cmd );

	dwEventState = WAIT_OBJECT_0;

	if( cmd.SRB_Status == SS_PENDING ){
		dwEventState = WaitForSingleObject( hAspiEvent, INFINITE );
	}

	if( dwEventState == WAIT_OBJECT_0 ){
		ResetEvent( hAspiEvent );
	}

	return(( cmd.SRB_Status == SS_COMP ) ? TRUE : FALSE );
}

//
BOOL CAspi32::GetScsiInquiry( int nHANo, int nID, int nLUN, int nLen, LPBYTE lpbBuf )
{
	SRB_ExecSCSICmd inq;
	HANDLE hAspiEvent;
	DWORD dwAspiState;
	DWORD dwEventState;

	hAspiEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
	if( hAspiEvent == NULL ){
		return( FALSE );
	}

	inq.SRB_Cmd			= SC_EXEC_SCSI_CMD;
	inq.SRB_HaId		= nHANo;
	inq.SRB_Flags		= SRB_DIR_IN | SRB_EVENT_NOTIFY;
	inq.SRB_Hdr_Rsvd	= 0;
	inq.SRB_Target		= nID;
	inq.SRB_Lun			= nLUN;
	inq.SRB_BufLen		= nLen;
	inq.SRB_BufPointer	= lpbBuf;
	inq.SRB_SenseLen	= SENSE_LEN;
	inq.SRB_CDBLen		= 6;
	inq.SRB_PostProc	= hAspiEvent;
	inq.CDBByte[0]		= SCSI_INQUIRY;
	inq.CDBByte[1]		= ( nLUN << 5 ) & 0xE0;
	inq.CDBByte[2]		= 0;
	inq.CDBByte[3]		= 0;
	inq.CDBByte[4]		= nLen;
	inq.CDBByte[5]		= 0;

	dwAspiState = SendASPI32Command( &inq );

	dwEventState = WAIT_OBJECT_0;

	if( inq.SRB_Status == SS_PENDING ){
		dwEventState = WaitForSingleObject( hAspiEvent, INFINITE );
	}

	if( dwEventState == WAIT_OBJECT_0 ){
		ResetEvent( hAspiEvent );
	}

	if( inq.SRB_Status == SS_COMP ){
		return( TRUE );
	}

	return( FALSE );
}

//
BOOL CAspi32::ReadCapacity( int nHANo, int nID, int nLUN, LPBYTE lpbBuf )
{
	SRB_ExecSCSICmd inq;
	HANDLE hAspiEvent;
	DWORD dwAspiState;
	DWORD dwEventState;

	hAspiEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
	if( hAspiEvent == NULL ){
		return( FALSE );
	}

	inq.SRB_Cmd			= SC_EXEC_SCSI_CMD;
	inq.SRB_HaId		= nHANo;
	inq.SRB_Flags		= SRB_DIR_IN | SRB_EVENT_NOTIFY;
	inq.SRB_Hdr_Rsvd	= 0;
	inq.SRB_Target		= nID;
	inq.SRB_Lun			= nLUN;
	inq.SRB_BufLen		= 8;
	inq.SRB_BufPointer	= lpbBuf;
	inq.SRB_SenseLen	= SENSE_LEN;
	inq.SRB_CDBLen		= 10;
	inq.SRB_PostProc	= hAspiEvent;
	inq.CDBByte[0]		= SCSI_READCAPA;
	inq.CDBByte[1]		= ( nLUN << 5 ) & 0xE0;
	inq.CDBByte[2]		= 0;
	inq.CDBByte[3]		= 0;
	inq.CDBByte[4]		= 0;
	inq.CDBByte[5]		= 0;
	inq.CDBByte[6]		= 0;
	inq.CDBByte[7]		= 0;
	inq.CDBByte[8]		= 0;
	inq.CDBByte[9]		= 0;

	dwAspiState = SendASPI32Command( &inq );

	dwEventState = WAIT_OBJECT_0;

	if( inq.SRB_Status == SS_PENDING ){
		dwEventState = WaitForSingleObject( hAspiEvent, INFINITE );
	}

	if( dwEventState == WAIT_OBJECT_0 ){
		ResetEvent( hAspiEvent );
	}

	if( inq.SRB_Status == SS_COMP ){
		return( TRUE );
	}

	return( FALSE );
}

//
BOOL CAspi32::StartStopUnit( int nHANo, int nID, int nLUN, BOOL fEject, BOOL fStart )
{
	SRB_ExecSCSICmd inq;
	HANDLE hAspiEvent;
	DWORD dwAspiState;
	DWORD dwEventState;

	hAspiEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
	if( hAspiEvent == NULL ){
		return( FALSE );
	}

	inq.SRB_Cmd			= SC_EXEC_SCSI_CMD;
	inq.SRB_HaId		= nHANo;
	inq.SRB_Flags		= SRB_DIR_IN | SRB_EVENT_NOTIFY;
	inq.SRB_Hdr_Rsvd	= 0;
	inq.SRB_Target		= nID;
	inq.SRB_Lun			= nLUN;
	inq.SRB_BufLen		= 0;
	inq.SRB_BufPointer	= NULL;
	inq.SRB_SenseLen	= SENSE_LEN;
	inq.SRB_CDBLen		= 6;
	inq.SRB_PostProc	= hAspiEvent;
	inq.CDBByte[0]		= SCSI_STARTSTOP;
	inq.CDBByte[1]		= ( nLUN << 5 ) & 0xE0;
	inq.CDBByte[2]		= 0;
	inq.CDBByte[3]		= 0;
	inq.CDBByte[4]		= ( fEject ? 0x02 : 0x00 ) | ( fStart ? 0x01 : 0x00 );
	inq.CDBByte[5]		= 0;

	dwAspiState = SendASPI32Command( &inq );

	dwEventState = WAIT_OBJECT_0;

	if( inq.SRB_Status == SS_PENDING ){
		dwEventState = WaitForSingleObject( hAspiEvent, INFINITE );
	}

	if( dwEventState == WAIT_OBJECT_0 ){
		ResetEvent( hAspiEvent );
	}

	if( inq.SRB_Status == SS_COMP ){
		return( TRUE );
	}

	return( FALSE );
}

//
void CScsiInq::SplitInquiryData( LPBYTE lpbBuf )
{
	nDevType	= lpbBuf[0] & 0x1F;
	fRemovable	= ((( lpbBuf[1] & 0x80 ) != 0 ) ? TRUE : FALSE );
	nAnsiVer	= lpbBuf[2] & 0x07;
	nInqLen		= lpbBuf[4];

	ZeroMemory( szVenderID, sizeof(szVenderID) );
	CopyMemory( szVenderID, &lpbBuf[8], 8 );

	ZeroMemory( szProductID, sizeof(szProductID) );
	CopyMemory( szProductID, &lpbBuf[16], 16 );

	ZeroMemory( szRevision, sizeof(szRevision) );
	CopyMemory( szRevision, &lpbBuf[32], 4 );

	const static char* pszDevType[10] = {
		"Direct Access",
		"Sequential Access",
		"Printer",
		"Processer",
		"Write Once",
		"CD-ROM",
		"Scanner",
		"Magnet Optical",
		"Media Chenger",
		"Communication",
	};

	if( nDevType < 10 ){
		wsprintf( szDevTypeStr, "%s (%u)", pszDevType[nDevType], nDevType );
	} else {
		wsprintf( szDevTypeStr, "UNKNOWN (%u)", nDevType );
	}

	return;
}

// end-of-file
