#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <fstream>
#include <vector>
#include <android/log.h>

#define LOG_TAG "IL2CPP_MOD"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

class Memory {
public:
    static uintptr_t g_Il2CppBase;
    static uintptr_t g_UnityBase;

    // ─── Encontra o base address de um .so via /proc/self/maps ───────────────
    static uintptr_t GetBaseAddress(const char* moduleName) {
        std::ifstream maps("/proc/self/maps");
        std::string line;
        while (std::getline(maps, line)) {
            if (line.find(moduleName) != std::string::npos &&
                line.find("r-xp")    != std::string::npos)
            {
                uintptr_t base = 0;
                std::sscanf(line.c_str(), "%lx-", &base);
                LOGI("[Memory] %s base: 0x%lX", moduleName, base);
                return base;
            }
        }
        LOGE("[Memory] Modulo nao encontrado: %s", moduleName);
        return 0;
    }

    // ─── Leitura genérica (any type) ─────────────────────────────────────────
    template<typename T>
    static T Read(uintptr_t address) {
        if (!address) return T{};
        T val{};
        std::memcpy(&val, reinterpret_cast<void*>(address), sizeof(T));
        return val;
    }

    // ─── Escrita genérica ─────────────────────────────────────────────────────
    template<typename T>
    static bool Write(uintptr_t address, T value) {
        if (!address) return false;
        std::memcpy(reinterpret_cast<void*>(address), &value, sizeof(T));
        return true;
    }

    // ─── Pointer chain (multi-level deref) ───────────────────────────────────
    static uintptr_t ReadChain(uintptr_t base, const std::vector<uintptr_t>& offsets) {
        uintptr_t addr = Read<uintptr_t>(base);
        for (size_t i = 0; i < offsets.size() - 1; i++) {
            if (!addr) return 0;
            addr = Read<uintptr_t>(addr + offsets[i]);
        }
        return addr ? addr + offsets.back() : 0;
    }

    // ─── Inicializa todos os base addresses ──────────────────────────────────
    static bool Init() {
        g_Il2CppBase = GetBaseAddress("libil2cpp.so");
        g_UnityBase  = GetBaseAddress("libunity.so");
        return (g_Il2CppBase != 0);
    }
};

// Definição das variáveis estáticas (em Memory.cpp)
uintptr_t Memory::g_Il2CppBase = 0;
uintptr_t Memory::g_UnityBase  = 0;
