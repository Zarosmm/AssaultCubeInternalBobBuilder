#include "render.h"
#include "globals.h"
#include "memory.h"

#include "imgui_impl_win32.h"
#include "imgui_impl_dx9.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool UpdateOverlayToMatchGame(HWND hwndGame, HWND hwndOverlay) {
    RECT clientRect;
    POINT topLeft = { 0, 0 };

    if (GetClientRect(hwndGame, &clientRect) && ClientToScreen(hwndGame, &topLeft)) {
        int width = clientRect.right - clientRect.left;
        int height = clientRect.bottom - clientRect.top;

        if (topLeft.x != lastRect.left || topLeft.y != lastRect.top ||
            width != (lastRect.right - lastRect.left) || height != (lastRect.bottom - lastRect.top)) {

            lastRect.left = topLeft.x;
            lastRect.top = topLeft.y;
            lastRect.right = topLeft.x + width;
            lastRect.bottom = topLeft.y + height;

            MoveWindow(hwndOverlay, lastRect.left, lastRect.top, width, height, TRUE);

            screenWidth = width;
            screenHeight = height;

            return true;
        }
    }

    return false;
}

bool CreateOverlayWindow() {
    wc = {
        sizeof(WNDCLASSEX),
        CS_CLASSDC,
        WndProc,
        0L,
        0L,
        GetModuleHandle(NULL),
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        L"OverlayClass",
        nullptr
    };

    RegisterClassEx(&wc);
    HWND parentWindow = CreateWindowEx(0, L"STATIC", nullptr, WS_DISABLED,
        0, 0, 0, 0, nullptr, nullptr, wc.hInstance, nullptr);

    hwndOverlay = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED,
        L"OverlayClass", L"Overlay",
        WS_POPUP, 100, 100, 800, 600,
        parentWindow, nullptr, wc.hInstance, nullptr
    );

    SetLayeredWindowAttributes(hwndOverlay, RGB(0, 0, 0), 255, LWA_COLORKEY);
    ShowWindow(hwndOverlay, SW_SHOW);
    return true;
}

bool CreateDevice() {
    pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!pD3D) return false;

    d3dpp = {};
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.hDeviceWindow = hwndOverlay;
    d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
    d3dpp.BackBufferWidth = 800;
    d3dpp.BackBufferHeight = 600;

    return SUCCEEDED(pD3D->CreateDevice(
        D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwndOverlay,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDevice));
}

ImGuiIO& InitOverlay() {
    CreateOverlayWindow();
    CreateDevice();

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(hwndOverlay);
    ImGui_ImplDX9_Init(pDevice);

    return io;
}

void BeginImGuiFrame(ImGuiIO& io) {
    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    io.MouseDown[0] = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    ImGui::NewFrame();
}

void EndImGuiFrame() {
    ImGui::Render();
    pDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
    if (pDevice->BeginScene() >= 0) {
        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
        pDevice->EndScene();
    }
    pDevice->Present(NULL, NULL, NULL, NULL);
}

void Cleanup() {
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (pDevice) pDevice->Release();
    if (pD3D) pD3D->Release();

    UnregisterClass(wc.lpszClassName, wc.hInstance);
}

void DrawMenu(const Player& localPlayer) {
    if (!ImGui::GetCurrentContext()) return;

    static ImVec2 collapsedSize = ImVec2(275, 110);
    static ImVec2 expandedSize = ImVec2(275, 585);
    static ImVec2 windowPos = ImVec2(10, 75);

    float t = ImGui::GetTime();
    float pulse = (sinf(t * 2.0f) + 1.0f) * 0.5f;
    ImVec4 pulseColor = ImLerp(ImVec4(1.0f, 0.3f, 0.0f, 1.0f), ImVec4(1.0f, 1.0f, 0.0f, 1.0f), pulse);
    ImVec4 techBlue = ImVec4(0.2f, 0.7f, 1.0f, 1.0f);
    ImVec4 lightGray = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
    ImVec4 bracketPulse = ImLerp(ImVec4(0.5f, 1.0f, 1.0f, 1.0f), ImVec4(1.0f, 0.6f, 1.0f, 1.0f), pulse);

    ImGui::SetNextWindowSize(settings.showMenu ? expandedSize : collapsedSize, ImGuiCond_Always);
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_FirstUseEver);

    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(pulseColor.x, pulseColor.y, pulseColor.z, 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);

    if (ImGui::Begin("AssaultCube 1.3.0.2 Cheat", nullptr,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {

        windowPos = ImGui::GetWindowPos();
        // Top Title (Game + Version)
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 10);
        ImVec4 glow = ImLerp(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), ImVec4(1.0f, 1.0f, 0.3f, 1.0f), (sinf(t * 3.0f) + 1.0f) * 0.5f);
        ImGui::TextColored(glow, "  By BobBuilder / Raphael");


        // Sub-author line with animated gradient
        float t = ImGui::GetTime();
        

        int gamemodeId = *reinterpret_cast<int*>(OFFSET_GAMEMODE);
        const char* gamemodeStr = "Unknown";
        switch (gamemodeId) {
        case 7: gamemodeStr = "TDM"; break;
        case 8: gamemodeStr = "DM"; break;
        case 12: gamemodeStr = "OSOK"; break;
        case 18: gamemodeStr = "Pistol Frenzy"; break;
        case 19: gamemodeStr = "Last Swiss Standing"; break;
        case 20: gamemodeStr = "Team Survivor"; break;
        case 21: gamemodeStr = "Team OSOK"; break;
        }

        char mapName[32] = {};
        strncpy_s(mapName, reinterpret_cast<char*>(OFFSET_MAPNAME), sizeof(mapName) - 1);

        ImGui::Text("Gamemode: "); ImGui::SameLine(); ImGui::TextColored(techBlue, "%s", gamemodeStr);
        ImGui::SameLine(); ImGui::Text("|"); ImGui::SameLine();
        ImGui::Text("Map: "); ImGui::SameLine(); ImGui::TextColored(techBlue, "%s", mapName);
        ImGui::Text("Fly"); ImGui::SameLine(); ImGui::TextColored(bracketPulse, "[F]");
        ImGui::SameLine(); ImGui::Text("|"); ImGui::SameLine(); ImGui::Text("Menu"); ImGui::SameLine(); ImGui::TextColored(bracketPulse, "[INS]");
        ImGui::SameLine(); ImGui::Text("|"); ImGui::SameLine(); ImGui::Text("Unload"); ImGui::SameLine(); ImGui::TextColored(bracketPulse, "[DEL]");

        if (settings.showMenu) {
            ImGui::TextColored(pulseColor, "Visuals");
            ImGui::Checkbox("Show Enemies", &settings.showEnemies);
            ImGui::Checkbox("Show Friendlies", &settings.showFriendlies);
            ImGui::Checkbox("Show Player Names", &settings.showNames);
            ImGui::Checkbox("Show Player Distances", &settings.showDistances);
            ImGui::Checkbox("Show Snaplines", &settings.showSnaplines);

            ImGui::TextColored(pulseColor, "Gameplay Mods");
            ImGui::Checkbox("Godmode", &settings.godmode);
            ImGui::Checkbox("Unlimited Ammo", &settings.infiniteAmmo);
            ImGui::Checkbox("No Recoil", &settings.noRecoil);
            ImGui::Checkbox("Fast Shooting", &settings.fastGun);


            ImGui::TextColored(pulseColor, "Hook Features");
            ImGui::Checkbox("Instant Kill", &settings.enableInstantKill);
            ImGui::Checkbox("Silent Aim", &settings.enableSilentAim);

            ImGui::TextColored(pulseColor, "Aimbot");
            ImGui::Checkbox("Aimbot Enabled", &settings.enableAimbot);
            static const char* aimKeys[] = { "None", "Left Click", "Right Click", "CTRL", "ALT" };
            static int selectedKey = 0;
            if (ImGui::Combo("Aimbot Key", &selectedKey, aimKeys, IM_ARRAYSIZE(aimKeys))) {
                settings.aimKey = static_cast<AimKey>(selectedKey);
            }

            ImGui::TextColored(pulseColor, "FOV Options");
            ImGui::Checkbox("Show Circle", &settings.showFovCircle);
            if (settings.showFovCircle) {
                ImGui::SliderFloat("Radius", &settings.fovSize, 50.0f, 1000.0f, "%.0f px");
            }

            ImGui::TextColored(pulseColor, "Snapline Origin");
            static const char* allOrigins[] = {
                "Top of Screen",
                "Bottom of Screen",
                "Top of FOV Circle",
                "Bottom of FOV Circle"
            };
            static const char* minimalOrigins[] = {
                "Top of Screen",
                "Bottom of Screen"
            };
            int currentOrigin = static_cast<int>(settings.snaplineOrigin);
            if (!settings.showFovCircle && currentOrigin >= 2) {
                currentOrigin = 0;
                settings.snaplineOrigin = SnaplineOrigin::TopScreen;
            }
            if (settings.showFovCircle) {
                if (ImGui::Combo("Snapline", &currentOrigin, allOrigins, IM_ARRAYSIZE(allOrigins))) {
                    settings.snaplineOrigin = static_cast<SnaplineOrigin>(currentOrigin);
                }
            }
            else {
                if (ImGui::Combo("Snapline", &currentOrigin, minimalOrigins, IM_ARRAYSIZE(minimalOrigins))) {
                    settings.snaplineOrigin = static_cast<SnaplineOrigin>(currentOrigin);
                }
            }

        }

        ImGui::Separator();

        // Footer: GitHub and Website
        if (ImGui::BeginTable("links", 2, ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("GitHub:");
            ImGui::TableSetColumnIndex(1); ImGui::TextColored(techBlue, "github.com/bobbuilder123");

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("Website:");
            ImGui::TableSetColumnIndex(1); ImGui::TextColored(techBlue, "adminions.ca");
            ImGui::EndTable();
        }


        if (settings.showMenu)
            expandedSize = ImGui::GetWindowSize();
        else if (ImGui::GetFrameCount() < 3 || collapsedSize.y < 200.0f)
            collapsedSize = ImGui::GetWindowSize();
    }

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}




void UpdateRenderDimensions() {
    RECT clientRect;
    GetClientRect(hwndGame, &clientRect);
    POINT topLeft = { 0, 0 };
    ClientToScreen(hwndGame, &topLeft);

    screenWidth = clientRect.right;
    screenHeight = clientRect.bottom;
    MoveWindow(hwndOverlay, topLeft.x, topLeft.y, screenWidth, screenHeight, TRUE);
}

bool IsGameMinimized() {
    WINDOWPLACEMENT placement = { sizeof(WINDOWPLACEMENT) };
    GetWindowPlacement(hwndGame, &placement);
    return placement.showCmd == SW_SHOWMINIMIZED;
}

void HandleWindowMessages(MSG& msg) {
    if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

bool HandleDeviceResetIfNeeded(int& lastWidth, int& lastHeight) {
    if (screenWidth == lastWidth && screenHeight == lastHeight) return true;
    if (settings.showMenu && ImGui::IsAnyMouseDown()) return true;

    lastWidth = screenWidth;
    lastHeight = screenHeight;

    ImGui_ImplDX9_InvalidateDeviceObjects();

    d3dpp.BackBufferWidth = screenWidth;
    d3dpp.BackBufferHeight = screenHeight;

    HRESULT hr = pDevice->Reset(&d3dpp);
    if (FAILED(hr)) {
        MessageBoxA(NULL, "[ERROR] Device reset failed", "Debug", MB_OK | MB_TOPMOST);
        return false;
    }

    ImGui_ImplDX9_CreateDeviceObjects();
    return true;
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED) return 0;
        screenWidth = (UINT)LOWORD(lParam);
        screenHeight = (UINT)HIWORD(lParam);
        return 0;

    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;

    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }

    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
