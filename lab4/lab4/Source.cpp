#include <iostream>
#include <windows.h>
#include <setupapi.h>
#include <Dshow.h>
#include <combaseapi.h>
#include <conio.h>
#include "opencv2/opencv.hpp"

#pragma comment (lib, "setupapi.lib")

using namespace std;
using namespace cv;

#define FPS 25
#define BUFFER_SIZE 100

struct SaveImageData{

	string fileNameBlank;
	Mat frame;
	vector<int> compressionParams;

} saveImageData;

int photoCount = 0;
bool exitFlag = false;

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {

	bool processed = false;
	if (nCode == HC_ACTION) {

		if (wParam == WM_KEYUP) {

			KBDLLHOOKSTRUCT *hookStruct = (KBDLLHOOKSTRUCT*)lParam;
			if (hookStruct->vkCode == VK_SPACE) {

				imwrite((saveImageData.fileNameBlank + to_string(photoCount) + ".png").c_str(), 
					saveImageData.frame, saveImageData.compressionParams);
				photoCount++;
				processed = true;
			}
			if (hookStruct->vkCode == VK_ESCAPE) { exitFlag = true; }
		}
	}
	if (processed == false) return CallNextHookEx(NULL, nCode, wParam, lParam);
	return 1;
}

DWORD WINAPI captureVideo(LPVOID parameter);

int main() {

	ShowWindow(GetConsoleWindow(), SW_SHOW);
	setlocale(LC_ALL, "Russian");

	HDEVINFO hdevinfo = SetupDiGetClassDevs(NULL, "USB", NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT);

	if (hdevinfo == INVALID_HANDLE_VALUE) { return GetLastError(); }

	SP_DEVINFO_DATA devInfoData;
	devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

	int i = 0;
	BYTE *buffer = new BYTE[BUFFER_SIZE];
	while (SetupDiEnumDeviceInfo(hdevinfo, i, &devInfoData)) {

		if (!SetupDiGetDeviceRegistryProperty(hdevinfo, &devInfoData, SPDRP_SERVICE, 
			NULL, buffer, BUFFER_SIZE, NULL)) { return GetLastError(); }

		if (strcmp("usbvideo", (const char*)buffer) == 0) {
			
			if (!SetupDiGetDeviceRegistryProperty(hdevinfo, &devInfoData, SPDRP_FRIENDLYNAME,
				NULL, buffer, BUFFER_SIZE, NULL)) { return GetLastError(); }
			cout << "Friendly name: " << buffer << endl;

			if (!SetupDiGetDeviceRegistryProperty(hdevinfo, &devInfoData, SPDRP_DEVICEDESC,
				NULL, buffer, BUFFER_SIZE, NULL)) {
				return GetLastError();
			}
			cout << "Description: " << buffer << endl;

			if (!SetupDiGetDeviceRegistryProperty(hdevinfo, &devInfoData, SPDRP_MFG,
				NULL, buffer, BUFFER_SIZE, NULL)) { return GetLastError(); }
			cout << "Manufacturer: " << buffer << endl;
		}
		i++;
	}

	bool exitLoopFlag = false;
	cout << "\n\n's' - to capture video and photo in hidden mode\n'e' - to exit\n";
	do {
		char c = _getch();
		switch (c) {
		case 's':
			exitLoopFlag = true;
			break;
		case 'e':
			return 0;
		}
	} while (!exitLoopFlag);

	ShowWindow(GetConsoleWindow(), SW_HIDE);

	saveImageData.fileNameBlank = "photo/capture";
	vector<int> compressionParams;
	compressionParams.push_back(IMWRITE_PNG_COMPRESSION);
	compressionParams.push_back(777);
	saveImageData.compressionParams = compressionParams;

	HHOOK hhook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, 0, 0);
	if (hhook == NULL) { 
		cout << "SetWindowsHookEx FAILED.\n"; 
		return -1; 
	}

	HANDLE hThread;
	DWORD threadId;
	hThread = CreateThread(NULL, 0, captureVideo, NULL, 0, &threadId);

	MSG msg;
	BOOL result;
	while ((result = GetMessage(&msg, NULL, NULL, NULL)) != 0) {

		if (result != -1) {

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if (exitFlag == true) break;
	}
	WaitForSingleObject(hThread, INFINITE);
	UnhookWindowsHookEx(hhook);
	return 0;
}

DWORD WINAPI captureVideo(LPVOID parameter) {

	VideoCapture cap(0);
	if (!cap.isOpened()) {
		cout << "Error opening video stream or file" << endl;
		return -1;
	}
	VideoWriter writer("video/capture.avi", VideoWriter::fourcc('M', 'J', 'P', 'G'), FPS, Size(cap.get(CAP_PROP_FRAME_WIDTH), cap.get(CAP_PROP_FRAME_HEIGHT)));
	
	while (1) {

		Mat frame;    
		cap.read(frame);

		if (frame.empty()) break;

		writer.write(frame);
		saveImageData.frame = frame;

		waitKey(25);
		if (exitFlag == true) break;
	}
	cap.release();
	writer.release();
	destroyAllWindows();
	
	return 0;
}
