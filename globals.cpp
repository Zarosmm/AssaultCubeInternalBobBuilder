#include "globals.h"

HWND hwndOverlay = nullptr;
HWND hwndGame = nullptr;
LPDIRECT3D9 pD3D = nullptr;
LPDIRECT3DDEVICE9 pDevice = nullptr;
HANDLE hGame = nullptr;
MSG msg = {};
WNDCLASSEX wc = {};
D3DPRESENT_PARAMETERS d3dpp = {};

int screenWidth = 0;
int screenHeight = 0;
RECT lastRect = {};

bool lastKeyState = false;
bool lastInstantKill = false;
bool lastSilentAim = false;

CheatSettings settings;