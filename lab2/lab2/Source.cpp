#include <iostream>
#include <windows.h>
#include <string>
#include <fileapi.h>
#include <vector>
#include <iomanip>

using namespace std;

vector<string> busTypes = { "Unknown", "Scsi", "Atapi", "Ata", "1394", 
							"Ssa", "Fibre", "Usb", "RAID", "iScsi", 
							"Sas", "Sata", "Sd", "Mmc", "Virtual", 
							"FileBackedVirtual", "Spaces" };

#define BUF_SIZE 256

DWORD PrintDriveDescriptorInfo(HANDLE hDevice) {

	STORAGE_PROPERTY_QUERY spq = { StorageDeviceTrimProperty, PropertyStandardQuery };
	DWORD bytesReturned = 0;

	// obtaining drive type
	DEVICE_TRIM_DESCRIPTOR dtd;

	if (!DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, &spq, sizeof(STORAGE_PROPERTY_QUERY),
		&dtd, sizeof(DEVICE_TRIM_DESCRIPTOR), &bytesReturned, NULL)) {

		return GetLastError();
	}

	if (dtd.TrimEnabled == true) {
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

	// drive transfer mode
	spq.PropertyId = StorageAdapterProperty;

	STORAGE_ADAPTER_DESCRIPTOR sad;

	if (!DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, &spq, sizeof(STORAGE_PROPERTY_QUERY),
		&sad, sizeof(STORAGE_ADAPTER_DESCRIPTOR), &bytesReturned, NULL)) {

		return GetLastError();
	}

	cout << "Supported modes: ";
	if (sad.AdapterUsesPio) {
		cout << "PIO";
	}
	cout << endl;

	delete[]outBuffer;

	return 0;
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
	
	while ((hDevice = CreateFile((driveName + to_string(i)).c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE) {

		cout << "Drive name: PhysicalDrive" << to_string(i) << endl;
		
		result = PrintDriveDescriptorInfo(hDevice);
		if (result != 0) { return result; }

		result = PrintMemoryUsage(GetAllLogicalDrives());
		if (result != 0) { return result; }
		
		i++;
	}

	return 0;
}

