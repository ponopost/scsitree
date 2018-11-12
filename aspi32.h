// aspi32.h

//
#include <windows.h>
#include "wnaspi32.h"
//

//
#define SCSI_TST_U_RDY			0x00	// Test Unit Ready
#define SCSI_INQUIRY			0x12	// Inquiry
#define SCSI_STARTSTOP			0x1B	// Start/Stop Unit
#define SCSI_READCAPA			0x25	// Read Capacity
//
#define SCSI_INQUIRY_LENGTH		36
//

//
class CAspi32
{
public:
	DWORD dwAspiState;
	int   nHACount;

public:
	CAspi32()
	{
		dwAspiState	= 0;
		nHACount	= 0;
	}

public:
	int  GetHACount( void );
	BOOL GetHAInquiry( int nHANo, LPSTR lpszInq1, LPSTR lpszInq2 );
	int  GetHAScsiID( int nHANo );
	int  GetScsiType( int nHANo, int nID, int nLUN );
	BOOL TestUnitReady( int nHANo, int nID, int nLUN );
	BOOL GetScsiInquiry( int nHANo, int nID, int nLUN, int nLen, LPBYTE lpbBuf );
	BOOL ReadCapacity( int nHANo, int nID, int nLUN, LPBYTE lpbBuf );
	BOOL StartStopUnit( int nHANo, int nID, int nLUN, BOOL fEject, BOOL fStart );
};

//
class CScsiInq
{
public:
	int  nDevType;
	BOOL fRemovable;
	int  nAnsiVer;
	int  nInqLen;
	char szVenderID[16];		// 8
	char szProductID[32];		// 16
	char szRevision[8];			// 4
	char szDevTypeStr[32];
public:
	void SplitInquiryData( LPBYTE lpbBuf );
};

// end-of-aspi33.h
