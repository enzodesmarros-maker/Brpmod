#pragma once
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"
#include "Hooks.h"
#include "GameData.h"
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android/native_window.h>
#include <android/log.h>

namespace ImGuiOverlay {
    static bool g_initialized = false;
    static EGLDisplay g_display = EGL_NO_DISPLAY;

    inline void Init(ANativeWindow* window) {
        if (g_initialized) return;
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(1080, 1920);

        g_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        ImGui_ImplAndroid_Init(window);
        ImGui_ImplOpenGL3_Init("#version 300 es");

        ImGui::StyleColorsDark();
        g_initialized = true;
        LOGI("[ImGui] Inicializado");
    }

    inline void Render() {
        if (!g_initialized) return;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplAndroid_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_Once);
        ImGui::SetNextWindowPos(ImVec2(10, 100), ImGuiCond_Once);

        ImGui::Begin("BRP Mod", nullptr, ImGuiWindowFlags_NoCollapse);

        ImGui::Text("== Jogador ==");
        if (ImGui::Checkbox("God Mode", &State::godMode))
            Patches::ApplyGodMode(State::godMode);

        ImGui::Checkbox("Speed Hack", &State::speedHack);
        if (State::speedHack)
            ImGui::SliderFloat("Speed", &State::speedMultiplier, 1.0f, 5.0f);

        ImGui::Separator();
        ImGui::Text("== Visual ==");
        ImGui::Checkbox("ESP", &State::esp);
        ImGui::Checkbox("Aimbot", &State::aimbot);
        if (State::aimbot) {
            ImGui::Checkbox("Head Only", &State::aimbotHeadOnly);
            ImGui::SliderFloat("FOV", &State::aimbotFOV, 50.0f, 300.0f);
            ImGui::SliderFloat("Smooth", &State::aimbotSmooth, 0.0f, 1.0f);
        }

        ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    inline void Shutdown() {
        if (!g_initialized) return;
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplAndroid_Shutdown();
        ImGui::DestroyContext();
        g_initialized = false;
    }
}

namespace ImGuiOverlay {
    // Hook do eglSwapBuffers para renderizar ImGui a cada frame
    typedef EGLBoolean (*eglSwapBuffers_t)(EGLDisplay, EGLSurface);
    eglSwapBuffers_t orig_eglSwapBuffers = nullptr;

    EGLBoolean hk_eglSwapBuffers(EGLDisplay display, EGLSurface surface) {
        Render();
        return orig_eglSwapBuffers(display, surface);
    }

    inline void InstallHook() {
        void* egl = dlopen("libEGL.so", RTLD_NOW);
        if (!egl) { LOGE("[ImGui] libEGL nao encontrada"); return; }
        void* sym = dlsym(egl, "eglSwapBuffers");
        if (!sym) { LOGE("[ImGui] eglSwapBuffers nao encontrado"); return; }
        DobbyHook(sym, (void*)hk_eglSwapBuffers, (void**)&orig_eglSwapBuffers);
        LOGI("[ImGui] Hook eglSwapBuffers instalado");
    }
}
