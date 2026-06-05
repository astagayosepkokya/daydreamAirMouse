#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <cmath>
#include <windows.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Devices.Bluetooth.h>
#include <winrt/Windows.Devices.Bluetooth.GenericAttributeProfile.h>
#include <winrt/Windows.Devices.Enumeration.h>
#include <winrt/Windows.Storage.Streams.h>

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Devices::Bluetooth;
using namespace Windows::Devices::Bluetooth::GenericAttributeProfile;
using namespace Windows::Devices::Enumeration;
using namespace Windows::Storage::Streams;

const guid SERVICE_UUID{ 0x0000fe55, 0x0000, 0x1000, { 0x80, 0x00, 0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb } };
const guid CHAR_UUID{ 0x00000001, 0x1000, 0x1000, { 0x80, 0x00, 0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb } };

struct DaydreamState {
    int gyroX, gyroY, gyroZ;
    int touchX, touchY;
    bool isTouching;
    bool btnClick, btnApp, btnHome, btnVolPlus, btnVolMinus;
};

class DaydreamController {
private:
    BluetoothLEDevice device{ nullptr };
    GattCharacteristic dataChar{ nullptr };
    event_token valueChangedToken;
    event_token statusChangedToken;

    DaydreamState prevState;
    bool hasPrevState = false;
    float rxAccum = 0, ryAccum = 0;

    // Tuning & Calibration
    float sensitivity = 0.025f;
    int gOffX = 0, gOffY = 0, gOffZ = 0;
    bool isWaitingForSteady = false;
    ULONGLONG steadyStartTime = 0;
    bool isSubscribed = false;

    int GetBitsLE(const std::vector<uint8_t>& bytes, int startBit, int length) {
        int val = 0;
        for (int i = 0; i < length; i++) {
            int bitIdx = startBit + i;
            int byteIdx = bitIdx / 8;
            if (byteIdx >= (int)bytes.size()) return 0;
            if ((bytes[byteIdx] & (1 << (bitIdx % 8))) != 0) val |= (1 << i);
        }
        if (val & (1 << (length - 1))) val -= (1 << length);
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

    void SendMouseBtn(DWORD downFlag, DWORD upFlag, bool down) {
        INPUT input = {0};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = down ? downFlag : upFlag;
        SendInput(1, &input, sizeof(INPUT));
    }

    void SendKey(WORD vk, bool down) {
        INPUT input = {0};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = vk;
        input.ki.dwFlags = down ? 0 : KEYEVENTF_KEYUP;
        SendInput(1, &input, sizeof(INPUT));
    }

    void ParseData(const std::vector<uint8_t>& data) {
        if (data.size() < 20) return;
        DaydreamState state;
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
        OnStateUpdate(state);
    }

public:
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
            std::cout << "[SUCCESS] Daydream Air Mouse ACTIVE" << std::endl;
        } catch (...) {}
    }

    virtual void OnStateUpdate(const DaydreamState& state) {
        std::cout << "\033[H\033[2J"; 
        std::cout << "=== Daydream Air Mouse (Final) ===" << std::endl;
        std::cout << "Status: " << (state.isTouching ? "MOVING" : "IDLE") << std::endl;
        std::cout << "Sens  : " << std::fixed << std::setprecision(3) << sensitivity << std::endl;
        std::cout << "Map   : [Touch:Left] [Home:Right] [App:Mid]" << std::endl;
        std::cout << "-----------------------------------" << std::endl;

        if (isWaitingForSteady) {
            ULONGLONG elapsed = GetTickCount64() - steadyStartTime;
            if (elapsed < 3000) {
                std::cout << "[!] PLACE ON FLAT SURFACE... " << (3 - (elapsed / 1000)) << "s" << std::endl;
                return;
            } else {
                gOffX = state.gyroX; gOffZ = state.gyroZ; gOffY = state.gyroY;
                isWaitingForSteady = false;
                std::cout << ">>> CALIBRATED!" << std::endl;
                Beep(800, 200);
                return;
            }
        }

        if (hasPrevState) {
            // Calibration: Vol+ AND Vol-
            if (state.btnVolPlus && state.btnVolMinus) {
                isWaitingForSteady = true; steadyStartTime = GetTickCount64(); Beep(600, 100); return;
            }

            // Sensitivity Adjust: App + Vol+/-
            if (state.btnApp && state.btnVolPlus && !prevState.btnVolPlus) {
                sensitivity += 0.005f; Beep(1000, 50);
            }
            if (state.btnApp && state.btnVolMinus && !prevState.btnVolMinus) {
                sensitivity -= 0.005f; if (sensitivity < 0.005f) sensitivity = 0.005f; Beep(500, 50);
            }

            // Volume Control
            if (state.btnVolPlus && !state.btnApp && !state.btnVolMinus && !prevState.btnVolPlus) {
                keybd_event(VK_VOLUME_UP, 0, 0, 0); keybd_event(VK_VOLUME_UP, 0, KEYEVENTF_KEYUP, 0);
            }
            if (state.btnVolMinus && !state.btnApp && !state.btnVolPlus && !prevState.btnVolMinus) {
                keybd_event(VK_VOLUME_DOWN, 0, 0, 0); keybd_event(VK_VOLUME_DOWN, 0, KEYEVENTF_KEYUP, 0);
            }

            // --- SWAPPED BUTTON MAPPINGS ---
            // Touch Click -> LEFT CLICK
            if (state.btnClick != prevState.btnClick) SendMouseBtn(MOUSEEVENTF_LEFTDOWN, MOUSEEVENTF_LEFTUP, state.btnClick);
            
            // Home Button -> RIGHT CLICK
            if (state.btnHome != prevState.btnHome) SendMouseBtn(MOUSEEVENTF_RIGHTDOWN, MOUSEEVENTF_RIGHTUP, state.btnHome);
            
            // App Button (modifier handled above) -> MIDDLE CLICK
            if (!state.btnVolPlus && !state.btnVolMinus) {
               if (state.btnApp != prevState.btnApp) SendMouseBtn(MOUSEEVENTF_MIDDLEDOWN, MOUSEEVENTF_MIDDLEUP, state.btnApp);
            }

            // --- MOUSE MOVEMENT ---
            if (state.isTouching) {
                int vY = -(state.gyroZ - gOffZ);
                int vP = -(state.gyroX - gOffX);
                const int deadzone = 25; 
                if (abs(vY) > deadzone || abs(vP) > deadzone) {
                    rxAccum += (float)vY * sensitivity;
                    ryAccum += (float)vP * sensitivity;
                    int dx = (int)rxAccum; int dy = (int)ryAccum;
                    if (dx != 0 || dy != 0) {
                        INPUT input = {0}; input.type = INPUT_MOUSE; input.mi.dwFlags = MOUSEEVENTF_MOVE;
                        input.mi.dx = dx; input.mi.dy = dy; 
                        SendInput(1, &input, sizeof(INPUT));
                        rxAccum -= dx; ryAccum -= dy;
                    }
                }
            } else { rxAccum = 0; ryAccum = 0; }
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

int main() {
    init_apartment();
    DaydreamController controller;
    controller.Connect().get();
    while (!(GetAsyncKeyState(VK_ESCAPE) & 0x8000)) {
        if (!controller.IsSubscribed()) controller.TrySubscribe();
        Sleep(1000);
    }
    return 0;
}
