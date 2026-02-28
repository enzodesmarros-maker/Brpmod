#include <pthread.h>
#include <unistd.h>
#include <dlfcn.h>
#include "Memory.h"
#include "GameData.h"
#include "Hooks.h"
#include "Aimbot.h"
#include "ImGuiOverlay.h"

void* ModThread(void*) {
    sleep(5); // Aguarda GTA SA carregar completamente
    LOGI("=== BRP MOD CARREGADO ===");

    // 1. Base address da libGTASA (engine real do jogo)
    uintptr_t gtasaBase = Memory::GetBaseAddress("libGTASA.so");
    if (!gtasaBase) {
        LOGE("[Main] libGTASA.so nao encontrada!");
        return nullptr;
    }
    Memory::g_Il2CppBase = gtasaBase; // reutiliza o campo para GTA SA
    GameManager::InitFunctions(gtasaBase);

    // 2. Log das libs disponíveis
    uintptr_t sampBase  = Memory::GetBaseAddress("libsamp.so");
    uintptr_t cleoBase  = Memory::GetBaseAddress("libCLEOMod.so");
    LOGI("[Main] SAMP base:  0x%lX", sampBase);
    LOGI("[Main] CLEO base:  0x%lX", cleoBase);

    // 3. Hooks de gameplay
    Hooks::InstallAll();
    Aimbot::InstallSilentHook();

    // 4. ImGui via hook do eglSwapBuffers
    ImGuiOverlay::InstallHook();

    LOGI("[Main] Tudo pronto!");
    return nullptr;
}

__attribute__((constructor))
void OnLibLoad() {
    pthread_t thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&thread, &attr, ModThread, nullptr);
    pthread_attr_destroy(&attr);
}

__attribute__((destructor))
void OnLibUnload() {
    ImGuiOverlay::Shutdown();
    LOGI("[Main] BRP Mod descarregado.");
}
