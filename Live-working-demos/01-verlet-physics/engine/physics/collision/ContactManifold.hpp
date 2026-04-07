#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace engine::physics {

class RigidBody;

/**
 * @brief Represents a single contact point between two objects
 */
struct ContactPoint {
    glm::vec3 worldPointA;        // Contact point on body A in world space
    glm::vec3 worldPointB;        // Contact point on body B in world space
    glm::vec3 localPointA;        // Contact point on body A in local space
    glm::vec3 localPointB;        // Contact point on body B in local space
    
    float penetrationDepth;       // How deep objects are penetrating
    float normalImpulse;          // Accumulated normal impulse (for warm starting)
    float tangentImpulse1;        // Accumulated tangent impulse 1
    float tangentImpulse2;        // Accumulated tangent impulse 2
    
    // Contact feature IDs for coherence
    uint32_t featureA;
    uint32_t featureB;
    
    ContactPoint() : penetrationDepth(0.0f), normalImpulse(0.0f), 
                    tangentImpulse1(0.0f), tangentImpulse2(0.0f),
                    featureA(0), featureB(0) {}
};

/**
 * @brief Represents a collection of contact points between two bodies
 */
class ContactManifold {
public:
    ContactManifold();
    ContactManifold(RigidBody* bodyA, RigidBody* bodyB);
    
    // Body accessors
    RigidBody* getBodyA() const { return bodyA; }
    RigidBody* getBodyB() const { return bodyB; }
    void setBodies(RigidBody* a, RigidBody* b);
    
    // Normal and material properties
    const glm::vec3& getNormal() const { return normal; }
    void setNormal(const glm::vec3& n) { normal = glm::normalize(n); }
    
    float getFriction() const { return friction; }
    void setFriction(float f) { friction = glm::max(0.0f, f); }
    
    float getRestitution() const { return restitution; }
    void setRestitution(float r) { restitution = glm::clamp(r, 0.0f, 1.0f); }
    
    // Contact point management
    void addContact(const ContactPoint& contact);
    void addContact(const glm::vec3& worldPointA, const glm::vec3& worldPointB, 
                   float penetration);
    
    const std::vector<ContactPoint>& getContacts() const { return contacts; }
    size_t getContactCount() const { return contacts.size(); }
    bool hasContacts() const { return !contacts.empty(); }
    
    void clearContacts() { contacts.clear(); }
    
    // Contact processing
    void removeDeepestContact();
    void removeDuplicateContacts(float tolerance = 0.01f);
    void updateContactFeatures();
    
    // Solver interface
    void prepare(float dt);
    void warmStart();
    void solve(float dt);
    void postSolve();
    
    // Debug information
    float getTotalContactArea() const;
    glm::vec3 getCenterOfContacts() const;
    float getAverageDepth() const;
    
    // Validation
    bool isValid() const;
    
private:
    RigidBody* bodyA;
    RigidBody* bodyB;
    
    glm::vec3 normal;             // Contact normal (from A to B)
    std::vector<ContactPoint> contacts;
    
    float friction;               // Combined friction
    float restitution;            // Combined restitution
    
    // Solver cached data
    glm::vec3 tangent1, tangent2; // Tangent vectors for friction
    bool solverDataValid;
    
    // Internal contact management
    void maintainContactCount();
    void selectBestContacts();
    float calculateContactScore(const ContactPoint& contact) const;
    
    // Physics material combination
    void calculateMaterialProperties();
    float combineFriction(float frictionA, float frictionB) const;
    float combineRestitution(float restitutionA, float restitutionB) const;
};

} // namespace engine::physics