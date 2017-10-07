#include <Windows.h>
#include <winioctl.h>
#include <ntddscsi.h>
#include <iostream>
#include <vector>
#include <stdio.h>
#include <math.h>
#include <bitset>
#include <iomanip> 
using namespace std;

class HDDinfo {
private:
	vector<WORD> data;
	bool flagIsPIO;
	__int64 TotalSpace;
	__int64 FreeSpace;

	string parseDataToString(int start, int end) {
		string s;
		for (int i = start; i<end; i++) {
			s.push_back((char)(data[i] >> 8));
			s.push_back((char)data[i]);
		}
		return s;
	}

	void getTotalAndFreeSpace()
	{
		TotalSpace = FreeSpace = 0;
		ULARGE_INTEGER FreeBytesAvailable;
		ULARGE_INTEGER TotalNumberOfBytes;
		ULARGE_INTEGER TotalNumberOfFreeBytes;
		bitset<16> drives((__int16)GetLogicalDrives());
		for (int i = 0; i<16; i++) {
			if (drives[i] == 1) {
				wchar_t s1[8];
				s1[0] = (wchar_t)('A' + i);
				s1[1] = '\0';
				wcscat(s1, L":\\");
				if (GetDriveType(s1) == 3)
				{
					bool fl = GetDiskFreeSpaceEx(s1, &FreeBytesAvailable, &TotalNumberOfBytes, &TotalNumberOfFreeBytes);
					if (fl) {
						TotalSpace += TotalNumberOfBytes.QuadPart;
						FreeSpace += TotalNumberOfFreeBytes.QuadPart;
					}
					else { throw new runtime_error("Can't read disk"); }
				}
			}
		}
	}
public:

	HDDinfo() {
		HANDLE hDevice = INVALID_HANDLE_VALUE;
		hDevice = CreateFileW(L"\\\\.\\PhysicalDrive0",          // drive to open
			GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

		if (hDevice == INVALID_HANDLE_VALUE)    // cannot open the drive
		{
			cout << "Error openeing device: " << GetLastError() << endl;
		}

		CONST UINT bufferSize = 512;
		UCHAR identifyDataBuffer[bufferSize + sizeof(ATA_PASS_THROUGH_EX)] = { 0 };


		BOOL status = FALSE;
		DWORD dataSize = sizeof(ATA_PASS_THROUGH_EX) + 512;
		BYTE Buffer[sizeof(ATA_PASS_THROUGH_EX) + 512];
		DWORD bytescopied = 0;
		PATA_PASS_THROUGH_EX pATAData = (ATA_PASS_THROUGH_EX *)identifyDataBuffer;

		ZeroMemory(pATAData, dataSize); // clears the buffer

		pATAData->Length = sizeof(ATA_PASS_THROUGH_EX);
		pATAData->DataBufferOffset = sizeof(ATA_PASS_THROUGH_EX);
		pATAData->DataTransferLength = 512;
		pATAData->TimeOutValue = 2;
		pATAData->AtaFlags = ATA_FLAGS_DRDY_REQUIRED | ATA_FLAGS_DATA_IN;

		CONST BYTE identifyDataCommandId = 0xEC;
		IDEREGS * ideRegs = (IDEREGS *)pATAData->CurrentTaskFile;
		ideRegs->bCommandReg = identifyDataCommandId;
		ideRegs->bSectorCountReg = 1;

		BOOL bResult = DeviceIoControl(hDevice, IOCTL_ATA_PASS_THROUGH, pATAData,
			dataSize, pATAData,
			dataSize, &bytescopied, 0);

		if (bResult == FALSE) {
			std::cout << "Oops, something went wrong, error code: "
				<< GetLastError() << std::endl;
		}

		WORD *dataIn = (WORD*)(identifyDataBuffer + sizeof(ATA_PASS_THROUGH_EX));

		for (int i = 0; i<256; i++) {
			data.push_back(dataIn[i]);
		}

		STORAGE_PROPERTY_QUERY query = {};
		query.PropertyId = StorageAdapterProperty;
		query.QueryType = PropertyStandardQuery;

		STORAGE_ADAPTER_DESCRIPTOR descriptor = {};

		DWORD read;
		if (!DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY,
			&query,
			sizeof(query),
			&descriptor,
			sizeof(descriptor),
			&read,
			NULL
		))
		{
			throw(L"DeviceIoControl error: %i\n", GetLastError());
		}
		else
		{
			flagIsPIO = descriptor.AdapterUsesPio;
		}
		CloseHandle(hDevice);
	}


	string getSerialNumber() {
		return parseDataToString(10, 19);
	}

	string getFirmwareRevision() {
		return parseDataToString(23, 26);
	}

	string getModelNumber() {
		return parseDataToString(27, 46);
	}

	vector<string> getATASupport() {
		string ATAstandards[] = { "Reserved", "Obsolete", "Obsolete", "Obsolete",
			"Obsolete", "ATA/ATAPI-5", "ATA/ATAPI-6", "ATA/ATAPI-7",
			"ATA8-ACS", "ACS-2", "ACS-3" };
		vector<string> v;
		bitset<16> ATAsupport(data[80]);
		for (int i = (sizeof(ATAstandards) / sizeof(*ATAstandards)) - 1; i >= 0; i--) {
			if (ATAsupport.at(i)) {
				v.push_back(ATAstandards[i]);
			}
		}
		return v;
	}

	vector<string> getSATASupport() {
		string SATAstandards[] = { "ATA8-APT ATA8-AST", "ATA/ATAPI-7 SATA 1.0a", "SATA II: Extensions", "SATA 2.5",
			"SATA 2.6", "SATA 3.0", "SATA 3.1" };
		vector<string> v;
		bitset<16> SATAsupport(data[222]);
		for (int i = (sizeof(SATAstandards) / sizeof(*SATAstandards)) - 1; i >= 0; i--) {
			if (SATAsupport.at(i)) {
				v.push_back(SATAstandards[i]);
			}
		}
		return v;
	}

	vector<string> getDMASupport() {
		vector<string> v;
		bitset<16> DMAsupport(data[62]);
		for (int i = 15; i >= 0; i--) {
			if (DMAsupport.at(i)) {
				if (i == 0) v.push_back("Ultra DMA mode 0 is supported");
				else if (i <= 6) v.push_back("Ultra DMA mode  and below are supported" + i);
				else if (i >= 7 && i <= 9) v.push_back("Multiword DMA mode is supported" + (i - 7));
				else if (i == 10) v.push_back("DMA is supported");
				else v.push_back("Reserved");
			}
		}
		return v;
	}

	vector<string> getUltraDMASupport() {
		vector<string> v;
		char numstr[21];
		bitset<16> UltraDMAsupport(data[88]);
		for (int i = 15; i >= 0; i--) {
			if (UltraDMAsupport.at(i)) {
				if (i == 0) v.push_back("Ultra DMA mode 0 is supported");
				else if (i <= 6) {
					string s = itoa(i, numstr, 10);
					s += " and below are supported";
					v.push_back("Ultra DMA mode " + s);
				}
				else if (i>7 && i<15) {
					string s = itoa((i - 8), numstr, 10);
					s += " is selected";
					v.push_back("Ultra DMA mode " + s);
				}
				else v.push_back("Reserved");
			}
		}
		return v;
	}

	bool isPIO() {
		return flagIsPIO;
	}

	__int64 getTotalSpace() {
		getTotalAndFreeSpace();
		return TotalSpace;
	}

	__int64 getFreeSpace() {
		getTotalAndFreeSpace();
		return FreeSpace;
	}

	__int64 getOccupiedSpace() {
		getTotalAndFreeSpace();
		return TotalSpace - FreeSpace;
	}

	~HDDinfo() {
	}
};
