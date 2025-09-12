#pragma once
#include <box2d/box2d.h>
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include "Constants.hpp"
#include "Particle.hpp"
#include <algorithm>
namespace SandSim
{
    enum class RigidBodyShape
    {
        Circle,
        Square,
        Triangle
    };

    struct RigidBodyData
    {
        b2BodyId bodyId;
        RigidBodyShape shape;
        MaterialID materialType;
        float radius; // For circles
        float size;   // For squares (side length) and triangles (side length)
        sf::Color color;
        std::vector<sf::Vector2f> vertices; // For triangles and squares (local coordinates)
        std::vector<sf::Vector2i> previousPixels; // Track previous rendered pixels
        bool isActive;
        
        RigidBodyData() : bodyId(b2_nullBodyId), shape(RigidBodyShape::Circle), 
                         materialType(MaterialID::Stone), radius(10.0f), 
                         size(20.0f), color(sf::Color::White), isActive(true) {}
    };

    class RigidBodySystem
    {
    private:
        b2WorldId worldId;
        std::vector<std::unique_ptr<RigidBodyData>> rigidBodies;
        std::vector<b2BodyId> boundaryBodies; // Static ground boundaries
        
        // Physics constants
        static constexpr float PHYSICS_SCALE = 0.01f; // Convert pixels to meters (1 pixel = 0.01 meters)
        static constexpr float INV_PHYSICS_SCALE = 100.0f; // Convert meters to pixels
        
        // World boundaries
        int worldWidth, worldHeight;
        
    public:
        RigidBodySystem(int width, int height);
        ~RigidBodySystem();
        
        // Create rigid bodies
        RigidBodyData* createCircle(float x, float y, float radius, MaterialID material);
        RigidBodyData* createSquare(float x, float y, float size, MaterialID material);
        RigidBodyData* createTriangle(float x, float y, float size, MaterialID material);
        
        // Update physics simulation
        void update(float deltaTime);
        
        // Rendering data
        const std::vector<std::unique_ptr<RigidBodyData>>& getRigidBodies() const { return rigidBodies; }
        
        // Interaction with particle world
        void applyParticleForces(class ParticleWorld* particleWorld);
        void renderToParticleWorld(class ParticleWorld* particleWorld);
        
        // Utility functions
        sf::Vector2f box2DToSFML(const b2Vec2& vec) const;
        b2Vec2 sfmlToBox2D(const sf::Vector2f& vec) const;
        
        // Cleanup
        void removeInactiveBodies();
        void clear();
        
    private:
        void createWorldBoundaries();
        void setupRigidBodyVertices(RigidBodyData* rbData);
        sf::Color getMaterialColor(MaterialID material) const;
        float getMaterialDensity(MaterialID material) const;
        float getMaterialFriction(MaterialID material) const;
        float getMaterialRestitution(MaterialID material) const;
    };

} // namespace SandSim