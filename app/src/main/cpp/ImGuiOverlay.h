#pragma once
#include "imgui/imgui.h"
#include "imgui/imgui_impl_android.h"
#include "imgui/imgui_impl_opengl3.h"
#include "Hooks.h"
#include "Aimbot.h"
#include "GameData.h"
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android/native_window.h>

// ─── EGL State ────────────────────────────────────────────────────────────────
static EGLDisplay g_EglDisplay = EGL_NO_DISPLAY;
static EGLSurface g_EglSurface = EGL_NO_SURFACE;
static EGLContext g_EglContext  = EGL_NO_CONTEXT;
static bool       g_ImGuiReady  = false;
static bool       g_MenuVisible = true;

// Hook do eglSwapBuffers (chamado todo frame)
typedef EGLBoolean (*eglSwapBuffers_t)(EGLDisplay, EGLSurface);
eglSwapBuffers_t orig_eglSwapBuffers = nullptr;

namespace ImGuiOverlay {

    // ─── Cores do tema ────────────────────────────────────────────────────────
    static void ApplyTheme() {
        ImGuiStyle& s = ImGui::GetStyle();
        s.WindowRounding    = 10.0f;
        s.FrameRounding     = 5.0f;
        s.GrabRounding      = 4.0f;
        s.WindowBorderSize  = 1.0f;
        s.FrameBorderSize   = 0.0f;
        s.WindowPadding     = {12, 12};
        s.FramePadding      = {8, 4};
        s.ItemSpacing       = {8, 6};

        ImVec4* c = s.Colors;
        c[ImGuiCol_WindowBg]        = ImVec4(0.08f, 0.08f, 0.10f, 0.92f);
        c[ImGuiCol_TitleBg]         = ImVec4(0.18f, 0.00f, 0.40f, 1.00f);
        c[ImGuiCol_TitleBgActive]   = ImVec4(0.28f, 0.00f, 0.60f, 1.00f);
        c[ImGuiCol_Button]          = ImVec4(0.25f, 0.00f, 0.55f, 1.00f);
        c[ImGuiCol_ButtonHovered]   = ImVec4(0.35f, 0.10f, 0.70f, 1.00f);
        c[ImGuiCol_ButtonActive]    = ImVec4(0.45f, 0.15f, 0.85f, 1.00f);
        c[ImGuiCol_CheckMark]       = ImVec4(0.60f, 0.20f, 1.00f, 1.00f);
        c[ImGuiCol_SliderGrab]      = ImVec4(0.50f, 0.10f, 0.90f, 1.00f);
        c[ImGuiCol_SliderGrabActive]= ImVec4(0.70f, 0.20f, 1.00f, 1.00f);
        c[ImGuiCol_FrameBg]         = ImVec4(0.15f, 0.05f, 0.25f, 1.00f);
        c[ImGuiCol_FrameBgHovered]  = ImVec4(0.20f, 0.08f, 0.35f, 1.00f);
        c[ImGuiCol_Header]          = ImVec4(0.25f, 0.05f, 0.50f, 1.00f);
        c[ImGuiCol_HeaderHovered]   = ImVec4(0.35f, 0.10f, 0.65f, 1.00f);
        c[ImGuiCol_Separator]       = ImVec4(0.40f, 0.10f, 0.70f, 0.60f);
        c[ImGuiCol_Text]            = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    }

    // ─── Init ImGui ───────────────────────────────────────────────────────────
    inline void Init(ANativeWindow* window) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename  = nullptr;  // Sem salvar layout
        io.FontGlobalScale = 1.4f; // Fonte maior (tela de celular)

        ApplyTheme();
        ImGui_ImplAndroid_Init(window);
        ImGui_ImplOpenGL3_Init("#version 300 es");

        g_ImGuiReady = true;
        LOGI("[ImGui] Inicializado!");
    }

    // ─── Renderiza a janela de toggle (botão flutuante pequeno) ──────────────
    static void RenderToggleButton() {
        ImGui::SetNextWindowSize({60, 60}, ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.7f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {2, 2});
        ImGui::Begin("##toggle", nullptr,
                     ImGuiWindowFlags_NoDecoration |
                     ImGuiWindowFlags_NoScrollbar  |
                     ImGuiWindowFlags_AlwaysAutoResize);

        if (ImGui::Button(g_MenuVisible ? "X" : "M", {54, 54}))
            g_MenuVisible = !g_MenuVisible;

        ImGui::PopStyleVar();
        ImGui::End();
    }

    // ─── Render ESP overlay (caixas sobre inimigos) ───────────────────────────
    static void RenderESP(ImDrawList* draw) {
        if (!State::esp) return;

        float sw = Aimbot::g_ScreenW;
        float sh = Aimbot::g_ScreenH;

        for (int i = 0; i < GameManager::enemyCount; i++) {
            Entity& ent = GameManager::enemies[i];
            if (!ent.IsValid() || !ent.IsAlive()) continue;

            // Projeta corpo e cabeça na tela
            Vector2 screenBody, screenHead;
            bool bodyOk = AimMath::WorldToScreen(
                ent.GetPosition(), GameManager::viewMatrix, sw, sh, screenBody);
            bool headOk = AimMath::WorldToScreen(
                ent.GetHeadPosition(), GameManager::viewMatrix, sw, sh, screenHead);

            if (!bodyOk || !headOk) continue;

            float height = screenBody.y - screenHead.y;
            if (height < 5) continue;
            float width  = height * 0.4f;

            float x1 = screenHead.x - width;
            float y1 = screenHead.y;
            float x2 = screenHead.x + width;
            float y2 = screenBody.y;

            // Cor da caixa baseada na vida
            float hp      = ent.GetHealth();
            float hpRatio = hp / 100.0f;
            ImU32 boxColor = IM_COL32(
                (int)(255 * (1 - hpRatio)),
                (int)(255 * hpRatio),
                0, 220);

            // Caixa ESP
            draw->AddRect({x1, y1}, {x2, y2}, boxColor, 0, 0, 1.5f);

            // Linha de distância / barra de vida
            float hpBarH = height * hpRatio;
            draw->AddRectFilled(
                {x1 - 4, y2 - hpBarH},
                {x1 - 1, y2},
                boxColor);

            // Nome / distância (texto)
            float dist = GameManager::localPlayer.GetPosition()
                             .Distance(ent.GetPosition());
            char label[32];
            std::snprintf(label, sizeof(label), "%.0fm | %.0fhp", dist, hp);
            draw->AddText({x1, y1 - 14}, IM_COL32(255,255,255,200), label);

            // Ponto na cabeça (aimbot alvo)
            draw->AddCircleFilled({headOk ? screenHead.x : screenBody.x,
                                   headOk ? screenHead.y : screenBody.y},
                                  3.0f, IM_COL32(255, 50, 50, 255));
        }
    }

    // ─── Janela principal do menu ─────────────────────────────────────────────
    static void RenderMenu() {
        if (!g_MenuVisible) return;

        ImGui::SetNextWindowSize({310, 420}, ImGuiCond_Once);
        ImGui::SetNextWindowPos({80, 20},   ImGuiCond_Once);
        ImGui::Begin("  MOD MENU", nullptr,
                     ImGuiWindowFlags_NoScrollbar |
                     ImGuiWindowFlags_NoCollapse);

        // ── Info header ──────────────────────────────────────────────────────
        Entity& lp = GameManager::localPlayer;
        if (lp.IsValid()) {
            ImGui::TextColored({0.4f,1,0.4f,1},
                "HP: %.0f  |  Pos: (%.0f, %.0f, %.0f)",
                lp.GetHealth(),
                lp.GetPosition().x,
                lp.GetPosition().y,
                lp.GetPosition().z);
        } else {
            ImGui::TextColored({1,0.4f,0.4f,1}, "Player nao encontrado");
        }
        ImGui::Separator();

        // ── Tab bar ───────────────────────────────────────────────────────────
        if (ImGui::BeginTabBar("tabs")) {

            // ── Aba: Player ──────────────────────────────────────────────────
            if (ImGui::BeginTabItem("Player")) {
                if (ImGui::Checkbox("God Mode", &State::godMode))
                    Patches::ApplyGodMode(State::godMode);

                if (ImGui::Checkbox("Speed Hack", &State::speedHack)) {}
                if (State::speedHack)
                    ImGui::SliderFloat("Speed x", &State::speedMultiplier, 1.0f, 6.0f);

                ImGui::EndTabItem();
            }

            // ── Aba: Combat ──────────────────────────────────────────────────
            if (ImGui::BeginTabItem("Combat")) {
                if (ImGui::Checkbox("No Recoil", &State::noRecoil))
                    Patches::ApplyNoRecoil(State::noRecoil);

                ImGui::Separator();
                ImGui::Checkbox("Aimbot", &State::aimbot);

                if (State::aimbot) {
                    // Modo
                    int mode = (int)Aimbot::g_Mode;
                    const char* modes[] = {"OFF", "Legit", "Silent Aim", "Rage"};
                    if (ImGui::Combo("Modo", &mode, modes, 4))
                        Aimbot::g_Mode = (AimbotMode)mode;

                    ImGui::Checkbox("Head Only", &State::aimbotHeadOnly);
                    ImGui::SliderFloat("FOV",    &State::aimbotFOV,    30.f, 400.f);
                    ImGui::SliderFloat("Smooth", &State::aimbotSmooth, 0.01f, 1.0f);
                }
                ImGui::EndTabItem();
            }

            // ── Aba: Visuals ─────────────────────────────────────────────────
            if (ImGui::BeginTabItem("Visuals")) {
                ImGui::Checkbox("ESP / Caixas", &State::esp);

                if (State::esp) {
                    ImGui::TextDisabled("Mostra caixa, vida e distancia");
                    ImGui::TextDisabled("dos inimigos na tela.");
                }
                ImGui::EndTabItem();
            }

            // ── Aba: Info ────────────────────────────────────────────────────
            if (ImGui::BeginTabItem("Info")) {
                ImGui::Text("Build: %s", __DATE__);
                ImGui::Text("il2cpp base: 0x%lX", Memory::g_Il2CppBase);
                ImGui::Text("unity base:  0x%lX", Memory::g_UnityBase);
                ImGui::Text("Enemies: %d", GameManager::enemyCount);
                ImGui::Separator();
                ImGui::TextColored({0.6f,0.6f,0.6f,1},
                    "Preencha os offsets em\nGameData.h antes de usar!");
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
        ImGui::End();
    }

    // ─── Frame principal — chamado no hook do eglSwapBuffers ─────────────────
    inline void Frame() {
        if (!g_ImGuiReady) return;

        // Atualiza dados do jogo
        GameManager::Update();

        // Novo frame ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplAndroid_NewFrame();
        ImGui::NewFrame();

        // ESP (background layer)
        ImDrawList* draw = ImGui::GetBackgroundDrawList();
        RenderESP(draw);

        // FOV circle do aimbot
        if (State::aimbot) {
            draw->AddCircle(
                {Aimbot::g_ScreenW * 0.5f, Aimbot::g_ScreenH * 0.5f},
                State::aimbotFOV,
                IM_COL32(255, 255, 255, 60), 64, 1.0f);
        }

        // Menu + botão flutuante
        RenderToggleButton();
        RenderMenu();

        // Render
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    // ─── Hook do eglSwapBuffers ───────────────────────────────────────────────
    EGLBoolean hk_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
        static bool inited = false;
        if (!inited) {
            inited = true;
            Init(nullptr); // Inicializa ImGui na primeira chamada
        }
        Frame();
        return orig_eglSwapBuffers(dpy, surface);
    }

    // ─── Instala o hook do eglSwapBuffers ────────────────────────────────────
    inline void InstallHook() {
        void* eglSwap = dlsym(dlopen("libEGL.so", RTLD_NOW), "eglSwapBuffers");
        if (eglSwap) {
            DobbyHook(eglSwap,
                      reinterpret_cast<void*>(hk_eglSwapBuffers),
                      reinterpret_cast<void**>(&orig_eglSwapBuffers));
            LOGI("[ImGui] Hook eglSwapBuffers instalado");
        } else {
            LOGE("[ImGui] Falha ao encontrar eglSwapBuffers");
        }
    }

    inline void Shutdown() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplAndroid_Shutdown();
        ImGui::DestroyContext();
    }
}
