#include <iostream>
#include <endpointvolume.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>

using std::string;
using std::wstring;

#define NUMOFCHANNELS 2
#define NUMOFDIODES 50

#define REQUIRESCOM FALSE

// Release version sometimes has a problem with ANSI coding, so use this command in Command Prompt to enable it
// REG ADD HKCU\CONSOLE /f /v VirtualTerminalLevel /t REG_DWORD /d 1

// Requires at leat one sound in the target application ! ! !

void printVolume(string _name, int _lChannel, int _rChannel) {
    printf("\r\033[93mDisplay name: \033[92m%s\n\r\033[96m[", _name.c_str());
    
    for (int i = 0; i <= NUMOFDIODES; i++)
        (i < _lChannel) ? printf("=") : printf(" ");
    printf("] \033[93mLeft channel : ");
    _lChannel < 10 ? printf("\033[92m %d\n\r\033[91m[", _lChannel) : printf("\033[92m%d\n\r\033[91m[", _lChannel);
    
    for (int i = 0; i <= NUMOFDIODES; i++)
        (i < _rChannel) ? printf("=") : printf(" ");
    printf("] \033[93mRight channel : ");
    _rChannel < 10 ? printf("\033[92m %d\n\r\033[91m", _rChannel) : printf("\033[92m%d\n\r\033[91m", _rChannel);

    for (int i = 0; i < 3; i++)
        printf("\x1b[A");
}

void writeData(HANDLE _serial, int _leftChannel, int _rightChannel) {
    char lChannel = 'L';
    char rChannel = 'R';

    WriteFile(_serial, &rChannel, sizeof(rChannel), NULL, NULL);
    WriteFile(_serial, &_rightChannel, sizeof(_rightChannel), NULL, NULL);
    WriteFile(_serial, &lChannel, sizeof(lChannel), NULL, NULL);
    WriteFile(_serial, &_leftChannel, sizeof(_leftChannel), NULL, NULL);
}

int serialCommunication(HANDLE* _serial) {    

    *_serial = CreateFileA("\\\\.\\COM17", GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (*_serial == INVALID_HANDLE_VALUE) {
        if (GetLastError() == ERROR_FILE_NOT_FOUND) printf("Serial does not exist\n");
        else printf("Couldn't access serial port\n");
        CloseHandle(*_serial);
        return 0;
    }

    DCB dcbSerialParams = { 0 };
    
    if (!GetCommState(*_serial, &dcbSerialParams)) {
        printf("Error getting state\n");
        CloseHandle(*_serial);
        return 0;
    }

    dcbSerialParams.BaudRate = 9600;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = TWOSTOPBITS;
    dcbSerialParams.Parity = NOPARITY;

    if (!SetCommState(*_serial, &dcbSerialParams)) {
        printf("Error setting state\n");
        CloseHandle(*_serial);
        return 0;
    }

    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(*_serial, &timeouts)) {
        printf("Error setting timeouts\n");
        CloseHandle(*_serial);
        return 0;
    }

    return 1;
}

void getVolumeLevel(HANDLE _serial) {
    IMMDeviceEnumerator* pDeviceEnumerator;
    IMMDevice* pDevice;
    IAudioSessionManager2* pAudioSessionManager2;
    IAudioSessionEnumerator* pAudioSessionEnumerator;

    CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID*)&pDeviceEnumerator);
    pDeviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);

    pDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, NULL, (void**)&pAudioSessionManager2);
    pAudioSessionManager2->GetSessionEnumerator(&pAudioSessionEnumerator);

    pDevice->Release();
    pDevice = NULL;

    int nSessionCount;
    pAudioSessionEnumerator->GetCount(&nSessionCount);
    int refreshCooldown = 0;

    while (true) {
        for (int nSessionIndex = 0; nSessionIndex < nSessionCount; nSessionIndex++) {
            IAudioSessionControl* pSessionControl;
            IAudioSessionControl2* pSessionControl2;
            if (FAILED(pAudioSessionEnumerator->GetSession(nSessionIndex, &pSessionControl)))
                continue;            

            pAudioSessionEnumerator->GetSession(nSessionIndex, &pSessionControl);
            pSessionControl->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&pSessionControl2);

            DWORD processID;
            pSessionControl2->GetProcessId(&processID);

            HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS | PROCESS_QUERY_INFORMATION |
                PROCESS_VM_READ,
                FALSE, processID);

            string processName;

            if (hProcess) {
                WCHAR name[1024];
                DWORD size = 1024;
                if (QueryFullProcessImageName(hProcess, 0, name, &size)) {
                    auto bs = wcsrchr(name, L'\\');
                    wstring ws(bs ? bs + 0 : name);
                    processName = string(ws.begin(), ws.end());
                    processName.erase(0, 1);
                }
            }

            if (processName != "chrome.exe") continue;            

            IAudioMeterInformation* iAudio;
            pSessionControl->QueryInterface(&iAudio);


            unsigned int channelCount;
            float tLevel[NUMOFCHANNELS];
            iAudio->GetMeteringChannelCount(&channelCount);
            iAudio->GetChannelsPeakValues(channelCount, tLevel);

            int fLevel[NUMOFCHANNELS];
            for (int i = 0; i < NUMOFCHANNELS; i++) fLevel[i] = (int)(tLevel[i] * NUMOFDIODES);

            if (hProcess != NULL) {
                refreshCooldown = 1 - refreshCooldown;
                if (refreshCooldown) continue;

                printVolume(processName, fLevel[0], fLevel[1]);
                writeData(_serial, fLevel[0], fLevel[1]);
            }
        }
    } 
}

int openConnection() {
    CoInitialize(NULL);    

    HANDLE hSerial;
    if (serialCommunication(&hSerial) || !REQUIRESCOM) {
        getVolumeLevel(hSerial);
        CloseHandle(hSerial);
    } CoUninitialize();
}

int main() {
    openConnection();
    return 0;
}