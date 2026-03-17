#pragma once
#include <cmath>
#include <vector>
#include <algorithm>
#include <optional>

// ═══════════════════════════════════════════════════════════════
//  A minimal, educational raytracer
// ═══════════════════════════════════════════════════════════════

struct Vec3 {
    float x = 0, y = 0, z = 0;
    Vec3() = default;
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vec3 operator+(const Vec3& o) const { return {x+o.x, y+o.y, z+o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x-o.x, y-o.y, z-o.z}; }
    Vec3 operator*(float s) const { return {x*s, y*s, z*s}; }
    Vec3 operator*(const Vec3& o) const { return {x*o.x, y*o.y, z*o.z}; } // component
    Vec3& operator+=(const Vec3& o) { x+=o.x; y+=o.y; z+=o.z; return *this; }

    float dot(const Vec3& o) const { return x*o.x + y*o.y + z*o.z; }
    float length() const { return std::sqrt(dot(*this)); }
    Vec3 normalized() const { float l = length(); return l > 1e-8f ? Vec3{x/l,y/l,z/l} : Vec3{}; }
    Vec3 reflect(const Vec3& n) const { return *this - n * (2.0f * dot(n)); }

    static Vec3 lerp(const Vec3& a, const Vec3& b, float t) {
        return a * (1-t) + b * t;
    }
};

struct Ray {
    Vec3 origin, direction;
    Vec3 at(float t) const { return origin + direction * t; }
};

// ─── Materials ──────────────────────────────────────────────────
struct Material {
    Vec3  color       = {1, 1, 1};
    float reflectivity = 0.0f;   // 0 = matte, 1 = mirror
    float specular     = 30.0f;  // specular exponent
    float specStrength = 0.5f;   // specular intensity
};

// ─── Hit record ─────────────────────────────────────────────────
struct Hit {
    float    t;
    Vec3     point;
    Vec3     normal;
    Material material;
};

// ─── Sphere ─────────────────────────────────────────────────────
struct Sphere {
    Vec3     center;
    float    radius;
    Material material;

    std::optional<Hit> intersect(const Ray& ray) const {
        Vec3 oc = ray.origin - center;
        float a = ray.direction.dot(ray.direction);
        float b = 2.0f * oc.dot(ray.direction);
        float c = oc.dot(oc) - radius * radius;
        float disc = b*b - 4*a*c;
        if (disc < 0) return std::nullopt;
        float t = (-b - std::sqrt(disc)) / (2*a);
        if (t < 0.001f) {
            t = (-b + std::sqrt(disc)) / (2*a);
            if (t < 0.001f) return std::nullopt;
        }
        Vec3 p = ray.at(t);
        Vec3 n = (p - center).normalized();
        return Hit{t, p, n, material};
    }
};

// ─── Infinite checkerboard floor ────────────────────────────────
struct Floor {
    float y = 0.0f;
    Material mat1, mat2;
    float scale = 1.0f;

    std::optional<Hit> intersect(const Ray& ray) const {
        if (std::abs(ray.direction.y) < 1e-6f) return std::nullopt;
        float t = (y - ray.origin.y) / ray.direction.y;
        if (t < 0.001f) return std::nullopt;
        Vec3 p = ray.at(t);
        // Checkerboard pattern
        int cx = static_cast<int>(std::floor(p.x * scale));
        int cz = static_cast<int>(std::floor(p.z * scale));
        bool white = ((cx + cz) & 1) == 0;
        return Hit{t, p, {0, 1, 0}, white ? mat1 : mat2};
    }
};

// ─── Scene ──────────────────────────────────────────────────────
struct Scene {
    std::vector<Sphere> spheres;
    Floor floor;
    Vec3 lightDir   = Vec3{-0.5f, -1.0f, -0.7f}.normalized();
    Vec3 lightColor = {1.0f, 0.95f, 0.9f};
    Vec3 ambient    = {0.08f, 0.1f, 0.15f};
    int  maxBounces = 4;

    Vec3 skyColor(const Ray& ray) const {
        float t = 0.5f * (ray.direction.y + 1.0f);
        t = std::clamp(t, 0.0f, 1.0f);
        Vec3 horizon = {0.8f, 0.85f, 0.95f};
        Vec3 zenith  = {0.15f, 0.3f, 0.7f};
        // Sun glow
        float sun = std::max(0.0f, ray.direction.dot(lightDir * -1.0f));
        Vec3 sunGlow = Vec3{1.0f, 0.8f, 0.4f} * std::pow(sun, 64.0f) * 2.0f;
        return Vec3::lerp(horizon, zenith, t) + sunGlow;
    }

    Vec3 trace(const Ray& ray, int depth = 0) const {
        if (depth >= maxBounces) return skyColor(ray);

        // Find closest hit
        std::optional<Hit> closest;
        for (const auto& s : spheres) {
            auto hit = s.intersect(ray);
            if (hit && (!closest || hit->t < closest->t))
                closest = hit;
        }
        auto floorHit = floor.intersect(ray);
        if (floorHit && (!closest || floorHit->t < closest->t))
            closest = floorHit;

        if (!closest) return skyColor(ray);

        const auto& h = *closest;
        Vec3 toLight = lightDir * -1.0f;

        // Shadow test
        Ray shadowRay{h.point + h.normal * 0.001f, toLight};
        bool inShadow = false;
        for (const auto& s : spheres) {
            if (s.intersect(shadowRay)) { inShadow = true; break; }
        }

        // Diffuse
        float diff = std::max(0.0f, h.normal.dot(toLight));
        if (inShadow) diff *= 0.15f;

        // Specular (Blinn-Phong)
        Vec3 viewDir = (ray.origin - h.point).normalized();
        Vec3 halfDir = (toLight + viewDir).normalized();
        float spec = std::pow(std::max(0.0f, h.normal.dot(halfDir)), h.material.specular);
        if (inShadow) spec = 0;

        Vec3 color = h.material.color * (ambient + lightColor * diff)
                   + lightColor * spec * h.material.specStrength;

        // Reflection
        if (h.material.reflectivity > 0.01f && depth < maxBounces) {
            Vec3 reflDir = ray.direction.reflect(h.normal);
            Ray reflRay{h.point + h.normal * 0.001f, reflDir};
            Vec3 reflColor = trace(reflRay, depth + 1);
            color = color * (1.0f - h.material.reflectivity)
                  + reflColor * h.material.reflectivity;
        }

        return {std::min(color.x, 1.0f), std::min(color.y, 1.0f), std::min(color.z, 1.0f)};
    }
};

// ─── Build a beautiful demo scene ───────────────────────────────
inline Scene buildDemoScene() {
    Scene scene;

    // Floor
    scene.floor.y = -0.5f;
    scene.floor.scale = 0.5f;
    scene.floor.mat1 = {{0.9f, 0.9f, 0.9f}, 0.15f, 50, 0.3f};
    scene.floor.mat2 = {{0.3f, 0.3f, 0.35f}, 0.15f, 50, 0.3f};

    // Central chrome sphere
    scene.spheres.push_back({{0, 0.6f, -2.5f}, 1.1f,
        {{0.9f, 0.92f, 0.95f}, 0.85f, 120, 0.9f}});

    // Colored spheres around it
    scene.spheres.push_back({{-2.0f, 0.0f, -3.0f}, 0.5f,
        {{0.9f, 0.15f, 0.15f}, 0.1f, 40, 0.5f}});    // Red

    scene.spheres.push_back({{1.8f, 0.0f, -2.0f}, 0.5f,
        {{0.15f, 0.7f, 0.15f}, 0.05f, 30, 0.4f}});   // Green

    scene.spheres.push_back({{-1.0f, -0.1f, -1.2f}, 0.4f,
        {{0.2f, 0.3f, 0.9f}, 0.2f, 60, 0.6f}});      // Blue

    scene.spheres.push_back({{1.0f, 0.3f, -4.0f}, 0.8f,
        {{0.9f, 0.75f, 0.1f}, 0.3f, 80, 0.7f}});     // Gold

    scene.spheres.push_back({{-0.5f, -0.25f, -0.8f}, 0.25f,
        {{0.8f, 0.2f, 0.8f}, 0.4f, 50, 0.5f}});      // Purple

    scene.spheres.push_back({{2.5f, 0.2f, -3.5f}, 0.7f,
        {{0.15f, 0.8f, 0.85f}, 0.5f, 90, 0.6f}});    // Cyan/glass

    scene.lightDir = Vec3{-0.6f, -0.8f, -0.5f}.normalized();

    return scene;
}
