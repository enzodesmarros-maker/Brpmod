# 🎮 Android IL2CPP Mod — Unity Games

Stack: **KittyMemory · Dear ImGui · Dobby · NDK r25c · GitHub Actions**

---

## ✅ Features implementadas

| Feature | Arquivo | Status |
|---------|---------|--------|
| God Mode (patch TakeDamage) | `Hooks.h` | ✅ Pronto (falta offset) |
| Speed Hack (hook UpdateMovement) | `Hooks.h` | ✅ Pronto (falta offset) |
| No Recoil (patch + hook) | `Hooks.h` | ✅ Pronto (falta offset) |
| ESP / Caixas com vida | `ImGuiOverlay.h` | ✅ Pronto |
| Aimbot Legit (smooth + FOV) | `Aimbot.h` | ✅ Pronto |
| Silent Aim / Sandbox | `Aimbot.h` | ✅ Pronto (falta offset) |
| Aimbot Rage (snap headshot) | `Aimbot.h` | ✅ Pronto |
| Botão flutuante toggle menu | `ImGuiOverlay.h` | ✅ Pronto |
| WorldToScreen | `Math.h` | ✅ Pronto |

---

## 📁 Estrutura

```
app/src/main/cpp/
├── main.cpp              ← Ponto de entrada (__constructor)
├── Memory.h              ← Read/Write/GetBaseAddress
├── Math.h                ← Vector3, Matrix4x4, WorldToScreen
├── GameData.h            ← Offsets + Entity + GameManager
├── Hooks.h               ← KittyMemory patches + Dobby hooks
├── Aimbot.h              ← Legit/Silent/Rage aimbot
├── ImGuiOverlay.h        ← UI completa + ESP + eglSwapBuffers hook
├── Android.mk
├── Application.mk
└── libs/
    ├── KittyMemory/      ← git submodule
    ├── imgui/            ← git submodule
    └── Dobby/            ← .a pré-compilado + headers
```

---

## ⚙️ Setup

### 1. Clone com submódulos

```bash
git clone --recursive https://github.com/SEU_USER/SEU_REPO
```

Se já clonou sem `--recursive`:
```bash
git submodule update --init --recursive
```

### 2. Adicione as libs como submódulos

```bash
# KittyMemory
git submodule add https://github.com/MJx0/KittyMemory app/src/main/cpp/libs/KittyMemory

# Dear ImGui
git submodule add https://github.com/ocornut/imgui app/src/main/cpp/libs/imgui

# Coloque os .a do Dobby manualmente em:
# libs/Dobby/arm64-v8a/libdobby.a
# libs/Dobby/include/dobby.h
```

### 3. Preencha os offsets em `GameData.h`

Use **IL2CppDumper** + **IDA Pro** / **Ghidra** para extrair os offsets do seu jogo:

```bash
# IL2CppDumper
./IL2CppDumper libil2cpp.so global-metadata.dat output/
```

Substitua os `0xDEAD0001` etc. pelos offsets reais.

### 4. Build local

```bash
cd app/src/main/cpp
$NDK_ROOT/ndk-build NDK_PROJECT_PATH=. NDK_APPLICATION_MK=Application.mk -j8
```

### 5. Build via GitHub Actions

Push para `main` → Actions builda automaticamente → baixe o `.so` em **Artifacts**

---

## 🔧 Bytes ARM64 úteis

| Efeito | Bytes |
|--------|-------|
| `return true` | `20 00 80 52 C0 03 5F D6` |
| `return false` | `00 00 80 52 C0 03 5F D6` |
| `return 0` | `00 00 80 52 C0 03 5F D6` |
| Função vazia (NOP) | `C0 03 5F D6` |
| `return 1.0f` | `00 00 80 3F` (float patch) |
| `return 0.0f` | `00 00 00 00` (float patch) |

---

## 📌 Fluxo de execução

```
APK carrega libil2cpp.so (substituída)
    └── __attribute__((constructor)) OnLibLoad()
            └── pthread: ModThread()
                    ├── sleep(5) — aguarda jogo iniciar
                    ├── Memory::Init()          → obtém base addresses
                    ├── Hooks::InstallAll()     → Dobby hooks gameplay
                    ├── Aimbot::InstallSilentHook()
                    └── ImGuiOverlay::InstallHook() → hook eglSwapBuffers
                                └── A cada frame:
                                        ├── GameManager::Update()
                                        ├── ESP RenderESP()
                                        ├── Aimbot::Tick()
                                        └── ImGui RenderMenu()
```
