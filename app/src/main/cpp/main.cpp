#include <pthread.h>
#include <unistd.h>
#include <dlfcn.h>
#include <jni.h>
#include <string.h>
#include "Memory.h"
#include "GameData.h"
#include "Hooks.h"
#include "Aimbot.h"
#include "ImGuiOverlay.h"

// ═══════════════════════════════════════════════════════════════════════════════
// Hash SHA-256 do APK original — hardcoded para bypass da verificação
// ═══════════════════════════════════════════════════════════════════════════════
static const char* ORIGINAL_HASH = "9a1a03f7cb4b54b8868cefa61e03e184f09af4d50fd0197516e0499bde8c8cdb";

typedef void (*SendHash_t)(JNIEnv*, jobject, jstring);
SendHash_t orig_SendHash = nullptr;

void hk_SendHash(JNIEnv* env, jobject obj, jstring hash) {
    // Sempre envia o hash original independente do APK atual
    jstring fakeHash = env->NewStringUTF(ORIGINAL_HASH);
    orig_SendHash(env, obj, fakeHash);
    env->DeleteLocalRef(fakeHash);
    LOGI("[Hash] Hash original enviado: %s", ORIGINAL_HASH);
}

void InstallHashHook() {
    void* samp = dlopen("libsamp.so", RTLD_NOW | RTLD_NOLOAD);
    if (!samp) {
        LOGE("[Hash] libsamp.so nao encontrada");
        return;
    }
    void* sym = dlsym(samp, "Java_com_nvidia_devtech_NvEventQueueActivity_SendHash");
    if (!sym) {
        LOGE("[Hash] SendHash nao encontrado");
        return;
    }
    DobbyHook(sym, (void*)hk_SendHash, (void**)&orig_SendHash);
    LOGI("[Hash] Hook instalado com sucesso");
}

// ═══════════════════════════════════════════════════════════════════════════════
void* ModThread(void*) {
    sleep(3);
    LOGI("=== BRP MOD CARREGADO ===");

    // 1. Hook do hash PRIMEIRO — antes do jogo conectar ao servidor
    InstallHashHook();

    // 2. Base address da libGTASA
    uintptr_t gtasaBase = Memory::GetBaseAddress("libGTASA.so");
    if (!gtasaBase) {
        LOGE("[Main] libGTASA.so nao encontrada!");
        return nullptr;
    }
    Memory::g_Il2CppBase = gtasaBase;
    GameManager::InitFunctions(gtasaBase);

    uintptr_t sampBase = Memory::GetBaseAddress("libsamp.so");
    LOGI("[Main] GTASA base: 0x%lX", gtasaBase);
    LOGI("[Main] SAMP base:  0x%lX", sampBase);

    // 3. Hooks de gameplay
    Hooks::InstallAll();
    Aimbot::InstallSilentHook();

    // 4. ImGui overlay
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
