#include "ContactManifold.hpp"

namespace GameEngine {
namespace Physics {

    ContactManifold::ContactManifold(RigidBody* bodyA, RigidBody* bodyB)
        : m_BodyA(bodyA), m_BodyB(bodyB) {
    }

    void ContactManifold::AddContact(const ContactPoint& contact) {
        // Limit to 4 contacts per manifold (typical for box-box)
        if (m_Contacts.size() < 4) {
            m_Contacts.push_back(contact);
        } else {
            // Find contact with smallest penetration and replace it
            float minPenetration = contact.Penetration;
            int replaceIndex = -1;
            
            for (size_t i = 0; i < m_Contacts.size(); i++) {
                if (m_Contacts[i].Penetration < minPenetration) {
                    minPenetration = m_Contacts[i].Penetration;
                    replaceIndex = static_cast<int>(i);
                }
            }
            
            if (replaceIndex >= 0) {
                m_Contacts[replaceIndex] = contact;
            }
        }
    }

}}