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
            cout << " ���� ���÷��� ������ �ҷ����� ���߽��ϴ�." << endl;
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
            cout << "��ġ ������ �������� ���߽��ϴ�." << endl;
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
                cout << "Ŭ���� ��ġ �Ű������� �������� ���߽��ϴ�." << endl;
            }
            if (!SetupDiCallClassInstaller(DIF_PROPERTYCHANGE, hDevInfo, &devInfoData)) {
                cout << "����̽� ���� ���� ����" << endl;
            }
            else {
                cout << (enable ? "Ȱ��ȭ" : "��Ȱ��ȭ") << " ����" << endl;
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
        cout << "�ػ� �ٲٱ�(1), ������� ����(0): ";
        cin >> input;

        if (!manager.findPrimaryMonitor()) {
            cout << " �� ����͸� ã�� ���߽��ϴ�." << endl;
            return 1;
        }

        if (input == 1) {
            if (!manager.getCurrentResolution()) {
                cout << " ���� �ػ󵵸� �������� ���߽��ϴ�." << endl;
                return 1;
            }

            if (manager.changeResolution(1568, 1080)) {
                cout << " �ػ� ���� ����!" << endl;
                stateChanger.toggleMonitors(FALSE);  // ��� ����� ��Ȱ��ȭ
            }
            else {
                cout << " �ػ� ���� ����." << endl;
            }

        }
        else if (input == 0) {
            if (manager.restoreResolution()) {
                cout << "���� �ػ󵵷� �����߽��ϴ�." << endl;
                stateChanger.toggleMonitors(TRUE);  // ��� ����� Ȱ��ȭ
            }
            else {
                cout << "���� ����. ���� �ػ󵵰� ������� �ʾҰų� ���� �߻�." << endl;
            }
        }
        else {
            cout << "�߸��� �Է��Դϴ�." << endl;
        }
    }

    return 0;
}