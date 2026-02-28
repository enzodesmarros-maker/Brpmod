#pragma once
#include "Memory.h"
#include "GameData.h"
#include "KittyMemory.hpp"
#include "dobby.h"

// ═══════════════════════════════════════════════════════════════════════════════
// Flags de estado (controladas pela UI)
// ═══════════════════════════════════════════════════════════════════════════════
namespace State {
    bool godMode        = false;
    bool speedHack      = false;
    bool noRecoil       = false;
    bool aimbot         = false;
    bool esp            = false;
    bool aimbotHeadOnly = true;
    float speedMultiplier = 2.0f;
    float aimbotFOV       = 150.0f; // pixels
    float aimbotSmooth    = 0.35f;  // 0=snap, 1=muito suave
}

// ─── Patches guardados para poder reverter ────────────────────────────────────
namespace Patches {
    MemoryPatch patch_GodMode;
    MemoryPatch patch_NoRecoil;
    MemoryPatch patch_NoSpread;

    // ── God Mode: Patcha a função TakeDamage para não fazer nada ─────────────
    // ARM64: RET → C0 03 5F D6
    inline void ApplyGodMode(bool enable) {
        uintptr_t base = Memory::g_Il2CppBase;
        // TODO: substituir 0xDEAD0001 pelo offset de TakeDamage
        uintptr_t funcAddr = base + 0xDEAD0001;

        if (enable) {
            patch_GodMode = MemoryPatch::createWithHex(funcAddr, "C0 03 5F D6");
            patch_GodMode.Modify();
            LOGI("[Patch] GodMode ON");
        } else {
            patch_GodMode.Restore();
            LOGI("[Patch] GodMode OFF");
        }
    }

    // ── No Recoil: Zera o recoil amount e pattern ─────────────────────────────
    inline void ApplyNoRecoil(bool enable) {
        uintptr_t base = Memory::g_Il2CppBase;
        // TODO: substituir offsets
        uintptr_t recoilFunc = base + 0xDEAD0002;

        if (enable) {
            // MOV W0, #0 ; RET
            patch_NoRecoil = MemoryPatch::createWithHex(recoilFunc, "00 00 80 52 C0 03 5F D6");
            patch_NoRecoil.Modify();

            // Também zera via memória direta
            if (GameManager::localPlayer.IsValid()) {
                uintptr_t weaponAddr = Memory::Read<uintptr_t>(
                    GameManager::localPlayer.baseAddr + Offsets::CurrentWeapon);
                if (weaponAddr) {
                    Memory::Write<float>(weaponAddr + Offsets::RecoilAmount, 0.0f);
                    Memory::Write<float>(weaponAddr + Offsets::Spread, 0.0f);
                }
            }
            LOGI("[Patch] NoRecoil ON");
        } else {
            patch_NoRecoil.Restore();
            LOGI("[Patch] NoRecoil OFF");
        }
    }
}

// ─── Dobby Hooks ──────────────────────────────────────────────────────────────
namespace Hooks {

    // ── Hook: Update de movimento (Speed Hack) ────────────────────────────────
    typedef void (*UpdateMovement_t)(void* self);
    UpdateMovement_t orig_UpdateMovement = nullptr;

    void hk_UpdateMovement(void* self) {
        orig_UpdateMovement(self); // Chama original primeiro

        if (State::speedHack && self) {
            uintptr_t addr = reinterpret_cast<uintptr_t>(self);
            float origSpeed = Memory::Read<float>(addr + Offsets::MoveSpeed);
            // Aplica multiplicador sem quebrar a lógica do jogo
            Memory::Write<float>(addr + Offsets::MoveSpeed,
                                 origSpeed * State::speedMultiplier);
        }
    }

    // ── Hook: IsPlayerAlive → sempre true (God Mode alternativo) ─────────────
    typedef bool (*IsAlive_t)(void* self);
    IsAlive_t orig_IsAlive = nullptr;

    bool hk_IsAlive(void* self) {
        if (State::godMode) return true;
        return orig_IsAlive(self);
    }

    // ── Hook: GetRecoil → sempre 0 ────────────────────────────────────────────
    typedef float (*GetRecoil_t)(void* self);
    GetRecoil_t orig_GetRecoil = nullptr;

    float hk_GetRecoil(void* self) {
        if (State::noRecoil) return 0.0f;
        return orig_GetRecoil(self);
    }

    // ── Instala todos os hooks ────────────────────────────────────────────────
    inline void InstallAll() {
        uintptr_t base = Memory::g_Il2CppBase;

        // TODO: preencha os offsets corretos do seu jogo
        auto hookFunc = [&](uintptr_t offset, void* hook, void** orig, const char* name) {
            void* target = reinterpret_cast<void*>(base + offset);
            int result = DobbyHook(target, hook, orig);
            if (result == 0)
                LOGI("[Hook] %s instalado em 0x%lX", name, base + offset);
            else
                LOGE("[Hook] Falha em %s (err=%d)", name, result);
        };

        hookFunc(0xDEAD0010, (void*)hk_UpdateMovement,
                 (void**)&orig_UpdateMovement, "UpdateMovement");

        hookFunc(0xDEAD0020, (void*)hk_IsAlive,
                 (void**)&orig_IsAlive, "IsAlive");

        hookFunc(0xDEAD0030, (void*)hk_GetRecoil,
                 (void**)&orig_GetRecoil, "GetRecoil");

        LOGI("[Hooks] Todos instalados!");
    }
}
