#include <iostream>
#include <windows.h>
#include <string>
#include <fileapi.h>
#include <vector>
#include <iomanip>
#include <ntddscsi.h>

using namespace std;

vector<string> busTypes = { "Unknown", "Scsi", "Atapi", "Ata", "1394", 
							"Ssa", "Fibre", "Usb", "RAID", "iScsi", 
							"Sas", "Sata", "Sd", "Mmc", "Virtual", 
							"FileBackedVirtual", "Spaces" };

#define BUF_SIZE 256

#define BYTE_SIZE 8

DWORD ShowSupportedStandarts(HANDLE hDrive) {
	
	UCHAR buffer[512 + sizeof(ATA_PASS_THROUGH_EX)];
	ZeroMemory(buffer, sizeof(buffer));
	DWORD returnedBytes;

	ATA_PASS_THROUGH_EX& ATA = *(ATA_PASS_THROUGH_EX*)buffer;
	ATA.Length = sizeof(ATA);
	ATA.TimeOutValue = 10;
	ATA.DataTransferLength = 512;
	ATA.DataBufferOffset = sizeof(ATA_PASS_THROUGH_EX);
	ATA.AtaFlags = ATA_FLAGS_DATA_IN;

	IDEREGS* ideRegs = (IDEREGS*)ATA.CurrentTaskFile;
	ideRegs->bCommandReg = 0xEC;

	if (!DeviceIoControl(hDrive, IOCTL_ATA_PASS_THROUGH, &ATA, sizeof(buffer), 
		&ATA, sizeof(buffer), &returnedBytes, NULL)) {
		
		return GetLastError();
	}

	WORD* data = (WORD*)(buffer + sizeof(ATA_PASS_THROUGH_EX));
	int i, bitArray[2 * BYTE_SIZE];

	cout << "Supported standarts:\n\n";

	unsigned short ataSupportByte = data[80];
	i = 2 * BYTE_SIZE;
	short wordMask = 1;
	for (i = 0; i < 2 * BYTE_SIZE; i++) {

		if (wordMask & ataSupportByte)
			bitArray[i] = 1;
		else
			bitArray[i] = 0;
		
		wordMask = wordMask << 1;
	}

	cout << setw(20) << "ATA Support: ";
	for (i = 8; i >= 4; i--) {
		
		if (bitArray[i] == 1) {
			
			cout << "ATA" << i;
			if (i != 4) {
				cout << " ";
			}
			else
				cout << endl;
		}
	}

	unsigned short pioSupportByte = data[63];
	wordMask = 1;
	for (i = 0; i < 2 * BYTE_SIZE; i++) {

		if (wordMask & pioSupportByte)
			bitArray[i] = 1;
		else
			bitArray[i] = 0;

		wordMask = wordMask << 1;
	}

	cout << setw(20) << "PIO Support: ";
	for (i = 0; i < 2; i++) {
		
		if (bitArray[i] == 1) {
			
			cout << "PIO" << i + 3;
			if (i != 1) {
				cout << " ";
			}
			else
				cout << endl;
		}
	}

	unsigned short dmaSupportByte = data[63];
	wordMask = 1;
	for (i = 0; i < 2 * BYTE_SIZE; i++) {

		if (wordMask & dmaSupportByte)
			bitArray[i] = 1;
		else
			bitArray[i] = 0;

		wordMask = wordMask << 1;
	}
	
	cout << setw(20) << "DMA Support: ";
	for (i = 0; i < 8; i++) {
		
		if (bitArray[i] == 1) {
			
			cout << "DMA" << i;
			if (i != 2) {
				cout << " ";
			}
			else
				cout << endl;
		}
	}

	return 0;
}

DWORD PrintDriveDescriptorInfo(HANDLE hDevice) {

	STORAGE_PROPERTY_QUERY spq = { StorageDeviceTrimProperty, PropertyStandardQuery };
	DWORD bytesReturned = 0;

	// obtaining drive type
	DEVICE_TRIM_DESCRIPTOR dtd;

	if (!DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, &spq, sizeof(STORAGE_PROPERTY_QUERY),
		&dtd, sizeof(DEVICE_TRIM_DESCRIPTOR), &bytesReturned, NULL)) {

		return GetLastError();
	}

	if (dtd.TrimEnabled == TRUE) {
		cout << "Drive type: SSD" << endl;
	}
	else
		cout << "Drive type: HDD" << endl;

	// obtaining drive model, manufacturer, serial number, firmware, bus type
	spq.PropertyId = StorageDeviceProperty;

	STORAGE_DESCRIPTOR_HEADER sdh;
	ZeroMemory(&sdh, sizeof(STORAGE_DESCRIPTOR_HEADER));

	if (!DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, &spq, sizeof(STORAGE_PROPERTY_QUERY),
		&sdh, sizeof(STORAGE_DESCRIPTOR_HEADER), &bytesReturned, NULL)) {

		return GetLastError();
	}

	char *outBuffer = new char[sdh.Size];

	if (!DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, &spq, sizeof(STORAGE_PROPERTY_QUERY),
		outBuffer, sdh.Size, &bytesReturned, NULL)) {

		return GetLastError();
	}

	STORAGE_DEVICE_DESCRIPTOR *sdd = (STORAGE_DEVICE_DESCRIPTOR*)outBuffer;

	if (sdd->ProductIdOffset != 0) { cout << "Drive model: " << outBuffer + sdd->ProductIdOffset << endl; }
	if (sdd->VendorIdOffset != 0) { cout << "Drive manufacturer: " << outBuffer + sdd->VendorIdOffset << endl; }
	if (sdd->SerialNumberOffset != 0) { cout << "Drive serial number: " << outBuffer + sdd->SerialNumberOffset << endl; }
	if (sdd->ProductRevisionOffset != 0) { cout << "Drive firmware: " << outBuffer + sdd->ProductRevisionOffset << endl; }
	cout << "Drive bus type: " << busTypes.at(sdd->BusType) << endl;

	delete[]outBuffer;
	
	return ShowSupportedStandarts(hDevice);;
}

vector<string> GetAllLogicalDrives() {

	char *buffer = new char[BUF_SIZE];
	vector<string> logicalDrives;
	DWORD strSize = 0;

	if ((strSize = GetLogicalDriveStrings(BUF_SIZE, buffer)) != 0) {

		const char* tmp = buffer;

		while (tmp < buffer + strSize) {

			logicalDrives.push_back(tmp);
			tmp += logicalDrives.back().size() + 1;
		}
	}
	delete[]buffer;
	return logicalDrives;
}

DWORD PrintMemoryUsage(const vector<string>& logicalDrives) {

	ULARGE_INTEGER freeBytesAvailableToCaller;
	ULARGE_INTEGER totalNumberOfBytes;
	ULARGE_INTEGER totalNumberOfFreeBytes;

	vector<string>::const_iterator it;
	for (it = logicalDrives.begin(); it != logicalDrives.end(); it++) {

		if (GetDiskFreeSpaceEx((*it).c_str(), &freeBytesAvailableToCaller, &totalNumberOfBytes, &totalNumberOfFreeBytes)) {

			
			cout << "\nVolume: " << (*it).substr(0, (*it).size() - 2) << "\nMemory:\n" << setw(12) << "Total: " << totalNumberOfBytes.QuadPart / (1024 * 1024 * 1024) << "GB\n"
				<< setw(12) << "Used: " << (totalNumberOfBytes.QuadPart - totalNumberOfFreeBytes.QuadPart) / (1024 * 1024 * 1024) << "GB\n"
				<< setw(12) << "Free: " << totalNumberOfFreeBytes.QuadPart / (1024 * 1024 * 1024) << "GB\n";
		}
		else
			return GetLastError();
	}
	return 0;
}

int main() {

	HANDLE hDevice = NULL;

	string driveName = "\\\\.\\PhysicalDrive";
	int i = 0;
	DWORD result;
	
	while ((hDevice = CreateFile((driveName + to_string(i)).c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE) {

		cout << "Drive name: PhysicalDrive" << to_string(i) << endl;
		
		result = PrintDriveDescriptorInfo(hDevice);
		if (result != 0) { return result; }

		result = PrintMemoryUsage(GetAllLogicalDrives());
		if (result != 0) { return result; }
		
		CloseHandle(hDevice);
		i++;
	}

	system("pause");

	return 0;
}

