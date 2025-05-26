#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <d3d9.h>
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx9.h"
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <optional>
#include "vec.h"

// === CONSTANTS ===
constexpr uintptr_t OFFSET_LOCALPLAYER = 0x0058AC00;
constexpr uintptr_t OFFSET_NUMPLAYERS = 0x0058AC0C;
constexpr uintptr_t OFFSET_PLAYERLIST = 0x0058AC04;
constexpr uintptr_t OFFSET_CURRENT_FRAME = 0x0057F10C;
constexpr uintptr_t OFFSET_VIEWMATRIX_BASE = 0x0057DFD0;
constexpr uintptr_t OFFSET_RECOIL = 0x004C2EC3;
constexpr uintptr_t OFFSET_GAMEMODE = 0x0058ABF8;
constexpr uintptr_t OFFSET_MAPNAME = 0x0058AE0C;

constexpr int MAX_PLAYERS = 32;
inline const wchar_t* assault_cube_procname = L"AssaultCube";

// === GLOBAL STATE (externs) ===
extern HWND hwndOverlay;
extern HWND hwndGame;
extern LPDIRECT3D9 pD3D;
extern LPDIRECT3DDEVICE9 pDevice;
extern HANDLE hGame;
extern MSG msg;
extern WNDCLASSEX wc;
extern D3DPRESENT_PARAMETERS d3dpp;

extern int screenWidth;
extern int screenHeight;
extern RECT lastRect;

extern bool noRecoilCurrentlyPatched;
extern bool lastKeyState;
extern bool lastInstantKill;
extern bool lastSilentAim;

// === SETTINGS ===
enum AimKey {
    AIMKEY_NONE = 0,
    AIMKEY_LEFT = 1,
    AIMKEY_RIGHT = 2,
    AIMKEY_CTRL = 3,
    AIMKEY_ALT = 4
};

enum class SnaplineOrigin {
    TopScreen = 0,
    BottomScreen = 1,
    TopFOV = 2,
    BottomFOV = 3
};

struct CheatSettings {
    bool showMenu = false;
    bool showSnaplines = true;
    bool showEnemies = true;
    bool showFriendlies = true;
    bool showNames = true;
    bool showDistances = true;
    bool godmode = false;
    bool infiniteAmmo = false;
    bool enableAimbot = false;
    bool noRecoil = false;
    bool fastGun = false;
    bool enableInstantKill = false;
    bool enableSilentAim = false;
    bool showFovCircle = true;
    float fovSize = 400.0f;
    AimKey aimKey = AIMKEY_RIGHT;
    SnaplineOrigin snaplineOrigin = SnaplineOrigin::TopFOV;
};

extern CheatSettings settings;
