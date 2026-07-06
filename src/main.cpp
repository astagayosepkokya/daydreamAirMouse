#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <cmath>
#include <windows.h>
#include <shellapi.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Devices.Bluetooth.h>
#include <winrt/Windows.Devices.Bluetooth.GenericAttributeProfile.h>
#include <winrt/Windows.Devices.Enumeration.h>
#include <winrt/Windows.Storage.Streams.h>
#include <fstream>
#include <bitset>
#include <sstream>
#include "resource.h"
#pragma comment(lib, "gdi32.lib")

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Devices::Bluetooth;
using namespace Windows::Devices::Bluetooth::GenericAttributeProfile;
using namespace Windows::Devices::Enumeration;
using namespace Windows::Storage::Streams;

const guid SERVICE_UUID{ 0x0000fe55, 0x0000, 0x1000, { 0x80, 0x00, 0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb } };
const guid CHAR_UUID{ 0x00000001, 0x1000, 0x1000, { 0x80, 0x00, 0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb } };

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_EXIT 1001

struct ActionMap {
    int mod = 0;
    int key = 0;
};

struct AppConfig {
    int mouseMode = 0; 
    int volMode = 0;   
    float gyroSens = 0.025f;
    float touchSens = 1.0f;
    float touchAccel = 1.5f;    
    ActionMap padClick = {0, 0};
    ActionMap touchC = {0, 0}; 
    ActionMap touchU = {0, 3}; 
    ActionMap touchR = {0, 3}; 
    ActionMap touchD = {0, 3}; 
    ActionMap touchL = {0, 3}; 
    ActionMap home = {0, 1};  
    ActionMap app = {0, 2};   
};

AppConfig g_config;
bool g_serviceRunning = true;

void LoadConfig() {
    wchar_t path[MAX_PATH];
    GetModuleFileName(NULL, path, MAX_PATH);
    wchar_t* lastSlash = wcsrchr(path, L'\\');
    if (lastSlash) *(lastSlash + 1) = L'\0';
    wcscat_s(path, L"config.ini");

    wchar_t buf[64];
    g_config.mouseMode = GetPrivateProfileInt(L"Settings", L"MouseMode", 0, path);
    g_config.volMode = GetPrivateProfileInt(L"Settings", L"VolMode", 0, path);

    GetPrivateProfileString(L"Settings", L"GyroSens", L"0.025", buf, 64, path);
    g_config.gyroSens = std::wcstof(buf, nullptr);

    GetPrivateProfileString(L"Settings", L"TouchSens", L"1.0", buf, 64, path);
    g_config.touchSens = std::wcstof(buf, nullptr);

    GetPrivateProfileString(L"Settings", L"TouchAccel", L"1.5", buf, 64, path);
    g_config.touchAccel = std::wcstof(buf, nullptr);

    g_config.padClick.key = GetPrivateProfileInt(L"Mappings", L"PadClick", 0, path);
    g_config.padClick.mod = GetPrivateProfileInt(L"Mappings", L"PadClickMod", 0, path);
    g_config.touchC.key = GetPrivateProfileInt(L"Mappings", L"TouchC", 0, path);
    g_config.touchC.mod = GetPrivateProfileInt(L"Mappings", L"TouchCMod", 0, path);
    g_config.touchU.key = GetPrivateProfileInt(L"Mappings", L"TouchU", 3, path);
    g_config.touchU.mod = GetPrivateProfileInt(L"Mappings", L"TouchUMod", 0, path);
    g_config.touchR.key = GetPrivateProfileInt(L"Mappings", L"TouchR", 3, path);
    g_config.touchR.mod = GetPrivateProfileInt(L"Mappings", L"TouchRMod", 0, path);
    g_config.touchD.key = GetPrivateProfileInt(L"Mappings", L"TouchD", 3, path);
    g_config.touchD.mod = GetPrivateProfileInt(L"Mappings", L"TouchDMod", 0, path);
    g_config.touchL.key = GetPrivateProfileInt(L"Mappings", L"TouchL", 3, path);
    g_config.touchL.mod = GetPrivateProfileInt(L"Mappings", L"TouchLMod", 0, path);
    g_config.home.key = GetPrivateProfileInt(L"Mappings", L"Home", 1, path);
    g_config.home.mod = GetPrivateProfileInt(L"Mappings", L"HomeMod", 0, path);
    g_config.app.key = GetPrivateProfileInt(L"Mappings", L"App", 2, path);
    g_config.app.mod = GetPrivateProfileInt(L"Mappings", L"AppMod", 0, path);
}

void SaveConfig() {
    wchar_t path[MAX_PATH];
    GetModuleFileName(NULL, path, MAX_PATH);
    wchar_t* lastSlash = wcsrchr(path, L'\\');
    if (lastSlash) *(lastSlash + 1) = L'\0';
    wcscat_s(path, L"config.ini");

    wchar_t buf[64];
    swprintf_s(buf, L"%d", g_config.mouseMode);
    WritePrivateProfileString(L"Settings", L"MouseMode", buf, path);
    swprintf_s(buf, L"%d", g_config.volMode);
    WritePrivateProfileString(L"Settings", L"VolMode", buf, path);

    swprintf_s(buf, L"%.4f", g_config.gyroSens);
    WritePrivateProfileString(L"Settings", L"GyroSens", buf, path);
    swprintf_s(buf, L"%.4f", g_config.touchSens);
    WritePrivateProfileString(L"Settings", L"TouchSens", buf, path);
    swprintf_s(buf, L"%.4f", g_config.touchAccel);
    WritePrivateProfileString(L"Settings", L"TouchAccel", buf, path);

    auto saveMap = [&](const wchar_t* name, ActionMap map) {
        swprintf_s(buf, L"%d", map.key);
        WritePrivateProfileString(L"Mappings", name, buf, path);
        std::wstring modName = std::wstring(name) + L"Mod";
        swprintf_s(buf, L"%d", map.mod);
        WritePrivateProfileString(L"Mappings", modName.c_str(), buf, path);
    };

    saveMap(L"PadClick", g_config.padClick);
    saveMap(L"TouchC", g_config.touchC);
    saveMap(L"TouchU", g_config.touchU);
    saveMap(L"TouchR", g_config.touchR);
    saveMap(L"TouchD", g_config.touchD);
    saveMap(L"TouchL", g_config.touchL);
    saveMap(L"Home", g_config.home);
    saveMap(L"App", g_config.app);
}

DWORD GetMouseFlag(int action, bool down) {
    if (action == 0) return down ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
    if (action == 1) return down ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
    if (action == 2) return down ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
    return 0;
}

struct DaydreamState {
    int roll, yaw, pitch;
    int accX, accY, accZ;
    int gyroX, gyroY, gyroZ;
    int touchX, touchY;
    bool isTouching;
    bool btnClick, btnApp, btnHome, btnVolPlus, btnVolMinus;
};

DaydreamState g_latestState;
HWND g_hConfigDlg = NULL;
NOTIFYICONDATA nid = { sizeof(nid) };

#define WM_SHOW_OSD (WM_USER + 3)

HWND g_hOSDWnd = NULL;
std::wstring g_osdText;

LRESULT CALLBACK OSDWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        RECT rect;
        GetClientRect(hWnd, &rect);

        // Double buffering
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hbmMem = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
        HGDIOBJ hOldMem = SelectObject(hdcMem, hbmMem);

        // Clear with transparent color key (black)
        HBRUSH hTransBrush = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(hdcMem, &rect, hTransBrush);
        DeleteObject(hTransBrush);

        // Draw rounded rectangle filled with dark gray and a blue border
        HBRUSH hBgBrush = CreateSolidBrush(RGB(25, 25, 25));
        HGDIOBJ hOldBrush = SelectObject(hdcMem, hBgBrush);
        HPEN hBorderPen = CreatePen(PS_SOLID, 2, RGB(0, 120, 215));
        HGDIOBJ hOldPen = SelectObject(hdcMem, hBorderPen);

        RoundRect(hdcMem, rect.left + 2, rect.top + 2, rect.right - 2, rect.bottom - 2, 15, 15);

        SelectObject(hdcMem, hOldBrush);
        DeleteObject(hBgBrush);
        SelectObject(hdcMem, hOldPen);
        DeleteObject(hBorderPen);

        // Font
        HFONT hFont = CreateFont(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        HGDIOBJ hOldFont = SelectObject(hdcMem, hFont);

        SetTextColor(hdcMem, RGB(255, 255, 255));
        SetBkMode(hdcMem, TRANSPARENT);

        // Centered text
        DrawText(hdcMem, g_osdText.c_str(), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hdcMem, hOldFont);
        DeleteObject(hFont);

        BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, 0, 0, SRCCOPY);
        SelectObject(hdcMem, hOldMem);
        DeleteObject(hbmMem);
        DeleteDC(hdcMem);

        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_TIMER:
        if (wParam == 100) {
            ShowWindow(hWnd, SW_HIDE);
            KillTimer(hWnd, 100);
        }
        return 0;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
}

void TriggerOSD(const wchar_t* text) {
    g_osdText = text;
    if (!g_hOSDWnd) {
        int width = 300;
        int height = 50;
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        int x = (screenWidth - width) / 2;
        int y = 100; // Top-center position

        static bool classRegistered = false;
        HINSTANCE hInstance = GetModuleHandle(NULL);
        if (!classRegistered) {
            WNDCLASS wc = { 0 };
            wc.lpfnWndProc = OSDWndProc;
            wc.hInstance = hInstance;
            wc.lpszClassName = L"DaydreamOSDClass";
            RegisterClass(&wc);
            classRegistered = true;
        }

        g_hOSDWnd = CreateWindowEx(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_LAYERED,
            L"DaydreamOSDClass", L"Daydream OSD",
            WS_POPUP,
            x, y, width, height,
            NULL, NULL, hInstance, NULL
        );

        if (g_hOSDWnd) {
            SetLayeredWindowAttributes(g_hOSDWnd, RGB(0, 0, 0), 230, LWA_COLORKEY | LWA_ALPHA);
        }
    }

    if (g_hOSDWnd) {
        InvalidateRect(g_hOSDWnd, NULL, TRUE);
        ShowWindow(g_hOSDWnd, SW_SHOWNOACTIVATE);
        SetTimer(g_hOSDWnd, 100, 1200, NULL);
    }
}

class DaydreamController {
private:
    BluetoothLEDevice device{ nullptr };
    GattCharacteristic dataChar{ nullptr };
    event_token valueChangedToken;
    event_token statusChangedToken;

    DaydreamState prevState;
    bool hasPrevState = false;
    float rxAccum = 0, ryAccum = 0;

    int gOffX = 0, gOffY = 0, gOffZ = 0;
    bool isWaitingForSteady = false;
    ULONGLONG steadyStartTime = 0;
    bool isSubscribed = false;

    ULONGLONG lastScrollTime = 0;
    bool appComboTriggered = false;
    bool homeComboTriggered = false;

    int GetBitsLE(const std::vector<uint8_t>& bytes, int startBit, int length) {
        int val = 0;
        for (int i = 0; i < length; i++) {
            int bitIdx = startBit + i;
            int byteIdx = bitIdx / 8;
            if (byteIdx >= (int)bytes.size()) return 0;
            if ((bytes[byteIdx] & (1 << (bitIdx % 8))) != 0) val |= (1 << i);
        }
        return val;
    }

    int GetBitsBE(const std::vector<uint8_t>& bytes, int startBit, int length) {
        int val = 0;
        for (int i = 0; i < length; i++) {
            int bitIdx = startBit + i;
            int byteIdx = bitIdx / 8;
            if (byteIdx >= (int)bytes.size()) return 0;
            if ((bytes[byteIdx] & (1 << (7 - (bitIdx % 8)))) != 0) val |= (1 << (length - 1 - i));
        }
        if (val & (1 << (length - 1))) val -= (1 << length);
        return val;
    }

    void SendMouseBtn(int action, bool down) {
        DWORD flag = GetMouseFlag(action, down);
        if (flag == 0) return;
        INPUT input = {0};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = flag;
        SendInput(1, &input, sizeof(INPUT));
    }

    void SendAction(ActionMap act, bool down) {
        if (act.key == 0 && act.mod == 0) return;
        if (down) {
            if (act.mod != 0) SendSingle(act.mod, true);
            if (act.key != 0) SendSingle(act.key, true);
        } else {
            if (act.key != 0) SendSingle(act.key, false);
            if (act.mod != 0) SendSingle(act.mod, false);
        }
    }

    void SendSingle(int vk, bool down) {
        if (vk == 0) return; // None
        if (vk == VK_LBUTTON) { SendMouseBtn(0, down); return; }
        if (vk == VK_RBUTTON) { SendMouseBtn(1, down); return; }
        if (vk == VK_MBUTTON) { SendMouseBtn(2, down); return; }
        if (vk == VK_XBUTTON1) { SendMouseBtn(3, down); return; } // just mapping 3 to X1 if needed
        SendKey(vk, down);
    }

    void SendKey(WORD vk, bool down) {
        INPUT input = {0};
        input.type = INPUT_KEYBOARD;
        
        UINT scancode = MapVirtualKey(vk, MAPVK_VK_TO_VSC);
        input.ki.wScan = scancode;
        input.ki.wVk = vk; // Some apps prefer both
        
        input.ki.dwFlags = KEYEVENTF_SCANCODE | (down ? 0 : KEYEVENTF_KEYUP);
        
        switch (vk) {
            case VK_LEFT: case VK_UP: case VK_RIGHT: case VK_DOWN:
            case VK_PRIOR: case VK_NEXT: case VK_END: case VK_HOME:
            case VK_INSERT: case VK_DELETE: case VK_DIVIDE: case VK_NUMLOCK:
                input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
                break;
        }

        SendInput(1, &input, sizeof(INPUT));
    }

    void SendScroll(int amount) {
        INPUT input = {0};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_WHEEL;
        input.mi.mouseData = (DWORD)amount;
        SendInput(1, &input, sizeof(INPUT));
    }

    void ParseData(const std::vector<uint8_t>& data) {
        if (data.size() < 20) return;
        DaydreamState state;
        
        state.roll  = GetBitsBE(data, 16, 13);
        state.yaw   = GetBitsBE(data, 29, 13);
        state.pitch = GetBitsBE(data, 42, 13);
        
        state.accX  = GetBitsBE(data, 55, 13);
        state.accZ  = GetBitsBE(data, 68, 13);
        state.accY  = GetBitsBE(data, 81, 13);

        state.gyroX = GetBitsBE(data, 94, 13);
        state.gyroZ = GetBitsBE(data, 107, 13);
        state.gyroY = GetBitsBE(data, 120, 13);

        state.touchX = GetBitsLE(data, 128, 5); 
        state.touchY = GetBitsLE(data, 136, 5);
        state.isTouching = (state.touchX != 0 || state.touchY != 0);
        
        state.btnClick = (GetBitsLE(data, 144, 1) != 0);
        state.btnHome  = (GetBitsLE(data, 145, 1) != 0);
        state.btnApp   = (GetBitsLE(data, 146, 1) != 0);
        state.btnVolMinus = (GetBitsLE(data, 147, 1) != 0);
        state.btnVolPlus  = (GetBitsLE(data, 148, 1) != 0);

        g_latestState = state;
        if (g_serviceRunning) OnStateUpdate(state);
    }

public:
    void StartManualCalibration() {
        MessageBox(NULL, L"Please place the controller on a FLAT SURFACE and keep it steady.\n\nClick OK to start.", L"Sensor Calibration", MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TOPMOST);
        isWaitingForSteady = true;
        steadyStartTime = GetTickCount64();
        Beep(600, 100);
    }

    IAsyncAction TrySubscribe() {
        if (isSubscribed) co_return;
        try {
            auto sResult = co_await device.GetGattServicesForUuidAsync(SERVICE_UUID);
            if (sResult.Status() != GattCommunicationStatus::Success || sResult.Services().Size() == 0) co_return;
            auto service = sResult.Services().GetAt(0);
            auto cResult = co_await service.GetCharacteristicsForUuidAsync(CHAR_UUID);
            if (cResult.Status() != GattCommunicationStatus::Success || cResult.Characteristics().Size() == 0) co_return;
            dataChar = cResult.Characteristics().GetAt(0);
            valueChangedToken = dataChar.ValueChanged([this](auto&&, GattValueChangedEventArgs const& args) {
                auto reader = DataReader::FromBuffer(args.CharacteristicValue());
                std::vector<uint8_t> data(reader.UnconsumedBufferLength());
                reader.ReadBytes(data);
                ParseData(data);
            });
            co_await dataChar.WriteClientCharacteristicConfigurationDescriptorAsync(GattClientCharacteristicConfigurationDescriptorValue::Notify);
            isSubscribed = true;
        } catch (...) {}
    }

    virtual void OnStateUpdate(const DaydreamState& state) {
        if (isWaitingForSteady) {
            ULONGLONG elapsed = GetTickCount64() - steadyStartTime;
            if (elapsed < 3000) return;
            gOffX = state.gyroX; gOffZ = state.gyroZ; gOffY = state.gyroY;
            isWaitingForSteady = false;
            Beep(800, 200);
            MessageBox(NULL, L"Calibration Successful!", L"Success", MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TOPMOST);
            return;
        }

        if (hasPrevState) {
            // Track when buttons are first pressed down to reset flags
            if (state.btnApp && !prevState.btnApp) appComboTriggered = false;
            if (state.btnHome && !prevState.btnHome) homeComboTriggered = false;

            // Mode Toggle: App + Home
            if (state.btnApp && state.btnHome && !prevState.btnHome) {
                g_config.mouseMode = (g_config.mouseMode == 0) ? 1 : 0;
                SaveConfig();
                Beep(g_config.mouseMode == 1 ? 1200 : 800, 150);
                PostMessage(nid.hWnd, WM_SHOW_OSD, g_config.mouseMode, 0);
                appComboTriggered = true;
                homeComboTriggered = true;
            }

            // Sensitivity Adjust: App + Vol+/-
            if (state.btnApp && state.btnVolPlus && !prevState.btnVolPlus) {
                if (g_config.mouseMode == 0) {
                    g_config.gyroSens += 0.005f;
                    PostMessage(nid.hWnd, WM_SHOW_OSD, 2, 0);
                } else {
                    g_config.touchSens += 0.1f;
                    PostMessage(nid.hWnd, WM_SHOW_OSD, 3, 0);
                }
                Beep(1000, 50); SaveConfig();
                appComboTriggered = true;
            }
            if (state.btnApp && state.btnVolMinus && !prevState.btnVolMinus) {
                if (g_config.mouseMode == 0) {
                    g_config.gyroSens -= 0.005f; if (g_config.gyroSens < 0.005f) g_config.gyroSens = 0.005f;
                    PostMessage(nid.hWnd, WM_SHOW_OSD, 2, 0);
                } else {
                    g_config.touchSens -= 0.1f; if (g_config.touchSens < 0.1f) g_config.touchSens = 0.1f;
                    PostMessage(nid.hWnd, WM_SHOW_OSD, 3, 0);
                }
                Beep(500, 50); SaveConfig();
                appComboTriggered = true;
            }

            // Volume/Scroll/Page Mapping
            if (state.btnVolPlus && !state.btnApp && !prevState.btnVolPlus) {
                if (g_config.volMode == 0) { SendKey(VK_VOLUME_UP, true); SendKey(VK_VOLUME_UP, false); }
                else if (g_config.volMode == 1) { SendScroll(120); }
                else if (g_config.volMode == 2) { SendKey(VK_PRIOR, true); SendKey(VK_PRIOR, false); }
            }
            if (state.btnVolMinus && !state.btnApp && !prevState.btnVolMinus) {
                if (g_config.volMode == 0) { SendKey(VK_VOLUME_DOWN, true); SendKey(VK_VOLUME_DOWN, false); }
                else if (g_config.volMode == 1) { SendScroll(-120); }
                else if (g_config.volMode == 2) { SendKey(VK_NEXT, true); SendKey(VK_NEXT, false); }
            }

            // Touchpad Physical Click
            static ULONGLONG clickHoldStartTime = 0;
            static ULONGLONG clickLastRepeatTime = 0;
            if (state.btnClick != prevState.btnClick) {
                SendAction(g_config.padClick, state.btnClick);
                if (state.btnClick) {
                    clickHoldStartTime = GetTickCount64();
                    clickLastRepeatTime = clickHoldStartTime;
                }
            } else if (state.btnClick) {
                ActionMap act = g_config.padClick;
                bool isKey = (act.key != 0 && act.key != VK_LBUTTON && act.key != VK_RBUTTON && act.key != VK_MBUTTON && act.key != VK_XBUTTON1 && act.key != VK_XBUTTON2) ||
                             (act.mod != 0 && act.mod != VK_LBUTTON && act.mod != VK_RBUTTON && act.mod != VK_MBUTTON && act.mod != VK_XBUTTON1 && act.mod != VK_XBUTTON2);
                if (isKey) {
                    ULONGLONG now = GetTickCount64();
                    if (now - clickHoldStartTime > 400) {
                        if (now - clickLastRepeatTime > 33) {
                            SendAction(act, true);
                            clickLastRepeatTime = now;
                        }
                    }
                }
            }

            // Touch Area Activation
            static int activeTouchArea = -1;
            int currentTouchArea = -1;
            
            if (state.isTouching) {
                float dx = state.touchX - 15.5f;
                float dy = state.touchY - 15.5f;
                float distSq = dx*dx + dy*dy;
                if (distSq < 40.0f) currentTouchArea = 0;
                else {
                    float angle = atan2(dy, dx) * 180.0f / 3.14159f;
                    if (angle > -45 && angle <= 45) currentTouchArea = 2; // R
                    else if (angle > 45 && angle <= 135) currentTouchArea = 3; // D
                    else if (angle > -135 && angle <= -45) currentTouchArea = 1; // U
                    else currentTouchArea = 4; // L
                }
            }

            static ULONGLONG holdStartTime = 0;
            static ULONGLONG lastRepeatTime = 0;

            auto getTouchMap = [](int area) -> ActionMap {
                if (area == 0) return g_config.touchC;
                if (area == 1) return g_config.touchU;
                if (area == 2) return g_config.touchR;
                if (area == 3) return g_config.touchD;
                if (area == 4) return g_config.touchL;
                return {0, 0};
            };

            if (currentTouchArea != activeTouchArea) {
                if (activeTouchArea != -1) {
                    SendAction(getTouchMap(activeTouchArea), false);
                }
                if (currentTouchArea != -1) {
                    SendAction(getTouchMap(currentTouchArea), true);
                    holdStartTime = GetTickCount64();
                    lastRepeatTime = holdStartTime;
                }
                activeTouchArea = currentTouchArea;
            } else if (activeTouchArea != -1) {
                // Auto-repeat for keyboard keys
                ActionMap act = getTouchMap(activeTouchArea);
                bool isKey = (act.key != 0 && act.key != VK_LBUTTON && act.key != VK_RBUTTON && act.key != VK_MBUTTON && act.key != VK_XBUTTON1 && act.key != VK_XBUTTON2) ||
                             (act.mod != 0 && act.mod != VK_LBUTTON && act.mod != VK_RBUTTON && act.mod != VK_MBUTTON && act.mod != VK_XBUTTON1 && act.mod != VK_XBUTTON2);
                if (isKey) {
                    ULONGLONG now = GetTickCount64();
                    if (now - holdStartTime > 400) { 
                        if (now - lastRepeatTime > 33) { 
                            SendAction(act, true);
                            lastRepeatTime = now;
                        }
                    }
                }
            }
            
            // Home button release triggers normal action if it wasn't part of a combo
            if (!state.btnHome && prevState.btnHome) {
                if (!homeComboTriggered && !state.btnApp) {
                    SendAction(g_config.home, true);
                    SendAction(g_config.home, false);
                }
            }
            
            // App button release triggers normal action if it wasn't part of a combo
            if (!state.btnApp && prevState.btnApp) {
                if (!appComboTriggered && !state.btnVolPlus && !state.btnVolMinus && !state.btnHome) {
                    SendAction(g_config.app, true);
                    SendAction(g_config.app, false);
                }
            }

            int dx = 0, dy = 0;
            if (g_config.mouseMode == 0) {
                int vY = -(state.gyroZ - gOffZ);
                int vP = -(state.gyroX - gOffX);
                const int deadzone = 25; 
                if (abs(vY) > deadzone || abs(vP) > deadzone) {
                    rxAccum += (float)vY * g_config.gyroSens;
                    ryAccum += (float)vP * g_config.gyroSens;
                    dx = (int)rxAccum; dy = (int)ryAccum;
                    rxAccum -= dx; ryAccum -= dy;
                } else { rxAccum = 0; ryAccum = 0; }
            } else {
                if (state.isTouching && prevState.isTouching) {
                    float tdx = (float)(state.touchX - prevState.touchX);
                    float tdy = (float)(state.touchY - prevState.touchY);
                    if (abs(tdx) < 20 && abs(tdy) < 20) {
                        float dist = sqrt(tdx*tdx + tdy*tdy);
                        if (dist > 0.5f) {
                            float factor = pow(dist, g_config.touchAccel - 1.0f);
                            rxAccum += tdx * g_config.touchSens * factor;
                            ryAccum += tdy * g_config.touchSens * factor;
                            dx = (int)rxAccum; dy = (int)ryAccum;
                            rxAccum -= dx; ryAccum -= dy;
                        }
                    }
                } else { rxAccum = 0; ryAccum = 0; }
            }

            if (dx != 0 || dy != 0) {
                INPUT input = {0}; input.type = INPUT_MOUSE; input.mi.dwFlags = MOUSEEVENTF_MOVE;
                input.mi.dx = dx; input.mi.dy = dy; 
                SendInput(1, &input, sizeof(INPUT));
            }
        }
        prevState = state;
        hasPrevState = true;
    }

    IAsyncAction Connect() {
        hstring selector = BluetoothLEDevice::GetDeviceSelector();
        auto devices = co_await DeviceInformation::FindAllAsync(selector);
        std::wstring id;
        for (auto&& dev : devices) if (std::wstring_view(dev.Name()).find(L"Daydream") != std::wstring_view::npos) id = dev.Id().c_str();
        if (id.empty()) co_return;
        device = co_await BluetoothLEDevice::FromIdAsync(id);
        if (!device) co_return;
        statusChangedToken = device.ConnectionStatusChanged([this](BluetoothLEDevice const& sender, auto&&) {
            if (sender.ConnectionStatus() == BluetoothConnectionStatus::Connected) TrySubscribe();
            else isSubscribed = false;
        });
        if (device.ConnectionStatus() == BluetoothConnectionStatus::Connected) TrySubscribe();
    }
    bool IsSubscribed() { return isSubscribed; }
};

DaydreamController controller;

int g_mappingTarget = 0;
HHOOK g_hKeyboardHook = NULL;
HHOOK g_hMouseHook = NULL;
int g_mappedMod = 0;
int g_mappedKey = 0;

const wchar_t* GetKeyName(int vk) {
    if (vk == 0) return L"None";
    if (vk == VK_LBUTTON) return L"LClick";
    if (vk == VK_RBUTTON) return L"RClick";
    if (vk == VK_MBUTTON) return L"MClick";
    if (vk == VK_XBUTTON1) return L"X1Click";
    if (vk == VK_XBUTTON2) return L"X2Click";
    static wchar_t name[64];
    UINT scanCode = MapVirtualKey(vk, MAPVK_VK_TO_VSC);
    switch (vk) {
        case VK_LEFT: case VK_UP: case VK_RIGHT: case VK_DOWN:
        case VK_PRIOR: case VK_NEXT: case VK_END: case VK_HOME:
        case VK_INSERT: case VK_DELETE: case VK_DIVIDE: case VK_NUMLOCK:
            scanCode |= KF_EXTENDED; break;
    }
    if (GetKeyNameText(scanCode << 16, name, 64) == 0) swprintf_s(name, L"Key:0x%X", vk);
    return name;
}

std::wstring GetComboName(ActionMap act) {
    if (act.mod == 0 && act.key == 0) return L"None";
    if (act.mod == 0) return GetKeyName(act.key);
    if (act.key == 0) return GetKeyName(act.mod);
    return std::wstring(GetKeyName(act.mod)) + L" + " + GetKeyName(act.key);
}

void CompleteMapping(int mod, int key) {
    ActionMap act = {mod, key};
    if (g_mappingTarget == IDC_BTN_TOUCH_C) g_config.touchC = act;
    else if (g_mappingTarget == IDC_BTN_TOUCH_U) g_config.touchU = act;
    else if (g_mappingTarget == IDC_BTN_TOUCH_R) g_config.touchR = act;
    else if (g_mappingTarget == IDC_BTN_TOUCH_D) g_config.touchD = act;
    else if (g_mappingTarget == IDC_BTN_TOUCH_L) g_config.touchL = act;
    else if (g_mappingTarget == IDC_BTN_TOUCH_CLICK) g_config.padClick = act;
    else if (g_mappingTarget == IDC_BTN_HOME) g_config.home = act;
    else if (g_mappingTarget == IDC_BTN_APP) g_config.app = act;
    
    SetDlgItemText(g_hConfigDlg, g_mappingTarget, GetComboName(act).c_str());
    if (g_hKeyboardHook) { UnhookWindowsHookEx(g_hKeyboardHook); g_hKeyboardHook = NULL; }
    if (g_hMouseHook) { UnhookWindowsHookEx(g_hMouseHook); g_hMouseHook = NULL; }
    g_mappingTarget = 0;
}

void HandleHookInput(int vk, bool down) {
    if (vk == VK_ESCAPE) {
        CompleteMapping(0, 0); // Escape clears
        return;
    }
    if (down) {
        if (g_mappedMod == 0) g_mappedMod = vk;
        else if (g_mappedKey == 0 && vk != g_mappedMod) {
            g_mappedKey = vk;
            CompleteMapping(g_mappedMod, g_mappedKey);
        }
    } else {
        if (g_mappedMod == vk && g_mappedKey == 0) {
            CompleteMapping(0, g_mappedMod); 
        }
    }
}

LRESULT CALLBACK CatchKeyKeyboardHook(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) HandleHookInput(p->vkCode, true);
        else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) HandleHookInput(p->vkCode, false);
        return 1;
    }
    return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
}

LRESULT CALLBACK CatchKeyMouseHook(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        int vk = 0;
        if (wParam == WM_LBUTTONDOWN || wParam == WM_LBUTTONUP) vk = VK_LBUTTON;
        else if (wParam == WM_RBUTTONDOWN || wParam == WM_RBUTTONUP) vk = VK_RBUTTON;
        else if (wParam == WM_MBUTTONDOWN || wParam == WM_MBUTTONUP) vk = VK_MBUTTON;
        else if (wParam == WM_XBUTTONDOWN || wParam == WM_XBUTTONUP) {
            MSLLHOOKSTRUCT* p = (MSLLHOOKSTRUCT*)lParam;
            vk = (HIWORD(p->mouseData) == XBUTTON1) ? VK_XBUTTON1 : VK_XBUTTON2;
        }
        if (vk != 0) {
            bool down = (wParam == WM_LBUTTONDOWN || wParam == WM_RBUTTONDOWN || wParam == WM_MBUTTONDOWN || wParam == WM_XBUTTONDOWN);
            HandleHookInput(vk, down);
            return 1;
        }
    }
    return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
}

void StartMapping(HWND hwnd, int btnId) {
    if (g_hKeyboardHook) { UnhookWindowsHookEx(g_hKeyboardHook); g_hKeyboardHook = NULL; }
    if (g_hMouseHook) { UnhookWindowsHookEx(g_hMouseHook); g_hMouseHook = NULL; }
    
    g_mappingTarget = btnId;
    g_mappedMod = 0;
    g_mappedKey = 0;
    SetDlgItemText(hwnd, btnId, L"Press key(s)...");
    g_hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, CatchKeyKeyboardHook, GetModuleHandle(NULL), 0);
    g_hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, CatchKeyMouseHook, GetModuleHandle(NULL), 0);
}

INT_PTR CALLBACK ConfigDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    const wchar_t* modeOptions[] = { L"Gyroscope", L"Touchpad" };
    const wchar_t* volOptions[] = { L"Volume Up/Down", L"Scroll Up/Down", L"Page Up/Down" };
    switch (msg) {
    case WM_INITDIALOG: {
        g_hConfigDlg = hwnd;
        wchar_t buf[64];
        swprintf_s(buf, L"%.3f", g_config.gyroSens);
        SetDlgItemText(hwnd, IDC_GYRO_SENS_EDIT, buf);
        swprintf_s(buf, L"%.3f", g_config.touchSens);
        SetDlgItemText(hwnd, IDC_TOUCH_SENS_EDIT, buf);
        swprintf_s(buf, L"%.3f", g_config.touchAccel);
        SetDlgItemText(hwnd, IDC_TOUCH_ACCEL_EDIT, buf);
        SetDlgItemText(hwnd, IDC_TOGGLE_SERVICE_BTN, g_serviceRunning ? L"Stop Service" : L"Start Service");

        HWND hMode = GetDlgItem(hwnd, IDC_MOUSE_MODE_COMBO);
        for (auto opt : modeOptions) SendMessage(hMode, CB_ADDSTRING, 0, (LPARAM)opt);
        SendMessage(hMode, CB_SETCURSEL, g_config.mouseMode, 0);

        HWND hVol = GetDlgItem(hwnd, IDC_VOL_MODE_COMBO);
        for (auto opt : volOptions) SendMessage(hVol, CB_ADDSTRING, 0, (LPARAM)opt);
        SendMessage(hVol, CB_SETCURSEL, g_config.volMode, 0);

        SetDlgItemText(hwnd, IDC_BTN_TOUCH_C, GetComboName(g_config.touchC).c_str());
        SetDlgItemText(hwnd, IDC_BTN_TOUCH_U, GetComboName(g_config.touchU).c_str());
        SetDlgItemText(hwnd, IDC_BTN_TOUCH_R, GetComboName(g_config.touchR).c_str());
        SetDlgItemText(hwnd, IDC_BTN_TOUCH_D, GetComboName(g_config.touchD).c_str());
        SetDlgItemText(hwnd, IDC_BTN_TOUCH_L, GetComboName(g_config.touchL).c_str());
        SetDlgItemText(hwnd, IDC_BTN_TOUCH_CLICK, GetComboName(g_config.padClick).c_str());
        SetDlgItemText(hwnd, IDC_BTN_HOME, GetComboName(g_config.home).c_str());
        SetDlgItemText(hwnd, IDC_BTN_APP, GetComboName(g_config.app).c_str());
        
        SetTimer(hwnd, 2, 100, NULL);
        return (INT_PTR)TRUE;
    }
    case WM_TIMER:
        if (wParam == 2) {
            std::wstringstream ss;
            ss << L"Mode: " << (g_config.mouseMode == 0 ? L"GYRO" : L"TOUCH") << L" | " << (g_serviceRunning ? L"ACTIVE" : L"IDLE") << L"\n";
            ss << L"Orient: R:" << (g_latestState.roll/100*100) << L" Y:" << (g_latestState.yaw/100*100) << L" P:" << (g_latestState.pitch/100*100) << L"\n";
            ss << L"Accel: X:" << (g_latestState.accX/100*100) << L" Z:" << (g_latestState.accZ/100*100) << L" Y:" << (g_latestState.accY/100*100);
            SetDlgItemText(hwnd, IDC_RAW_DATA_STATIC, ss.str().c_str());

            HWND hMode = GetDlgItem(hwnd, IDC_MOUSE_MODE_COMBO);
            if (hMode) {
                int curSel = (int)SendMessage(hMode, CB_GETCURSEL, 0, 0);
                if (curSel != g_config.mouseMode) {
                    SendMessage(hMode, CB_SETCURSEL, g_config.mouseMode, 0);
                }
            }
        }
        break;
    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_MINIMIZE) {
            ShowWindow(hwnd, SW_HIDE);
            return (INT_PTR)TRUE;
        }
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_BTN_TOUCH_C || LOWORD(wParam) == IDC_BTN_TOUCH_U || LOWORD(wParam) == IDC_BTN_TOUCH_R || 
            LOWORD(wParam) == IDC_BTN_TOUCH_D || LOWORD(wParam) == IDC_BTN_TOUCH_L || LOWORD(wParam) == IDC_BTN_TOUCH_CLICK || 
            LOWORD(wParam) == IDC_BTN_HOME || LOWORD(wParam) == IDC_BTN_APP) {
            StartMapping(hwnd, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        if (LOWORD(wParam) == IDC_TOGGLE_SERVICE_BTN) {
            g_serviceRunning = !g_serviceRunning;
            SetDlgItemText(hwnd, IDC_TOGGLE_SERVICE_BTN, g_serviceRunning ? L"Stop Service" : L"Start Service");
            return (INT_PTR)TRUE;
        }
        if (LOWORD(wParam) == IDC_SAVE_BTN) {
            wchar_t buf[64];
            g_config.mouseMode = (int)SendMessage(GetDlgItem(hwnd, IDC_MOUSE_MODE_COMBO), CB_GETCURSEL, 0, 0);
            g_config.volMode = (int)SendMessage(GetDlgItem(hwnd, IDC_VOL_MODE_COMBO), CB_GETCURSEL, 0, 0);
            GetDlgItemText(hwnd, IDC_GYRO_SENS_EDIT, buf, 64);
            g_config.gyroSens = std::wcstof(buf, nullptr);
            GetDlgItemText(hwnd, IDC_TOUCH_SENS_EDIT, buf, 64);
            g_config.touchSens = std::wcstof(buf, nullptr);
            GetDlgItemText(hwnd, IDC_TOUCH_ACCEL_EDIT, buf, 64);
            g_config.touchAccel = std::wcstof(buf, nullptr);
            
            // VKs are already updated by hooks directly into g_config
            SaveConfig();
            TriggerOSD(L"Settings Saved");
            return (INT_PTR)TRUE;
        }
        if (LOWORD(wParam) == IDC_CANCEL_BTN) {
            KillTimer(hwnd, 2);
            g_hConfigDlg = NULL;
            EndDialog(hwnd, IDCANCEL);
            Shell_NotifyIcon(NIM_DELETE, &nid);
            if (g_hOSDWnd) DestroyWindow(g_hOSDWnd);
            PostQuitMessage(0);
            return (INT_PTR)TRUE;
        }
        if (LOWORD(wParam) == IDC_CALIBRATE_BTN) {
            controller.StartManualCalibration();
            return (INT_PTR)TRUE;
        }
        break;
    case WM_CLOSE:
        if (g_hKeyboardHook) { UnhookWindowsHookEx(g_hKeyboardHook); g_hKeyboardHook = NULL; }
        if (g_hMouseHook) { UnhookWindowsHookEx(g_hMouseHook); g_hMouseHook = NULL; }
        g_mappingTarget = 0;
        KillTimer(hwnd, 2);
        g_hConfigDlg = NULL;
        EndDialog(hwnd, IDCANCEL);
        Shell_NotifyIcon(NIM_DELETE, &nid);
        if (g_hOSDWnd) DestroyWindow(g_hOSDWnd);
        PostQuitMessage(0);
        return (INT_PTR)TRUE;
    }
    return (INT_PTR)FALSE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_TRAYICON) {
        if (LOWORD(lParam) == WM_LBUTTONUP) {
            PostMessage(hWnd, WM_COMMAND, ID_TRAY_SETTINGS, 0);
        } else if (LOWORD(lParam) == WM_RBUTTONUP) {
            POINT pt; GetCursorPos(&pt);
            HMENU hMenu = CreatePopupMenu();
            AppendMenu(hMenu, MF_STRING, ID_TRAY_SETTINGS, L"Settings...");
            AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenu(hMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");
            SetForegroundWindow(hWnd);
            TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);
            DestroyMenu(hMenu);
        }
    } else if (message == WM_SHOW_OSD) {
        int mode = (int)wParam;
        wchar_t buf[128];
        if (mode == 0) {
            TriggerOSD(L"Mode: Gyroscope");
        } else if (mode == 1) {
            TriggerOSD(L"Mode: Touchpad");
        } else if (mode == 2) {
            swprintf_s(buf, L"Gyro Sens: %.3f", g_config.gyroSens);
            TriggerOSD(buf);
        } else if (mode == 3) {
            swprintf_s(buf, L"Touch Sens: %.2f", g_config.touchSens);
            TriggerOSD(buf);
        }
    } else if (message == WM_COMMAND) {
        if (LOWORD(wParam) == ID_TRAY_EXIT) {
            Shell_NotifyIcon(NIM_DELETE, &nid);
            if (g_hOSDWnd) DestroyWindow(g_hOSDWnd);
            if (g_hConfigDlg) EndDialog(g_hConfigDlg, IDCANCEL);
            PostQuitMessage(0);
        } else if (LOWORD(wParam) == ID_TRAY_SETTINGS) {
            if (g_hConfigDlg) {
                ShowWindow(g_hConfigDlg, SW_SHOW);
                SetForegroundWindow(g_hConfigDlg);
            } else {
                DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_CONFIG_DIALOG), hWnd, ConfigDlgProc);
            }
        }
    } else if (message == WM_DESTROY) {
        Shell_NotifyIcon(NIM_DELETE, &nid);
        if (g_hOSDWnd) DestroyWindow(g_hOSDWnd);
        if (g_hConfigDlg) EndDialog(g_hConfigDlg, IDCANCEL);
        PostQuitMessage(0);
    } else if (message == WM_TIMER) {
        if (wParam == 1 && !controller.IsSubscribed()) controller.TrySubscribe();
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    HANDLE hMutex = CreateMutex(NULL, TRUE, L"Global\\DaydreamAirMouseMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBox(NULL, L"Daydream Air Mouse is already running in the system tray.", L"Already Running", MB_OK | MB_ICONINFORMATION);
        return 0;
    }

    init_apartment();
    LoadConfig();

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"DaydreamTrayClass";
    RegisterClass(&wc);

    HWND hWnd = CreateWindow(L"DaydreamTrayClass", L"Daydream Controller", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);

    nid.hWnd = hWnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(101));
    wcscpy_s(nid.szTip, L"Daydream Controller");
    Shell_NotifyIcon(NIM_ADD, &nid);

    controller.Connect();
    SetTimer(hWnd, 1, 5000, NULL);

    DialogBox(hInstance, MAKEINTRESOURCE(IDD_CONFIG_DIALOG), hWnd, ConfigDlgProc);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (hMutex) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }
    return (int)msg.wParam;
}
