#pragma once
#include "Memory.h"
#include "GameData.h"
#include "KittyMemory.hpp"
#include "dobby.h"

// MemoryPatch — tenta .hpp primeiro, fallback para .h
#if __has_include("MemoryPatch.hpp")
  #include "MemoryPatch.hpp"
#elif __has_include("MemoryPatch.h")
  #include "MemoryPatch.h"
#endif

namespace State {
    bool godMode        = false;
    bool speedHack      = false;
    bool noRecoil       = false;
    bool aimbot         = false;
    bool esp            = false;
    bool aimbotHeadOnly = true;
    float speedMultiplier = 2.0f;
    float aimbotFOV       = 150.0f;
    float aimbotSmooth    = 0.35f;
}

namespace Patches {
    MemoryPatch patch_GodMode;
    MemoryPatch patch_NoRecoil;

    inline void ApplyGodMode(bool enable) {
        uintptr_t base = Memory::g_Il2CppBase;
        uintptr_t funcAddr = base + Offsets::Funcs::CPed_IsAlive;
        if (enable) {
            patch_GodMode = MemoryPatch::createWithHex(funcAddr, "20 00 80 52 C0 03 5F D6");
            patch_GodMode.Modify();
            LOGI("[Patch] GodMode ON");
        } else {
            patch_GodMode.Restore();
            LOGI("[Patch] GodMode OFF");
        }
    }
}

namespace Hooks {
    typedef void (*ProcessControl_t)(void* self);
    ProcessControl_t orig_ProcessControl = nullptr;

    void hk_ProcessControl(void* self) {
        orig_ProcessControl(self);
        if (State::speedHack && self) {
            uintptr_t addr = reinterpret_cast<uintptr_t>(self);
            float spd = Memory::Read<float>(addr + 0x1C);
            Memory::Write<float>(addr + 0x1C, spd * State::speedMultiplier);
        }
    }

    inline void InstallAll() {
        uintptr_t base = Memory::g_Il2CppBase;
        void* target = reinterpret_cast<void*>(base + Offsets::Funcs::CPlayerPed_ProcessControl);
        DobbyHook(target, (void*)hk_ProcessControl, (void**)&orig_ProcessControl);
        LOGI("[Hooks] Instalados!");
    }
}
