#include <windows.h>
#include <tlhelp32.h>
#include <iostream>

DWORD GetProcessID(const wchar_t* exeName) {
    PROCESSENTRY32W entry = { sizeof(entry) };
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        std::wcerr << L"[!] CreateToolhelp32Snapshot failed: " << GetLastError() << "\n";
        return 0;
    }

    while (Process32NextW(snapshot, &entry)) {
        if (_wcsicmp(entry.szExeFile, exeName) == 0) {
            CloseHandle(snapshot);
            std::wcout << L"[+] Found process: " << exeName << L", PID: " << entry.th32ProcessID << "\n";
            return entry.th32ProcessID;
        }
    }

    std::wcerr << L"[!] Process not found: " << exeName << "\n";
    CloseHandle(snapshot);
    return 0;
}

bool InjectDLL(DWORD pid, const wchar_t* dllPath) {
    std::wcout << L"[+] Opening target process...\n";
    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProc) {
        std::wcerr << L"[!] OpenProcess failed: " << GetLastError() << "\n";
        return false;
    }

    SIZE_T size = (wcslen(dllPath) + 1) * sizeof(wchar_t);
    std::wcout << L"[+] Allocating memory in target...\n";
    LPVOID remoteMem = VirtualAllocEx(hProc, nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remoteMem) {
        std::wcerr << L"[!] VirtualAllocEx failed: " << GetLastError() << "\n";
        CloseHandle(hProc);
        return false;
    }

    std::wcout << L"[+] Writing DLL path to remote memory...\n";
    if (!WriteProcessMemory(hProc, remoteMem, dllPath, size, nullptr)) {
        std::wcerr << L"[!] WriteProcessMemory failed: " << GetLastError() << "\n";
        VirtualFreeEx(hProc, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProc);
        return false;
    }

    std::wcout << L"[+] Getting address of LoadLibraryW...\n";
    FARPROC loadLib = GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW");
    if (!loadLib) {
        std::wcerr << L"[!] GetProcAddress failed: " << GetLastError() << "\n";
        VirtualFreeEx(hProc, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProc);
        return false;
    }

    std::wcout << L"[~] Creating remote thread...\n";
    HANDLE hThread = CreateRemoteThread(hProc, nullptr, 0,
        (LPTHREAD_START_ROUTINE)loadLib, remoteMem, 0, nullptr);
    if (!hThread) {
        std::wcerr << L"[!] CreateRemoteThread failed: " << GetLastError() << "\n";
        VirtualFreeEx(hProc, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProc);
        return false;
    }

    std::wcout << L"[+] Waiting for DLL to be loaded...\n";
    WaitForSingleObject(hThread, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeThread(hThread, &exitCode);
    if (exitCode == 0) {
        std::wcerr << L"[!] LoadLibrary failed inside the game. (exit code = 0)\n";
    }
    else {
        std::wcout << L"[+] DLL loaded at base address: 0x" << std::hex << exitCode << "\n";
    }

    CloseHandle(hThread);
    VirtualFreeEx(hProc, remoteMem, 0, MEM_RELEASE);
    CloseHandle(hProc);
    return exitCode != 0;
}

int wmain(int argc, wchar_t* argv[]) {
    const wchar_t* dllPath = L"Path_for_AssaultCubeAimbot.dll";
    const wchar_t* procName = L"ac_client.exe";

    DWORD pid = GetProcessID(procName);
    if (!pid) return 1;

    if (InjectDLL(pid, dllPath)) {
        std::wcout << L"Injection successful.\n";
    } else {
        std::wcerr << L"Injection failed.\n";
    }
    return 0;
}