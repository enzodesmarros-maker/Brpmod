#pragma once
#include "Math.h"
#include "GameData.h"
#include "Hooks.h"

// ─── Tipos de aimbot ──────────────────────────────────────────────────────────
enum class AimbotMode {
    OFF,
    LEGIT,     // Suave, dentro do FOV — parece humano
    SILENT,    // Silent aim: bala vai no alvo sem mover a mira (sandbox)
    RAGE       // Snap instantâneo na cabeça
};

namespace Aimbot {

    static float   g_ScreenW    = 1080.f;
    static float   g_ScreenH    = 2340.f;
    static AimbotMode g_Mode    = AimbotMode::LEGIT;

    // Ângulos atuais da câmera/mira (preencher via hook ou leitura de memória)
    static Angles  g_CurrentAngles = {0, 0};

    // ─── Encontra o melhor alvo dentro do FOV ─────────────────────────────────
    static Entity* FindBestTarget() {
        if (!GameManager::localPlayer.IsValid()) return nullptr;

        Vector3 myPos  = GameManager::localPlayer.GetPosition();
        Vector2 center = { g_ScreenW * 0.5f, g_ScreenH * 0.5f };

        Entity* best    = nullptr;
        float   bestDist = State::aimbotFOV; // só mira dentro do FOV (pixels)

        for (int i = 0; i < GameManager::enemyCount; i++) {
            Entity& ent = GameManager::enemies[i];
            if (!ent.IsValid() || !ent.IsAlive()) continue;

            // Posição na tela — cabeça ou corpo
            Vector3 targetPos = State::aimbotHeadOnly
                ? ent.GetHeadPosition()
                : ent.GetPosition();

            Vector2 screenPos;
            bool onScreen = AimMath::WorldToScreen(
                targetPos, GameManager::viewMatrix,
                g_ScreenW, g_ScreenH, screenPos);

            if (!onScreen) continue;

            // Distância da mira até o alvo na tela
            float dist = AimMath::Distance2D(center, screenPos);
            if (dist < bestDist) {
                bestDist = dist;
                best     = &ent;
            }
        }

        return best;
    }

    // ─── LEGIT aim: retorna ângulos suavizados em direção ao alvo ─────────────
    static Angles CalcLegitAngles(Entity* target) {
        if (!target) return g_CurrentAngles;

        Vector3 myPos    = GameManager::localPlayer.GetPosition();
        Vector3 aimPoint = State::aimbotHeadOnly
            ? target->GetHeadPosition()
            : target->GetPosition();

        Angles desired = AimMath::CalcAimAngles(myPos, aimPoint);

        // Suavização (lerp)
        float smooth = State::aimbotSmooth;
        return {
            AimMath::Lerp(g_CurrentAngles.pitch, desired.pitch, smooth),
            AimMath::Lerp(g_CurrentAngles.yaw,   desired.yaw,   smooth)
        };
    }

    // ─── SILENT aim: teleporta a bala para o alvo (via hook de projectile) ────
    // Esse modo não move a câmera visualmente — parece sandbox
    static Vector3 CalcSilentTarget() {
        Entity* target = FindBestTarget();
        if (!target) return {};
        return State::aimbotHeadOnly
            ? target->GetHeadPosition()
            : target->GetPosition();
    }

    // ─── Tick principal — chame a cada frame ─────────────────────────────────
    static Angles Tick() {
        if (!State::aimbot || g_Mode == AimbotMode::OFF)
            return g_CurrentAngles;

        Entity* target = FindBestTarget();
        if (!target) return g_CurrentAngles;

        switch (g_Mode) {
            case AimbotMode::LEGIT:
                return CalcLegitAngles(target);

            case AimbotMode::RAGE: {
                // Snap direto sem suavização
                Vector3 myPos = GameManager::localPlayer.GetPosition();
                return AimMath::CalcAimAngles(myPos, target->GetHeadPosition());
            }

            case AimbotMode::SILENT:
                // Silent aim é aplicado no hook do projectile, não aqui
                // Aqui só mantemos os ângulos atuais
                return g_CurrentAngles;

            default:
                return g_CurrentAngles;
        }
    }

    // ─── Hook de projetil para Silent Aim ────────────────────────────────────
    // Redireciona a origem/destino do projetil para a cabeça do alvo
    typedef void (*FireProjectile_t)(void* weapon, Vector3* origin, Vector3* direction);
    FireProjectile_t orig_FireProjectile = nullptr;

    void hk_FireProjectile(void* weapon, Vector3* origin, Vector3* direction) {
        if (State::aimbot && g_Mode == AimbotMode::SILENT) {
            Vector3 silentTarget = CalcSilentTarget();
            if (!silentTarget.IsZero() && origin) {
                // Calcula nova direção para o alvo silencioso
                Vector3 dir = silentTarget - *origin;
                float len   = dir.Length();
                if (len > 0.001f) {
                    *direction = dir * (1.0f / len); // normaliza
                }
            }
        }
        orig_FireProjectile(weapon, origin, direction);
    }

    // ─── Instala hook de silent aim ───────────────────────────────────────────
    inline void InstallSilentHook() {
        uintptr_t base     = Memory::g_Il2CppBase;
        uintptr_t funcAddr = base + 0xDEAD0040; // TODO: offset de FireProjectile
        DobbyHook(
            reinterpret_cast<void*>(funcAddr),
            reinterpret_cast<void*>(hk_FireProjectile),
            reinterpret_cast<void**>(&orig_FireProjectile));
        LOGI("[Aimbot] Silent aim hook instalado");
    }
}
