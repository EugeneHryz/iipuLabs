#include <iostream>
#include <windows.h>
#include <setupapi.h>
#include <vector>
#include <string>
#include <iomanip>

using namespace std;

#pragma comment (lib, "setupapi.lib")


#define BUF_SIZE 300

void printDeviceInfo(HDEVINFO hdevinfo) {

	SP_DEVINFO_DATA DeviceInfoData;
	DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

	BYTE *tmpBuf = new BYTE[BUF_SIZE];

	vector<string> deviceIds;

	int i = 0;
	while (SetupDiEnumDeviceInfo(hdevinfo, i, &DeviceInfoData)) {

		if (SetupDiGetDeviceRegistryProperty(hdevinfo, &DeviceInfoData, SPDRP_DEVICEDESC, NULL, tmpBuf, BUF_SIZE, NULL)) {

			cout << tmpBuf << endl;
		}
		
		if (SetupDiGetDeviceRegistryProperty(hdevinfo, &DeviceInfoData, SPDRP_HARDWAREID, NULL, tmpBuf, BUF_SIZE, NULL)) {

			string s((const char*)tmpBuf);
			cout << "Device ID: " << s.substr(17, 4) << endl << "Vendor ID: " << s.substr(8, 4) << "\n\n";
		}
		i++;
	}

	delete tmpBuf;
}

int main() {

	setlocale(LC_ALL, "Russian");

	HDEVINFO hdevinfo = SetupDiGetClassDevs(NULL, "PCI", NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT);

	if (hdevinfo == INVALID_HANDLE_VALUE) {

		return GetLastError();
	}

	printDeviceInfo(hdevinfo);

	if (!SetupDiDestroyDeviceInfoList(hdevinfo))
		return -1;

	return 0;
}