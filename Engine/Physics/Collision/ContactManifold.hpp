#pragma once

#include "../../Core/Base.hpp"
#include <glm/glm.hpp>
#include <vector>

namespace GameEngine {
namespace Physics {

    class RigidBody;

    /**
     * @brief Contact point between two bodies
     */
    struct ContactPoint {
        glm::vec3 PositionA;        // Contact point on body A (world space)
        glm::vec3 PositionB;        // Contact point on body B (world space)
        glm::vec3 Normal;           // Contact normal (from A to B)
        float Penetration;          // Penetration depth
        float NormalImpulse;        // Accumulated normal impulse
        float TangentImpulse[2];    // Accumulated tangent impulses
        
        ContactPoint()
            : Penetration(0.0f)
            , NormalImpulse(0.0f) {
            TangentImpulse[0] = 0.0f;
            TangentImpulse[1] = 0.0f;
        }
    };

    /**
     * @brief Collection of contact points between two bodies
     */
    class ContactManifold {
    public:
        ContactManifold(RigidBody* bodyA, RigidBody* bodyB);
        
        /**
         * @brief Add contact point
         */
        void AddContact(const ContactPoint& contact);
        
        /**
         * @brief Get all contacts
         */
        const std::vector<ContactPoint>& GetContacts() const { return m_Contacts; }
        std::vector<ContactPoint>& GetContacts() { return m_Contacts; }
        
        /**
         * @brief Clear all contacts
         */
        void Clear() { m_Contacts.clear(); }
        
        /**
         * @brief Get bodies
         */
        RigidBody* GetBodyA() const { return m_BodyA; }
        RigidBody* GetBodyB() const { return m_BodyB; }
        
        /**
         * @brief Get contact count
         */
        size_t GetContactCount() const { return m_Contacts.size(); }
        
        /**
         * @brief Physics material properties
         */
        float Friction = 0.5f;
        float Restitution = 0.3f;
        
    private:
        RigidBody* m_BodyA;
        RigidBody* m_BodyB;
        std::vector<ContactPoint> m_Contacts;
    };

}}