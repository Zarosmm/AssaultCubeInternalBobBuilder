#include "player.h"

Player::Player(PlayerStruct* playerPtr) {
    if (!playerPtr) return;
    ptr = playerPtr;
    address = reinterpret_cast<uintptr_t>(playerPtr);
}

Player::Player(Player&& other) noexcept {
    *this = std::move(other);
}

Player& Player::operator=(Player&& other) noexcept {
    ptr = other.ptr;
    address = other.address;
    screenHead = other.screenHead;
    screenFeet = other.screenFeet;
    isEnemy = other.isEnemy;
    isVisible = other.isVisible;
    distance = other.distance;
    boxHeight = other.boxHeight;
    boxWidth = other.boxWidth;
    distanceToAim = other.distanceToAim;
    return *this;
}

PlayerStruct* Player::operator->() const { return ptr; }
PlayerStruct& Player::operator*() const { return *ptr; }

bool Player::IsValid() const { return ptr != nullptr; }
bool Player::IsAlive() const { return ptr && ptr->health > 0; }
int Player::GetTeam() const { return ptr ? (ptr->state & 1) : -1; }
bool Player::IsEnemy(const Player& localPlayer) const { return IsValid() && ((ptr->state & 1) != (localPlayer->state & 1)); }
bool Player::IsVisible(int currentFrame) const { return IsValid() && ptr->lastVisibleFrame >= currentFrame; }

int Player::GetHealth() const { return ptr ? ptr->health : 0; }
int Player::GetArmor() const { return ptr ? ptr->armor : 0; }
const char* Player::GetName() const { return ptr ? ptr->name : ""; }
float Player::GetYaw() const { return ptr ? ptr->yaw : 0.f; }
float Player::GetPitch() const { return ptr ? ptr->pitch : 0.f; }
Vec3 Player::HeadPos() const { return ptr->positionHead + Vec3(0.f, 0.f, 0.75f); }
Vec3 Player::FeetPos() const { return ptr->positionFeet; }
float Player::DistanceTo(const Player& other) const { return this->HeadPos().Distance(other.HeadPos()); }

CurrentWeapon* Player::GetWeapon() const {
    if (!ptr || !ptr->lastWeapon || !ptr->lastWeapon->currWeapon)
        return nullptr;
    return ptr->lastWeapon->currWeapon;
}
void Player::DisableRecoil() const {
    CurrentWeapon* cw = GetWeapon();
    if (!cw) return;
    cw->noRecoil = 0;
}

void Player::IncreaseGunSpeed() const {
    CurrentWeapon* cw = GetWeapon();
    if (!cw) return;
    cw->shootingSpeed = 0;
}

void Player::SetMag(int value) const { if (address) *reinterpret_cast<int*>(address + 0x11C) = value; }
void Player::SetAmmo(int value) const { if (address) *reinterpret_cast<int*>(address + 0x140) = value; }
void Player::SetHealth(int value) const { if (address) *reinterpret_cast<int*>(address + 0xEC) = value; }
void Player::SetShield(int value) const { if (address) *reinterpret_cast<int*>(address + 0xF0) = value; }

void Player::ComputeBoxDimensions() {
    boxHeight = screenFeet.y - screenHead.y;
    boxWidth = boxHeight * 0.5f;
}

ImVec2 Player::GetBoxTopLeft() const {
    return ImVec2(screenHead.x - boxWidth / 2, screenHead.y);
}

ImVec2 Player::GetBoxBottomRight() const {
    return ImVec2(screenHead.x + boxWidth / 2, screenFeet.y);
}

void Player::DrawBox(ImDrawList* drawList, float rounding, float thickness) const {
    drawList->AddRect(GetBoxTopLeft(), GetBoxBottomRight(), GetBoxColor(), rounding, 0, isVisible ? thickness*2.5 : thickness);
}

ImU32 LerpColor(const ImVec4& a, const ImVec4& b, float t) {
    return ImColor(
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t,
        a.w + (b.w - a.w) * t
    );
}

ImU32 Player::GetBoxColor() const {
    float t = (sinf(ImGui::GetTime() * 3.5f) + 1.0f) / 2.0f;

    if (isEnemy) {
        if (isVisible) {
            // Red → reddish-orange
            return LerpColor(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), ImVec4(1.0f, 0.3f, 0.0f, 1.0f), t);
        }
        else {
            // Yellow → warm orange-yellow (pure yellow zone)
            return LerpColor(ImVec4(1.0f, 1.0f, 0.3f, 1.0f), ImVec4(1.0f, 0.85f, 0.0f, 1.0f), t);
        }
    }
    else {
        if (isVisible) {
            // Light blue → cyan-blue
            return LerpColor(ImVec4(0.0f, 0.5f, 1.0f, 1.0f), ImVec4(0.2f, 0.8f, 1.0f, 1.0f), t);
        }
        else {
            // Teal → bright cyan
            return LerpColor(ImVec4(0.0f, 0.7f, 0.7f, 1.0f), ImVec4(0.4f, 1.0f, 1.0f, 1.0f), t);
        }
    }
}






void Player::DrawHealthBar(ImDrawList* drawList) const {
    float time = ImGui::GetTime();
    float boxHeight = screenFeet.y - screenHead.y;
    float healthPct = clamp(GetHealth() / 100.0f, 0.0f, 1.0f);
    float healthHeight = boxHeight * healthPct;
    float pulse = 0.75f + 0.05f * sinf(time * 2.0f);  // subtle pulse
    ImU32 fillColor = IM_COL32(0, static_cast<int>(255 * pulse), 0, 255);
    ImVec2 topLeft = GetBoxTopLeft();
    ImVec2 bottomRight = GetBoxBottomRight();
    ImVec2 barStart = { topLeft.x - 6, bottomRight.y - healthHeight };
    ImVec2 barEnd = { topLeft.x - 2, bottomRight.y };
    drawList->AddRectFilled(barStart, barEnd, IM_COL32(0, 255, 0, 255));
    drawList->AddRect(ImVec2(topLeft.x - 6, topLeft.y), ImVec2(topLeft.x - 2, bottomRight.y), fillColor);
}

void Player::DrawName(ImDrawList* drawList) const {
    const char* name = GetName();
    float fontSize = 16.0f;
    float yOffset = -20.0f;
    bool breathe = true;

    float scale = breathe ? (1.0f + 0.12f * sinf(ImGui::GetTime() * 2.5f)) : 1.0f;
    fontSize *= scale;

    ImVec2 topLeft = GetBoxTopLeft();
    ImVec2 bottomRight = GetBoxBottomRight();
    float centerX = (topLeft.x + bottomRight.x) * 0.5f;
    float y = topLeft.y + yOffset;

    ImVec2 textSize = ImGui::CalcTextSize(name);
    ImVec2 pos = ImVec2(centerX - textSize.x * 0.5f, y);

    drawList->AddText(nullptr, fontSize, pos, IM_COL32(255, 255, 255, 255), name);
}


void Player::DrawDistance(ImDrawList* drawList, ImU32 bracketColor) const {
    const char* lBracket = "[";
    const char* rBracket = "]";
    char distBuf[16]; snprintf(distBuf, sizeof(distBuf), "%.1fm", distance);

    float fontSize = 14.0f;
    float yOffset = -14.0f;

    ImVec2 lSize = ImGui::CalcTextSize(lBracket);
    ImVec2 dSize = ImGui::CalcTextSize(distBuf);
    ImVec2 rSize = ImGui::CalcTextSize(rBracket);
    float totalWidth = lSize.x + dSize.x + rSize.x + 2.0f;

    float centerX = (GetBoxTopLeft().x + GetBoxBottomRight().x) / 2.0f;
    float x = centerX - totalWidth * 0.5f;
    float y = GetBoxTopLeft().y + yOffset;

    drawList->AddText(ImVec2(x, y), bracketColor, lBracket);
    x += lSize.x;
    drawList->AddText(ImVec2(x, y), IM_COL32(255, 255, 255, 255), distBuf);
    x += dSize.x + 2.0f;
    drawList->AddText(ImVec2(x, y), bracketColor, rBracket);
}

void Player::DrawNameAndDistance(ImDrawList* drawList) const {
    const float nameFontSize = 17.0f;
    const float distFontSize = 15.0f;
    const float spacing = 12.0f;

    const char* name = GetName();
    char distBuf[16]; snprintf(distBuf, sizeof(distBuf), "%.1fm", distance);
    const char* lBracket = "[";
    const char* rBracket = "]";

    // Text measurements
    ImVec2 nameSize = ImGui::CalcTextSize(name);
    ImVec2 lSize = ImGui::CalcTextSize(lBracket);
    ImVec2 dSize = ImGui::CalcTextSize(distBuf);
    ImVec2 rSize = ImGui::CalcTextSize(rBracket);

    float totalWidth = nameSize.x + spacing + lSize.x + dSize.x + rSize.x;
    float centerX = (GetBoxTopLeft().x + GetBoxBottomRight().x) * 0.5f;
    float y = GetBoxTopLeft().y - 22.0f;
    float x = centerX - totalWidth * 0.5f;

    ImU32 white = IM_COL32(255, 255, 255, 255);
    ImVec4 base = isEnemy ? ImVec4(1.0f, 0.4f, 0.2f, 1.0f) : ImVec4(0.4f, 0.7f, 1.0f, 1.0f);
    ImU32 bracketColor = ImGui::ColorConvertFloat4ToU32(base);

    // Name (white)
    drawList->AddText(ImVec2(x, y), white, name);
    x += nameSize.x + spacing;

    // [distance] (white inside, colored brackets)
    drawList->AddText(nullptr, distFontSize, ImVec2(x, y), bracketColor, lBracket);
    x += lSize.x;
    drawList->AddText(ImVec2(x, y), white, distBuf);
    x += dSize.x;
    drawList->AddText(nullptr, distFontSize, ImVec2(x, y), bracketColor, rBracket);
}





void Player::DrawSnapline(ImDrawList* drawList) const {
    if (!settings.showSnaplines)
        return;

    ImVec2 start;

    switch (settings.snaplineOrigin) {
    case SnaplineOrigin::TopScreen:
        start = ImVec2(screenWidth / 2.0f, 0.0f);
        break;
    case SnaplineOrigin::BottomScreen:
        start = ImVec2(screenWidth / 2.0f, screenHeight);
        break;
    case SnaplineOrigin::TopFOV:
        start = ImVec2(screenWidth / 2.0f, screenHeight / 2.0f - settings.fovSize);
        break;
    case SnaplineOrigin::BottomFOV:
        start = ImVec2(screenWidth / 2.0f, screenHeight / 2.0f + settings.fovSize);
        break;
    }

    // Compute player box center X
    ImVec2 boxTop = GetBoxTopLeft();
    ImVec2 boxBottom = GetBoxBottomRight();
    float centerX = (boxBottom.x + boxTop.x) / 2.0f;

    // Snapline end: top or bottom of player box
    ImVec2 end;
    if (settings.snaplineOrigin == SnaplineOrigin::TopScreen || settings.snaplineOrigin == SnaplineOrigin::TopFOV)
        end = ImVec2(centerX, boxTop.y);  // Aim at top of player box
    else
        end = ImVec2(centerX, boxBottom.y); // Aim at bottom of player box

    drawList->AddLine(start, end, GetBoxColor(), isVisible ? 2.0f : 1.0f);
}


