#pragma once
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// ─── Structs básicas ──────────────────────────────────────────────────────────
struct Vector2 {
    float x, y;
    Vector2() : x(0), y(0) {}
    Vector2(float x, float y) : x(x), y(y) {}

    float Length() const { return std::sqrt(x*x + y*y); }
    Vector2 operator-(const Vector2& o) const { return {x-o.x, y-o.y}; }
};

struct Vector3 {
    float x, y, z;
    Vector3() : x(0), y(0), z(0) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    float Length() const { return std::sqrt(x*x + y*y + z*z); }
    float Distance(const Vector3& o) const {
        return Vector3(x-o.x, y-o.y, z-o.z).Length();
    }
    bool IsZero() const { return x == 0 && y == 0 && z == 0; }
    Vector3 operator+(const Vector3& o) const { return {x+o.x, y+o.y, z+o.z}; }
    Vector3 operator-(const Vector3& o) const { return {x-o.x, y-o.y, z-o.z}; }
    Vector3 operator*(float s)          const { return {x*s,   y*s,   z*s};   }
};

// Matrix 4x4 row-major (Unity usa column-major, mas ViewProjection é row-major)
struct Matrix4x4 {
    float m[4][4];
};

// ─── Ângulos de câmera ────────────────────────────────────────────────────────
struct Angles {
    float pitch; // Cima/baixo
    float yaw;   // Esquerda/direita
};

namespace AimMath {

    // ─── WorldToScreen ────────────────────────────────────────────────────────
    // Converte posição 3D do mundo para pixel na tela
    // vp = Camera.main.projectionMatrix * Camera.main.worldToCameraMatrix
    inline bool WorldToScreen(
        const Vector3&   worldPos,
        const Matrix4x4& vp,
        float            screenW,
        float            screenH,
        Vector2&         out)
    {
        float clipX = worldPos.x * vp.m[0][0] + worldPos.y * vp.m[1][0]
                    + worldPos.z * vp.m[2][0] + vp.m[3][0];
        float clipY = worldPos.x * vp.m[0][1] + worldPos.y * vp.m[1][1]
                    + worldPos.z * vp.m[2][1] + vp.m[3][1];
        float clipW = worldPos.x * vp.m[0][3] + worldPos.y * vp.m[1][3]
                    + worldPos.z * vp.m[2][3] + vp.m[3][3];

        if (clipW < 0.001f) return false; // Atrás da câmera

        float ndcX = clipX / clipW;
        float ndcY = clipY / clipW;

        out.x = (screenW * 0.5f) + (ndcX * screenW  * 0.5f);
        out.y = (screenH * 0.5f) - (ndcY * screenH  * 0.5f); // Y invertido no Android

        // Checa se está dentro da tela
        return (out.x >= 0 && out.x <= screenW &&
                out.y >= 0 && out.y <= screenH);
    }

    // ─── CalcAimAngles ────────────────────────────────────────────────────────
    // Retorna pitch/yaw necessários para mirar no targetPos vindo de myPos
    inline Angles CalcAimAngles(const Vector3& myPos, const Vector3& targetPos) {
        float dx = targetPos.x - myPos.x;
        float dy = targetPos.y - myPos.y;
        float dz = targetPos.z - myPos.z;
        float dist2D = std::sqrt(dx*dx + dz*dz);

        return {
            std::atan2(dy, dist2D) * (180.0f / (float)M_PI), // pitch
            std::atan2(dx, dz)     * (180.0f / (float)M_PI)  // yaw
        };
    }

    // ─── GetHeadPos ───────────────────────────────────────────────────────────
    // Estima posição da cabeça (Unity: body + ~1.75m no Y)
    inline Vector3 GetHeadPosition(const Vector3& bodyPos, float headOffset = 1.75f) {
        return { bodyPos.x, bodyPos.y + headOffset, bodyPos.z };
    }

    // ─── Distância entre dois pontos 2D (para aimbot: FOV check) ─────────────
    inline float Distance2D(const Vector2& a, const Vector2& b) {
        float dx = a.x - b.x, dy = a.y - b.y;
        return std::sqrt(dx*dx + dy*dy);
    }

    // ─── Lerp suave para smooth aimbot ────────────────────────────────────────
    inline float Lerp(float a, float b, float t) {
        return a + (b - a) * t;
    }
}
