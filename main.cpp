// assaultcube_esp_aimbot.cpp
// Internal AssaultCube ESP + Aimbot with full debug output and persistent overlay loop
#define IMGUI_DEFINE_MATH_OPERATORS
#include <Windows.h>
#include <TlHelp32.h>
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <optional>
#include "globals.h"
#include "vec.h"
#include "player.h"
#include "cheats.h"
#include "render.h"
#include <xmmintrin.h>     // For __m128
#include <emmintrin.h>     // For __m128i (SSE2)
#define MH_STATIC
#include "MinHook.h"
#include "cheats.h"
#include <intrin.h>
#pragma intrinsic(_ReturnAddress)
#pragma comment(lib, "d3d9.lib")


// Worker thread that runs inside the game process
DWORD WINAPI CheatThread(HMODULE hModule)
{
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);              // Redirect cout to console
    freopen_s(&f, "CONOUT$", "w", stderr);              // Redirect cerr
    freopen_s(&f, "CONIN$", "r", stdin);                // Optional: enable cin

    std::cout << "[+] Console ready\n";
    
    if (MH_Initialize() != MH_OK) {
        MessageBoxA(0, "MinHook initialization failed!", "ERROR", MB_ICONERROR);
        return 1;
    }

    // Instant Kill
    MH_CreateHook((LPVOID)0x0041C130, &hkDamageCalc, (void**)&oDamageCalc);
    // Silent Aim
    MH_CreateHook((LPVOID)0x004CA250, &hkTraceRayToPlayers, (void**)&oTraceRayToPlayers); 
    
    
    

    HWND hwndGame = FindWindow(NULL, L"AssaultCube");
    if (!hwndGame) {
        std::cout << "[!] Game window not found\n";
        return 1;
    }

    // Create overlay window and Direct3D device
    wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L,
           GetModuleHandle(NULL), nullptr, nullptr, nullptr, nullptr,
           L"OverlayClass", nullptr };
    RegisterClassEx(&wc);
    hwndOverlay = CreateWindowExW(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
        L"OverlayClass", L"Overlay", WS_POPUP, 100, 100, 800, 600,
        nullptr, nullptr, wc.hInstance, nullptr);
    SetLayeredWindowAttributes(hwndOverlay, RGB(0, 0, 0), 255, LWA_COLORKEY);
    ShowWindow(hwndOverlay, SW_SHOW);

    pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!pD3D) { std::cout << "[!] Direct3DCreate9 failed\n"; return 1; }

    D3DPRESENT_PARAMETERS d3dpp = {};
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.hDeviceWindow = hwndOverlay;
    d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
    d3dpp.BackBufferWidth = 800;
    d3dpp.BackBufferHeight = 600;

    if (FAILED(pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwndOverlay,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDevice))) {
        std::cout << "[!] CreateDevice failed\n"; return 1;
    }

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hwndOverlay);
    ImGui_ImplDX9_Init(pDevice);

    std::cout << "[+] ImGui overlay initialized\n";

    while (!(GetAsyncKeyState(VK_DELETE) & 1)) {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (UpdateOverlayToMatchGame(hwndGame, hwndOverlay)) {
            ImGui_ImplDX9_InvalidateDeviceObjects();

            D3DPRESENT_PARAMETERS d3dpp = {};
            d3dpp.Windowed = TRUE;
            d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
            d3dpp.hDeviceWindow = hwndOverlay;
            d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
            d3dpp.BackBufferWidth = screenWidth;
            d3dpp.BackBufferHeight = screenHeight;
            d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

            if (FAILED(pDevice->Reset(&d3dpp))) {
                std::cout << "[!] Device reset failed\n";
                break;
            }
            ImGui_ImplDX9_CreateDeviceObjects();
        }
        if (IsGameMinimized()) {
            ShowWindow(hwndOverlay, SW_HIDE);
            continue;
        }
        else {
            ShowWindow(hwndOverlay, SW_SHOW);
        }
        bool keyPressed = (GetAsyncKeyState(VK_INSERT) & 1);
        if (keyPressed && !lastKeyState) {
            settings.showMenu = !settings.showMenu;
            if (settings.showMenu) {
                ShowCursor(TRUE);
                SetForegroundWindow(hwndOverlay);
            }
        }
        lastKeyState = keyPressed;
        LONG_PTR exStyle = GetWindowLongPtr(hwndOverlay, GWL_EXSTYLE);

        if (settings.showMenu) {
            if (exStyle & WS_EX_TRANSPARENT) {
                SetWindowLongPtr(hwndOverlay, GWL_EXSTYLE, exStyle & ~WS_EX_TRANSPARENT);
            }
        }
        else {
            if (!(exStyle & WS_EX_TRANSPARENT)) {
                SetWindowLongPtr(hwndOverlay, GWL_EXSTYLE, exStyle | WS_EX_TRANSPARENT);
            }
        }

        
        io.DisplaySize = ImVec2((float)screenWidth, (float)screenHeight);

        if (settings.enableInstantKill != lastInstantKill) {
            if (settings.enableInstantKill)
                MH_EnableHook((LPVOID)0x0041C130);
            else if (lastInstantKill)
                MH_DisableHook((LPVOID)0x0041C130);
            lastInstantKill = settings.enableInstantKill;
        }

        if (settings.enableSilentAim != lastSilentAim) {
            if (settings.enableSilentAim)
                MH_EnableHook((LPVOID)0x004CA250);
            else if (lastSilentAim)
                MH_DisableHook((LPVOID)0x004CA250);
            lastSilentAim = settings.enableSilentAim;
        }

        PlayerStruct* localPtr = *reinterpret_cast<PlayerStruct**>(OFFSET_LOCALPLAYER);
        if (!localPtr) {
            Sleep(10);
            continue;
        }
        Player localPlayer(localPtr);

        if (GetAsyncKeyState('F') & 1) {
            if (localPlayer->fly != 4) {
                localPlayer->fly = 4;
            }
            else {
                localPlayer->fly = 0;
            }
        }
        if (settings.infiniteAmmo) {
            localPlayer.SetAmmo(999);
            localPlayer.SetMag(999);
        }
        if (settings.godmode) {
            localPlayer.SetHealth(100);
            localPlayer.SetShield(100);
        }

        if (settings.noRecoil) {
            localPlayer.DisableRecoil();
        }
        if (settings.fastGun) {
            localPlayer.IncreaseGunSpeed();
        }
        auto players = BuildPlayerList(localPlayer);

        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        io.MouseDown[0] = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
        ImGui::NewFrame();

        DrawESP(players);
        DrawFOV();
        Aimbot(players, localPlayer);
        DrawMenu(localPlayer);
        ImGui::Render();
        pDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
        if (pDevice->BeginScene() >= 0) {
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            pDevice->EndScene();
        }
        pDevice->Present(NULL, NULL, NULL, NULL);
        Sleep(10);
    }

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    if (pDevice) pDevice->Release();
    if (pD3D) pD3D->Release();
    if (hwndOverlay) DestroyWindow(hwndOverlay);
    UnregisterClass(wc.lpszClassName, wc.hInstance);
    MessageBoxA(NULL, "[+] Cheat thread stoped, closing...", "Debug", MB_OK | MB_TOPMOST);
    
    // Unhook minhook
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();

    fclose(stdin); fclose(stdout); fclose(stderr);
    FreeConsole();
    FreeLibraryAndExitThread(hModule, 0);
    return 0;
}


// DllMain: entry point called by Windows loader
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);

        HANDLE hThread = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)CheatThread, hModule, 0, nullptr);
        if (!hThread)
        {
            MessageBoxA(NULL, "[!] Failed to create cheat thread", "Error", MB_OK | MB_TOPMOST);
        }
    }
    return TRUE;
}

