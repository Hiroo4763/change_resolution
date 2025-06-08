#include <iostream>
#include <Windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <cfgmgr32.h>
#pragma comment(lib, "setupapi.lib")
using namespace std;

class MonitorManager {
private:
    WCHAR primaryDeviceName[32];
    DEVMODEW originalMode{};
    bool originalModeSaved = false;

public:
    MonitorManager() {
        ZeroMemory(primaryDeviceName, sizeof(primaryDeviceName));
    }

    bool findPrimaryMonitor() {
        DISPLAY_DEVICEW displayDevice;
        ZeroMemory(&displayDevice, sizeof(displayDevice));
        displayDevice.cb = sizeof(displayDevice);

        int adapterIndex = 0;
        while (EnumDisplayDevicesW(NULL, adapterIndex, &displayDevice, 0)) {
            if (displayDevice.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) {
                wcscpy_s(primaryDeviceName, displayDevice.DeviceName);
                return true;
            }
            adapterIndex++;
        }
        return false;
    }

    bool getCurrentResolution() {
        ZeroMemory(&originalMode, sizeof(originalMode));
        originalMode.dmSize = sizeof(originalMode);

        if (!EnumDisplaySettingsExW(primaryDeviceName, ENUM_CURRENT_SETTINGS, &originalMode, 0)) {
            return false;
        }
        originalModeSaved = true;
        return true;
    }

    bool changeResolution(int width, int height) {
        DEVMODEW devMode;
        ZeroMemory(&devMode, sizeof(devMode));
        devMode.dmSize = sizeof(devMode);

        if (!EnumDisplaySettingsExW(primaryDeviceName, ENUM_CURRENT_SETTINGS, &devMode, 0)) {
            cout << " 현재 디스플레이 설정을 불러오지 못했습니다." << endl;
            return false;
        }

        devMode.dmPelsWidth = width;
        devMode.dmPelsHeight = height;
        devMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;

        LONG result = ChangeDisplaySettingsExW(primaryDeviceName, &devMode, NULL, CDS_UPDATEREGISTRY | CDS_RESET, NULL);

        return result == DISP_CHANGE_SUCCESSFUL;
    }

    bool restoreResolution() {
        if (!originalModeSaved) return false;

        LONG result = ChangeDisplaySettingsExW(primaryDeviceName, &originalMode, NULL, CDS_UPDATEREGISTRY | CDS_RESET, NULL);
        return result == DISP_CHANGE_SUCCESSFUL;
    }
};

class MonitorStateChanger {
public:
    void toggleMonitors(BOOL enable) {
        HDEVINFO hDevInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_MONITOR, NULL, NULL, DIGCF_PRESENT);
        if (hDevInfo == INVALID_HANDLE_VALUE) {
            cout << "장치 정보를 가져오지 못했습니다." << endl;
            return;
        }

        SP_DEVINFO_DATA devInfoData;
        devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        DWORD index = 0;

        while (SetupDiEnumDeviceInfo(hDevInfo, index, &devInfoData)) {
            SP_PROPCHANGE_PARAMS params;
            ZeroMemory(&params, sizeof(params));
            params.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
            params.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
            params.StateChange = enable ? DICS_ENABLE : DICS_DISABLE;
            params.Scope = DICS_FLAG_GLOBAL;
            params.HwProfile = 0;

            if (!SetupDiSetClassInstallParams(hDevInfo, &devInfoData, &params.ClassInstallHeader, sizeof(params))) {
                cout << "클래스 설치 매개변수를 설정하지 못했습니다." << endl;
            }
            if (!SetupDiCallClassInstaller(DIF_PROPERTYCHANGE, hDevInfo, &devInfoData)) {
                cout << "디바이스 상태 변경 실패" << endl;
            }
            else {
                cout << (enable ? "활성화" : "비활성화") << " 성공" << endl;
            }

            index++;
        }

        SetupDiDestroyDeviceInfoList(hDevInfo);
    }
};

int main() {
    MonitorManager manager;
    MonitorStateChanger stateChanger;
    int input;

    while (1) {
        cout << "해상도 바꾸기(1), 원래대로 복구(0): ";
        cin >> input;

        if (!manager.findPrimaryMonitor()) {
            cout << " 주 모니터를 찾지 못했습니다." << endl;
            return 1;
        }

        if (input == 1) {
            if (!manager.getCurrentResolution()) {
                cout << " 현재 해상도를 저장하지 못했습니다." << endl;
                return 1;
            }

            if (manager.changeResolution(1568, 1080)) {
                cout << " 해상도 변경 성공!" << endl;
                stateChanger.toggleMonitors(FALSE);  // 모든 모니터 비활성화
            }
            else {
                cout << " 해상도 변경 실패." << endl;
            }

        }
        else if (input == 0) {
            if (manager.restoreResolution()) {
                cout << "원래 해상도로 복구했습니다." << endl;
                stateChanger.toggleMonitors(TRUE);  // 모든 모니터 활성화
            }
            else {
                cout << "복구 실패. 이전 해상도가 저장되지 않았거나 에러 발생." << endl;
            }
        }
        else {
            cout << "잘못된 입력입니다." << endl;
        }
    }

    return 0;
}