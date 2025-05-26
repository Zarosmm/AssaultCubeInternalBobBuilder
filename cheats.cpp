#include "cheats.h"
#include "globals.h"
#include "vec.h"


bool WorldToScreen(const Vec3& world, Vec2& screen, float* m, int width, int height) {
    // Matrix is column-major: index as [row + col * 4]
    float clipX = world.x * m[0] + world.y * m[4] + world.z * m[8] + m[12];
    float clipY = world.x * m[1] + world.y * m[5] + world.z * m[9] + m[13];
    float clipW = world.x * m[3] + world.y * m[7] + world.z * m[11] + m[15];

    if (clipW < 0.001f) return false;

    float ndcX = clipX / clipW;
    float ndcY = clipY / clipW;

    screen.x = (width / 2.0f) + (ndcX * width / 2.0f);
    screen.y = (height / 2.0f) - (ndcY * height / 2.0f);

    return true;
}

Player GetLocalPlayer() {
    PlayerStruct* ptr = *reinterpret_cast<PlayerStruct**>(OFFSET_LOCALPLAYER);
    if (!ptr) return Player();
    Player localPlayer(ptr);
    if (settings.infiniteAmmo) {
        localPlayer.SetAmmo(999);
        localPlayer.SetMag(999);
    }
    
    if (settings.godmode) {
        localPlayer.SetHealth(100);
        localPlayer.SetShield(100);
    }
    return localPlayer;
}

std::vector<Player> BuildPlayerList(const Player& localPlayer) {
    std::vector<Player> result;
    float* viewMatrix = reinterpret_cast<float*>(OFFSET_VIEWMATRIX_BASE);
    uintptr_t* list = *reinterpret_cast<uintptr_t**>(OFFSET_PLAYERLIST);
    int count = *reinterpret_cast<int*>(OFFSET_NUMPLAYERS);
    int frame = *reinterpret_cast<int*>(OFFSET_CURRENT_FRAME);
    Vec2 screenCenter = { screenWidth / 2.f, screenHeight / 2.f };

    for (int i = 0; i < count && i < MAX_PLAYERS; i++) {
        PlayerStruct* pStruct = reinterpret_cast<PlayerStruct*>(list[i]);
        if (!pStruct || pStruct == localPlayer.ptr || pStruct->health <= 0) continue;

        Player p(pStruct);
        if (!WorldToScreen(p.HeadPos(), p.screenHead, viewMatrix, screenWidth, screenHeight)) continue;
        if (!WorldToScreen(p.FeetPos(), p.screenFeet, viewMatrix, screenWidth, screenHeight)) continue;

        p.isEnemy = p.IsEnemy(localPlayer);
        p.isVisible = p.IsVisible(frame);
        p.distance = p.DistanceTo(localPlayer);
        p.boxHeight = p.screenFeet.y - p.screenHead.y;
        p.boxWidth = p.boxHeight * 0.5f;
        p.distanceToAim = screenCenter.DistanceTo(p.screenHead);

        result.push_back(std::move(p));
    }
    return result;
}

void DrawFOV() {
    if (!ImGui::GetCurrentContext()) return;
    auto* drawList = ImGui::GetBackgroundDrawList();
    if (!drawList || !settings.showFovCircle) return;

    float t = ImGui::GetTime();
    float pulse = (sin(t * 2.0f) + 1.0f) * 0.5f; // 0..1
    ImVec4 colorStart = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
    ImVec4 colorEnd = ImVec4(1.0f, 0.6f, 0.0f, 1.0f); // Orange
    ImVec4 mixedColor = ImLerp(colorStart, colorEnd, pulse);
    ImU32 finalColor = ImGui::ColorConvertFloat4ToU32(mixedColor);

    ImVec2 center = ImVec2(screenWidth / 2.f, screenHeight / 2.f);
    drawList->AddCircle(center, settings.fovSize, finalColor, 64, 2.0f);
}

void DrawESP(const std::vector<Player>& players) {
    if (!ImGui::GetCurrentContext()) return;

    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    if (!drawList) return;

    for (const auto& player : players) {
        if ((settings.showEnemies && player.isEnemy) || (settings.showFriendlies && !player.isEnemy)) {
            player.DrawBox(drawList, 3.0f);
            player.DrawHealthBar(drawList);

            if (settings.showNames && settings.showDistances)
                player.DrawNameAndDistance(drawList);
            else if (settings.showNames)
                player.DrawName(drawList);
            else if (settings.showDistances)
                player.DrawDistance(drawList, player.GetBoxColor());

            if (settings.showSnaplines)
                player.DrawSnapline(drawList);
        }
    }
}


bool IsAimKeyPressed() {
    switch (settings.aimKey) {
    case AIMKEY_LEFT:  return (GetAsyncKeyState(VK_LBUTTON) & 0x8000);
    case AIMKEY_RIGHT: return (GetAsyncKeyState(VK_RBUTTON) & 0x8000);
    case AIMKEY_CTRL:  return (GetAsyncKeyState(VK_CONTROL) & 0x8000);
    case AIMKEY_ALT:   return (GetAsyncKeyState(VK_MENU) & 0x8000);  // VK_MENU = Alt
    default: return false;
    }
}

void Aimbot(const std::vector<Player>& players, const Player& localPlayer) {
    if (!settings.enableAimbot || !IsAimKeyPressed()) return;

    const Player* closestPlayer = nullptr;

    for (const auto& player : players) {
        if (!player.isEnemy || !player.isVisible) continue;
        if (player.distanceToAim > settings.fovSize) continue;
        if (!closestPlayer || player.distanceToAim < closestPlayer->distanceToAim)
            closestPlayer = &player;
    }

    if (closestPlayer) {
        Vec2 aimAngles = CalcAimAngles(localPlayer.HeadPos(), closestPlayer->HeadPos() + Vec3(0.f, 0.f, 0.25f));
        localPlayer->yaw = aimAngles.y;
        localPlayer->pitch = aimAngles.x;
    }

}



typedef int(__thiscall* tDamageCalc)(void* thisPtr, int a1, int a2);
tDamageCalc oDamageCalc = nullptr;

int __fastcall hkDamageCalc(void* thisPtr, void* /*unusedEDX*/, int a1, int a2) {
    uintptr_t playerStructAddr = reinterpret_cast<uintptr_t>(thisPtr) - 0xE8;
    PlayerStruct* attacker = reinterpret_cast<PlayerStruct*>(playerStructAddr);
    PlayerStruct* local = *reinterpret_cast<PlayerStruct**>(OFFSET_LOCALPLAYER);

    if (!attacker || !local) return 0;

    // LocalPlayer attacking -> instant kill
    if (attacker == local) {
        std::cout << "[INFO] Enemy tried to do damage — blocked\n";
        a1 = 0;
    }
    else {
        std::cout << "[AIMBOT] Local player hit someone! Instant kill!\n";
        a1 = 999;

    }
    return oDamageCalc(thisPtr, a1, a2);
}


typedef PlayerStruct* (__cdecl* TraceRayToPlayersFn)(PlayerStruct* shooter, float* rayOrigin, int someId, float* outDistance, int* outTarget, char someFlag);
TraceRayToPlayersFn oTraceRayToPlayers = nullptr;

PlayerStruct* __cdecl hkTraceRayToPlayers(PlayerStruct* shooter, float* ray, int id, float* outDistance, int* outTarget, char someFlag) {
    PlayerStruct* localPtr = *reinterpret_cast<PlayerStruct**>(OFFSET_LOCALPLAYER);
    if (localPtr && shooter == localPtr) {
        Player localPlayer(localPtr);
        auto players = BuildPlayerList(localPlayer);
        for (auto& p : players) {
            if (p.IsValid() && p.IsEnemy(localPlayer)) {
                return p.ptr;
            }
        }
    }
    return 0;
}