#pragma once
#include "Memory.h"
#include "Math.h"

// ═══════════════════════════════════════════════════════════════════════════════
// Brasil Roleplay v12 — Offsets extraídos de libGTASA.so (arm64-v8a)
// Build: com_brp_game_34_ALFA | 2026-01-22
// Engine: GTA San Andreas (RenderWare) + SA-MP
// ═══════════════════════════════════════════════════════════════════════════════

namespace Offsets {

    // ── Funções (relativas ao base de libGTASA.so) ────────────────────────────
    constexpr uintptr_t FindPlayerPed       = 0x004EFAE0; // CPed* FindPlayerPed(int)
    constexpr uintptr_t FindPlayerHeight    = 0x004F0734; // float FindPlayerHeight()
    constexpr uintptr_t CPed_IsAlive        = 0x0059AFC8; // bool CPed::IsAlive()
    constexpr uintptr_t CPed_IsPlayer       = 0x00595400; // bool CPed::IsPlayer()
    constexpr uintptr_t CPed_SetAmmo        = 0x0059B918; // void CPed::SetAmmo(int, int)
    constexpr uintptr_t CPed_Teleport       = 0x0059DD90; // void CPed::Teleport(CVector*)
    constexpr uintptr_t CPed_GetBonePos     = 0x0059AEE4; // void CPed::GetBonePosition(RwV3d*,int,bool)
    constexpr uintptr_t CPlayerPed_Process  = 0x005C1170; // void CPlayerPed::ProcessControl()

    // ── Globais estáticos ─────────────────────────────────────────────────────
    constexpr uintptr_t CWorld_Players      = 0x00BDC738; // CPlayerInfo[4]
    constexpr uintptr_t CTimer_TimeStep     = 0x00BDC578; // float ms_fTimeStep
    constexpr uintptr_t CPools_PedPool      = 0x00981250; // CPedPool*

    // ── Struct CPed (offsets internos) ────────────────────────────────────────
    namespace Ped {
        constexpr uintptr_t fHealth         = 0x120;
        constexpr uintptr_t fMaxHealth      = 0x124;
        constexpr uintptr_t fArmour         = 0x168;
        constexpr uintptr_t nPedType        = 0x008;
        constexpr uintptr_t nCurrentWeapon  = 0x4BC;
        constexpr uintptr_t Placement       = 0x030; // CSimpleTransform (pos x,y,z + heading)
        constexpr int       BoneHead        = 8;
    }

    // ── Struct CVehicle (offsets internos) ────────────────────────────────────
    namespace Vehicle {
        constexpr uintptr_t fHealth         = 0x4C0;
        constexpr uintptr_t Placement       = 0x030;
        constexpr uintptr_t pDriver         = 0x5F0;
    }

    // ── CPlayerInfo dentro de CWorld::Players ─────────────────────────────────
    namespace PlayerInfo {
        constexpr uintptr_t pPed            = 0x00;
        constexpr uintptr_t nMoney          = 0xB8;
        constexpr uintptr_t strName         = 0x108;
    }
}

// ─── Tipos de função ──────────────────────────────────────────────────────────
typedef void*  (*FindPlayerPed_t)(int);
typedef bool   (*CPed_IsAlive_t)(void*);
typedef void   (*CPed_Teleport_t)(void*, Vector3*);
typedef void   (*CPed_SetAmmo_t)(void*, int, int);
typedef void   (*CPed_GetBonePos_t)(void*, Vector3*, int, bool);

static FindPlayerPed_t   fn_FindPlayerPed   = nullptr;
static CPed_IsAlive_t    fn_CPed_IsAlive    = nullptr;
static CPed_Teleport_t   fn_CPed_Teleport   = nullptr;
static CPed_SetAmmo_t    fn_CPed_SetAmmo    = nullptr;
static CPed_GetBonePos_t fn_CPed_GetBonePos = nullptr;

// ─── Entity ───────────────────────────────────────────────────────────────────
struct Entity {
    uintptr_t baseAddr = 0;
    bool IsValid() const { return baseAddr != 0; }

    Vector3 GetPosition() const {
        return Memory::Read<Vector3>(baseAddr + Offsets::Ped::Placement);
    }
    Vector3 GetHeadPosition() const {
        if (fn_CPed_GetBonePos) {
            Vector3 out{};
            fn_CPed_GetBonePos(reinterpret_cast<void*>(baseAddr), &out, Offsets::Ped::BoneHead, false);
            return out;
        }
        Vector3 p = GetPosition();
        return { p.x, p.y + 1.1f, p.z };
    }
    float GetHealth()  const { return Memory::Read<float>(baseAddr + Offsets::Ped::fHealth); }
    float GetArmour()  const { return Memory::Read<float>(baseAddr + Offsets::Ped::fArmour); }
    void  SetHealth(float v) {
        Memory::Write<float>(baseAddr + Offsets::Ped::fHealth, v);
        Memory::Write<float>(baseAddr + Offsets::Ped::fMaxHealth, v);
    }
    void  SetArmour(float v) { Memory::Write<float>(baseAddr + Offsets::Ped::fArmour, v); }
    bool  IsAlive() const {
        return fn_CPed_IsAlive
            ? fn_CPed_IsAlive(reinterpret_cast<void*>(baseAddr))
            : GetHealth() > 0;
    }
    void Teleport(const Vector3& pos) {
        if (fn_CPed_Teleport)
            fn_CPed_Teleport(reinterpret_cast<void*>(baseAddr), const_cast<Vector3*>(&pos));
    }
    void GiveWeapon(int id, int ammo) {
        if (fn_CPed_SetAmmo)
            fn_CPed_SetAmmo(reinterpret_cast<void*>(baseAddr), id, ammo);
    }
};

// ─── GameManager ─────────────────────────────────────────────────────────────
struct GameManager {
    static Entity    localPlayer;
    static Entity    enemies[128];
    static int       enemyCount;
    static Matrix4x4 viewMatrix;
    static uintptr_t gtasaBase;

    static void InitFunctions(uintptr_t base) {
        gtasaBase = base;
        fn_FindPlayerPed   = (FindPlayerPed_t)  (base + Offsets::FindPlayerPed);
        fn_CPed_IsAlive    = (CPed_IsAlive_t)   (base + Offsets::CPed_IsAlive);
        fn_CPed_Teleport   = (CPed_Teleport_t)  (base + Offsets::CPed_Teleport);
        fn_CPed_SetAmmo    = (CPed_SetAmmo_t)   (base + Offsets::CPed_SetAmmo);
        fn_CPed_GetBonePos = (CPed_GetBonePos_t)(base + Offsets::CPed_GetBonePos);
        LOGI("[GameData] Base libGTASA: 0x%lX — funcoes OK", base);
    }

    static void Update() {
        if (!fn_FindPlayerPed) return;
        void* ped = fn_FindPlayerPed(0);
        localPlayer.baseAddr = reinterpret_cast<uintptr_t>(ped);
        enemyCount = 0; // SA-MP: busca peds via CPedPool futuramente
    }
};

Entity    GameManager::localPlayer;
Entity    GameManager::enemies[128];
int       GameManager::enemyCount  = 0;
Matrix4x4 GameManager::viewMatrix  = {};
uintptr_t GameManager::gtasaBase   = 0;
