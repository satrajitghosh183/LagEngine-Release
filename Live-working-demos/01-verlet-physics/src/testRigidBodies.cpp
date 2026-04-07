#include <iostream>
#include <memory>
#include "engine/physics/PhysicsWorld.hpp"
#include "engine/physics/RigidBody.hpp"
#include "engine/physics/SphereShape.hpp"
#include "engine/physics/BoxShape.hpp"
#include "engine/core/Logger.hpp"

using namespace engine::physics;
using namespace engine::core::log;

int main() {
    Logger::log("Starting Rigid Body Physics Test", LogLevel::Info);
    
    // Create physics world
    PhysicsWorld world;
    world.setGravity(glm::vec3(0.0f, -9.81f, 0.0f));
   
   // Create ground (static box)
   auto ground = std::make_shared<RigidBody>(RigidBody::BodyType::Static);
   ground->setPosition(glm::vec3(0.0f, -5.0f, 0.0f));
   ground->setCollisionShape(std::make_shared<BoxShape>(glm::vec3(10.0f, 1.0f, 10.0f)));
   world.addRigidBody(ground);
   
   // Create falling sphere
   auto sphere = std::make_shared<RigidBody>(RigidBody::BodyType::Dynamic, 1.0f);
   sphere->setPosition(glm::vec3(0.0f, 10.0f, 0.0f));
   sphere->setCollisionShape(std::make_shared<SphereShape>(1.0f));
   world.addRigidBody(sphere);
   
   // Create falling box
   auto box = std::make_shared<RigidBody>(RigidBody::BodyType::Dynamic, 2.0f);
   box->setPosition(glm::vec3(2.0f, 15.0f, 0.0f));
   box->setCollisionShape(std::make_shared<BoxShape>(glm::vec3(1.0f, 1.0f, 1.0f)));
   box->setLinearVelocity(glm::vec3(-1.0f, 0.0f, 0.0f));
   world.addRigidBody(box);
   
   // Set up collision events
   world.onCollisionEnter = [](RigidBody* bodyA, RigidBody* bodyB, const ContactPoint& contact) {
       Logger::log("Collision detected at: (" + 
                  std::to_string(contact.position.x) + ", " +
                  std::to_string(contact.position.y) + ", " +
                  std::to_string(contact.position.z) + ")", LogLevel::Info);
   };
   
   // Simulation loop
   float totalTime = 0.0f;
   const float timeStep = 1.0f / 60.0f;
   const float simulationTime = 5.0f;
   
   Logger::log("Starting simulation...", LogLevel::Info);
   
   while (totalTime < simulationTime) {
       world.update(timeStep);
       totalTime += timeStep;
       
       // Log positions every second
       if (static_cast<int>(totalTime) > static_cast<int>(totalTime - timeStep)) {
           glm::vec3 spherePos = sphere->getPosition();
           glm::vec3 boxPos = box->getPosition();
           
           Logger::log("Time: " + std::to_string(totalTime) + "s", LogLevel::Info);
           Logger::log("Sphere: (" + std::to_string(spherePos.x) + ", " + 
                      std::to_string(spherePos.y) + ", " + std::to_string(spherePos.z) + ")", LogLevel::Info);
           Logger::log("Box: (" + std::to_string(boxPos.x) + ", " + 
                      std::to_string(boxPos.y) + ", " + std::to_string(boxPos.z) + ")", LogLevel::Info);
           Logger::log("Contact points: " + std::to_string(world.getContactCount()), LogLevel::Info);
       }
   }
   
   // Test raycast
   Ray ray(glm::vec3(0.0f, 20.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
   RaycastHit hit;
   
   if (world.raycast(ray, hit)) {
       Logger::log("Raycast hit at distance: " + std::to_string(hit.distance), LogLevel::Info);
       Logger::log("Hit point: (" + std::to_string(hit.point.x) + ", " + 
                  std::to_string(hit.point.y) + ", " + std::to_string(hit.point.z) + ")", LogLevel::Info);
   } else {
       Logger::log("Raycast missed", LogLevel::Warning);
   }
   
   Logger::log("Simulation completed successfully!", LogLevel::Info);
   return 0;
}