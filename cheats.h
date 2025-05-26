#pragma once

#include <vector>
#include "player.h"

// Core features
bool WorldToScreen(const Vec3& world, Vec2& screen, float* m, int width, int height);
void DrawESP(const std::vector<Player>& players);
void DrawFOV();
void Aimbot(const std::vector<Player>& players, const Player& localPlayer);

// Game logic
Player GetLocalPlayer();
std::vector<Player> BuildPlayerList(const Player& localPlayer);

// Hook features
typedef int(__thiscall* tDamageCalc)(void*, int, int);
typedef PlayerStruct* (__cdecl* TraceRayToPlayersFn)(PlayerStruct*, float*, int, float*, int*, char);

extern tDamageCalc oDamageCalc;
extern TraceRayToPlayersFn oTraceRayToPlayers;

int __fastcall hkDamageCalc(void*, void*, int, int);
PlayerStruct* __cdecl hkTraceRayToPlayers(PlayerStruct*, float*, int, float*, int*, char);