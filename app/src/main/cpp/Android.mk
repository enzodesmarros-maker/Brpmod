LOCAL_PATH := $(call my-dir)

# ═══════════════════════════════════════════════════════════════════════════════
# 1. Dobby — biblioteca estática pré-compilada
# ═══════════════════════════════════════════════════════════════════════════════
include $(CLEAR_VARS)
LOCAL_MODULE            := dobby
LOCAL_SRC_FILES         := libs/Dobby/$(TARGET_ARCH_ABI)/libdobby.a
include $(PREBUILT_STATIC_LIBRARY)

# ═══════════════════════════════════════════════════════════════════════════════
# 2. Módulo principal do mod
# ═══════════════════════════════════════════════════════════════════════════════
include $(CLEAR_VARS)

LOCAL_MODULE := libmod

LOCAL_SRC_FILES := \
    main.cpp \
    libs/KittyMemory/KittyMemory.cpp      \
    libs/KittyMemory/MemoryPatch.cpp      \
    libs/imgui/imgui.cpp                   \
    libs/imgui/imgui_draw.cpp              \
    libs/imgui/imgui_tables.cpp            \
    libs/imgui/imgui_widgets.cpp           \
    libs/imgui/backends/imgui_impl_android.cpp \
    libs/imgui/backends/imgui_impl_opengl3.cpp

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)                      \
    $(LOCAL_PATH)/libs/KittyMemory     \
    $(LOCAL_PATH)/libs/imgui           \
    $(LOCAL_PATH)/libs/imgui/backends  \
    $(LOCAL_PATH)/libs/Dobby/include

LOCAL_CPPFLAGS := \
    -std=c++17          \
    -O2                 \
    -fvisibility=hidden \
    -fexceptions        \
    -frtti              \
    -DIMGUI_IMPL_OPENGL_ES3 \
    -DANDROID

# ─── Libs do sistema (OBRIGATÓRIAS) ───────────────────────────────────────────
LOCAL_LDLIBS := \
    -llog       \
    -lGLESv3    \
    -lEGL       \
    -landroid   \
    -ldl        \
    -lz

LOCAL_STATIC_LIBRARIES := dobby

include $(BUILD_SHARED_LIBRARY)
