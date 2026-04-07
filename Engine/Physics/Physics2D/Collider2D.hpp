#pragma once

#include "../../Core/Base.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <vector>
#include <cstdint>

namespace GameEngine {
namespace Physics2D {

    /**
     * @brief 2D axis-aligned bounding box
     */
    struct AABB2D {
        glm::vec2 Min{0.0f};
        glm::vec2 Max{0.0f};

        bool overlaps(const AABB2D& other) const {
            return !(Max.x < other.Min.x || Min.x > other.Max.x ||
                     Max.y < other.Min.y || Min.y > other.Max.y);
        }

        glm::vec2 center() const { return (Min + Max) * 0.5f; }
        glm::vec2 extents() const { return (Max - Min) * 0.5f; }

        AABB2D merge(const AABB2D& other) const {
            return {glm::min(Min, other.Min), glm::max(Max, other.Max)};
        }
    };

    /**
     * @brief Shape type enumeration for 2D colliders
     */
    enum class ShapeType2D {
        Circle,
        Box,
        Polygon,
        Edge,
        Capsule
    };

    class RigidBody2D;

    /**
     * @brief Base 2D collider interface
     *
     * Colliders define the shape used for collision detection.
     * Each collider can have an offset relative to the body center,
     * a collision layer/mask for filtering, and a trigger flag.
     */
    class Collider2D {
    public:
        virtual ~Collider2D() = default;

        virtual ShapeType2D getType() const = 0;

        /**
         * @brief Compute the world-space AABB given body position and rotation
         */
        virtual AABB2D computeAABB(const glm::vec2& position, float rotation) const = 0;

        /**
         * @brief Compute rotational inertia for given mass
         */
        virtual float computeInertia(float mass) const = 0;

        /**
         * @brief Compute area of the shape (used for density-based mass)
         */
        virtual float computeArea() const = 0;

        // ----- Collision filtering -----

        uint16_t CollisionLayer = 0x0001;
        uint16_t CollisionMask = 0xFFFF;

        bool shouldCollideWith(const Collider2D& other) const {
            return (CollisionLayer & other.CollisionMask) != 0 &&
                   (other.CollisionLayer & CollisionMask) != 0;
        }

        // ----- Trigger mode -----

        bool IsTrigger = false;

        // ----- Offset from body center -----

        glm::vec2 Offset{0.0f};

        // ----- Owning body (set by PhysicsWorld2D) -----

        void setBody(RigidBody2D* body) { m_Body = body; }
        RigidBody2D* getBody() const { return m_Body; }

    public:
        /**
         * @brief Rotate a local-space point by angle (radians)
         */
        static glm::vec2 rotatePoint(const glm::vec2& p, float angle) {
            float c = std::cos(angle);
            float s = std::sin(angle);
            return {p.x * c - p.y * s, p.x * s + p.y * c};
        }

        /**
         * @brief Transform a local offset to world space
         */
        glm::vec2 getWorldOffset(const glm::vec2& position, float rotation) const {
            return position + rotatePoint(Offset, rotation);
        }

    private:
        RigidBody2D* m_Body = nullptr;
    };

    // =====================================================================
    // Concrete collider shapes
    // =====================================================================

    /**
     * @brief Circle collider (center + radius)
     */
    class CircleCollider2D : public Collider2D {
    public:
        explicit CircleCollider2D(float radius = 0.5f)
            : Radius(radius) {}

        ShapeType2D getType() const override { return ShapeType2D::Circle; }

        AABB2D computeAABB(const glm::vec2& position, float rotation) const override {
            glm::vec2 center = getWorldOffset(position, rotation);
            glm::vec2 r(Radius);
            return {center - r, center + r};
        }

        float computeInertia(float mass) const override {
            // I = 0.5 * m * r^2 (solid disk)
            return 0.5f * mass * Radius * Radius;
        }

        float computeArea() const override {
            return glm::pi<float>() * Radius * Radius;
        }

        float Radius;
    };

    /**
     * @brief Box collider (half-extents + center offset)
     */
    class BoxCollider2D : public Collider2D {
    public:
        explicit BoxCollider2D(const glm::vec2& halfExtents = glm::vec2(0.5f))
            : HalfExtents(halfExtents) {}

        ShapeType2D getType() const override { return ShapeType2D::Box; }

        AABB2D computeAABB(const glm::vec2& position, float rotation) const override {
            glm::vec2 center = getWorldOffset(position, rotation);
            float totalAngle = rotation;
            float c = std::abs(std::cos(totalAngle));
            float s = std::abs(std::sin(totalAngle));

            // Rotated half-extents projected onto axes
            glm::vec2 extent;
            extent.x = HalfExtents.x * c + HalfExtents.y * s;
            extent.y = HalfExtents.x * s + HalfExtents.y * c;

            return {center - extent, center + extent};
        }

        float computeInertia(float mass) const override {
            // I = m/12 * (w^2 + h^2), where w=2*hx, h=2*hy
            float w = 2.0f * HalfExtents.x;
            float h = 2.0f * HalfExtents.y;
            return (mass / 12.0f) * (w * w + h * h);
        }

        float computeArea() const override {
            return 4.0f * HalfExtents.x * HalfExtents.y;
        }

        /**
         * @brief Get the four corners in local space
         */
        void getLocalVertices(glm::vec2 outVerts[4]) const {
            outVerts[0] = Offset + glm::vec2(-HalfExtents.x, -HalfExtents.y);
            outVerts[1] = Offset + glm::vec2(HalfExtents.x, -HalfExtents.y);
            outVerts[2] = Offset + glm::vec2(HalfExtents.x, HalfExtents.y);
            outVerts[3] = Offset + glm::vec2(-HalfExtents.x, HalfExtents.y);
        }

        /**
         * @brief Get the four corners in world space
         */
        void getWorldVertices(const glm::vec2& position, float rotation, glm::vec2 outVerts[4]) const {
            glm::vec2 localVerts[4];
            getLocalVertices(localVerts);
            for (int i = 0; i < 4; i++) {
                outVerts[i] = position + rotatePoint(localVerts[i], rotation);
            }
        }

        glm::vec2 HalfExtents;
    };

    /**
     * @brief Convex polygon collider (max 8 vertices, wound CCW)
     */
    class PolygonCollider2D : public Collider2D {
    public:
        static constexpr int MaxVertices = 8;

        PolygonCollider2D() = default;

        /**
         * @brief Construct from a set of vertices (will be sorted CCW, must be convex)
         */
        explicit PolygonCollider2D(const std::vector<glm::vec2>& vertices) {
            setVertices(vertices);
        }

        ShapeType2D getType() const override { return ShapeType2D::Polygon; }

        AABB2D computeAABB(const glm::vec2& position, float rotation) const override {
            glm::vec2 minPt(std::numeric_limits<float>::max());
            glm::vec2 maxPt(std::numeric_limits<float>::lowest());

            for (int i = 0; i < m_VertexCount; i++) {
                glm::vec2 worldVert = position + rotatePoint(m_Vertices[i] + Offset, rotation);
                minPt = glm::min(minPt, worldVert);
                maxPt = glm::max(maxPt, worldVert);
            }
            return {minPt, maxPt};
        }

        float computeInertia(float mass) const override {
            // Use polygon inertia formula:
            // I = (m / 6) * Sum(|cross(v[i], v[i+1])| * (dot(v[i],v[i]) + dot(v[i],v[i+1]) + dot(v[i+1],v[i+1])))
            // divided by Sum(|cross(v[i], v[i+1])|)
            if (m_VertexCount < 3) return 0.0f;

            float numerator = 0.0f;
            float denominator = 0.0f;

            for (int i = 0; i < m_VertexCount; i++) {
                int j = (i + 1) % m_VertexCount;
                const glm::vec2& a = m_Vertices[i];
                const glm::vec2& b = m_Vertices[j];

                float cross = std::abs(a.x * b.y - a.y * b.x);
                numerator += cross * (glm::dot(a, a) + glm::dot(a, b) + glm::dot(b, b));
                denominator += cross;
            }

            if (denominator < 1e-8f) return 0.0f;
            return (mass / 6.0f) * (numerator / denominator);
        }

        float computeArea() const override {
            // Shoelace formula
            float area = 0.0f;
            for (int i = 0; i < m_VertexCount; i++) {
                int j = (i + 1) % m_VertexCount;
                area += m_Vertices[i].x * m_Vertices[j].y;
                area -= m_Vertices[j].x * m_Vertices[i].y;
            }
            return std::abs(area) * 0.5f;
        }

        void setVertices(const std::vector<glm::vec2>& vertices) {
            m_VertexCount = static_cast<int>(std::min(vertices.size(), static_cast<size_t>(MaxVertices)));
            for (int i = 0; i < m_VertexCount; i++) {
                m_Vertices[i] = vertices[i];
            }
            computeNormals();
        }

        int getVertexCount() const { return m_VertexCount; }
        const glm::vec2& getVertex(int index) const { return m_Vertices[index]; }
        const glm::vec2& getNormal(int index) const { return m_Normals[index]; }

        /**
         * @brief Get vertices in world space
         */
        void getWorldVertices(const glm::vec2& position, float rotation,
                              glm::vec2* outVerts, int& outCount) const {
            outCount = m_VertexCount;
            for (int i = 0; i < m_VertexCount; i++) {
                outVerts[i] = position + rotatePoint(m_Vertices[i] + Offset, rotation);
            }
        }

        /**
         * @brief Get normals in world space
         */
        void getWorldNormals(float rotation, glm::vec2* outNormals, int& outCount) const {
            outCount = m_VertexCount;
            for (int i = 0; i < m_VertexCount; i++) {
                outNormals[i] = rotatePoint(m_Normals[i], rotation);
            }
        }

    private:
        void computeNormals() {
            for (int i = 0; i < m_VertexCount; i++) {
                int j = (i + 1) % m_VertexCount;
                glm::vec2 edge = m_Vertices[j] - m_Vertices[i];
                // Outward normal (for CCW winding)
                m_Normals[i] = glm::normalize(glm::vec2(edge.y, -edge.x));
            }
        }

        int m_VertexCount = 0;
        glm::vec2 m_Vertices[MaxVertices] = {};
        glm::vec2 m_Normals[MaxVertices] = {};
    };

    /**
     * @brief Edge collider (chain of line segments)
     */
    class EdgeCollider2D : public Collider2D {
    public:
        EdgeCollider2D() = default;

        explicit EdgeCollider2D(const std::vector<glm::vec2>& points)
            : m_Points(points) {}

        ShapeType2D getType() const override { return ShapeType2D::Edge; }

        AABB2D computeAABB(const glm::vec2& position, float rotation) const override {
            if (m_Points.empty()) return {position, position};

            glm::vec2 minPt(std::numeric_limits<float>::max());
            glm::vec2 maxPt(std::numeric_limits<float>::lowest());

            for (const auto& p : m_Points) {
                glm::vec2 worldPt = position + rotatePoint(p + Offset, rotation);
                minPt = glm::min(minPt, worldPt);
                maxPt = glm::max(maxPt, worldPt);
            }
            return {minPt, maxPt};
        }

        float computeInertia(float /*mass*/) const override {
            // Edges have no area, so inertia is zero (treated as static typically)
            return 0.0f;
        }

        float computeArea() const override {
            return 0.0f; // Line segments have no area
        }

        void setPoints(const std::vector<glm::vec2>& points) { m_Points = points; }
        const std::vector<glm::vec2>& getPoints() const { return m_Points; }
        int getSegmentCount() const { return static_cast<int>(m_Points.size()) - 1; }

        /**
         * @brief Get a segment in world space
         */
        void getWorldSegment(int index, const glm::vec2& position, float rotation,
                             glm::vec2& outA, glm::vec2& outB) const {
            outA = position + rotatePoint(m_Points[index] + Offset, rotation);
            outB = position + rotatePoint(m_Points[index + 1] + Offset, rotation);
        }

    private:
        std::vector<glm::vec2> m_Points;
    };

    /**
     * @brief Capsule collider (two semicircles connected by a rectangle)
     */
    class CapsuleCollider2D : public Collider2D {
    public:
        CapsuleCollider2D(float height = 1.0f, float radius = 0.25f)
            : Height(height), Radius(radius) {}

        ShapeType2D getType() const override { return ShapeType2D::Capsule; }

        AABB2D computeAABB(const glm::vec2& position, float rotation) const override {
            // Half-height along the capsule axis (local Y)
            float halfH = Height * 0.5f;
            glm::vec2 localTop = Offset + glm::vec2(0.0f, halfH);
            glm::vec2 localBot = Offset + glm::vec2(0.0f, -halfH);

            glm::vec2 worldTop = position + rotatePoint(localTop, rotation);
            glm::vec2 worldBot = position + rotatePoint(localBot, rotation);

            glm::vec2 minPt = glm::min(worldTop, worldBot) - glm::vec2(Radius);
            glm::vec2 maxPt = glm::max(worldTop, worldBot) + glm::vec2(Radius);

            return {minPt, maxPt};
        }

        float computeInertia(float mass) const override {
            // Approximate: rectangle + two semicircles
            float rectH = Height;
            float rectW = Radius * 2.0f;
            float rectArea = rectH * rectW;
            float circleArea = glm::pi<float>() * Radius * Radius;
            float totalArea = rectArea + circleArea;
            if (totalArea < 1e-8f) return 0.0f;

            float rectMass = mass * (rectArea / totalArea);
            float circleMass = mass * (circleArea / totalArea);

            // Rectangle inertia about center
            float rectI = (rectMass / 12.0f) * (rectW * rectW + rectH * rectH);

            // Two semicircles: combined as full circle at distance from center
            float circleI = 0.5f * circleMass * Radius * Radius;
            float halfH = Height * 0.5f;
            circleI += circleMass * halfH * halfH; // parallel axis theorem (approximate)

            return rectI + circleI;
        }

        float computeArea() const override {
            // Rectangle + circle
            return Height * Radius * 2.0f + glm::pi<float>() * Radius * Radius;
        }

        /**
         * @brief Get the two center-of-circle endpoints in world space
         */
        void getWorldEndpoints(const glm::vec2& position, float rotation,
                               glm::vec2& outTop, glm::vec2& outBottom) const {
            float halfH = Height * 0.5f;
            outTop = position + rotatePoint(Offset + glm::vec2(0.0f, halfH), rotation);
            outBottom = position + rotatePoint(Offset + glm::vec2(0.0f, -halfH), rotation);
        }

        float Height; // distance between the two circle centers
        float Radius; // radius of the circles
    };

}}
