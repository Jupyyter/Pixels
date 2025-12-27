#pragma once
#include <vector>
#include <memory>
#include <cstdint>
#include <SFML/Graphics.hpp>
#include "particles/Particle.hpp"
#include "Constants.hpp"
#include "Random.hpp"
#include "RigidBody.hpp"
#include <iostream>
enum class RigidBodyShape; 
class RigidBodySystem; 
class ParticleWorld
{
private:
    std::vector<std::unique_ptr<Particle>> particles;
    std::vector<std::uint8_t> pixelBuffer;
    int width, height;
    uint32_t frameCounter;

    std::unique_ptr<RigidBodySystem> rigidBodySystem;
    void updatePixelColor(int x, int y, const sf::Color& color);
public:
void updateParticleColor(Particle* particle, ParticleWorld& world) ;
    // File I/O operations
    bool saveWorld(const std::string &baseFilename = "world");
    bool loadWorld(const std::string &filename);
    std::string getNextAvailableFilename(const std::string &baseName);

    ParticleWorld(unsigned int w, unsigned int h, const std::string &worldFile = "");

    // Reset world to empty state
    void clear();

    // Coordinate/bounds utilities
    int computeIndex(int x, int y) const { return y * width + x; }
    bool inBounds(int x, int y) const { return x >= 0 && x < width && y >= 0 && y < height; }
    bool isEmpty(int x, int y) const;

    // Particle access
    Particle *getParticleAt(int x, int y) { return particles[computeIndex(x, y)].get(); }
    void setParticleAt(int x, int y, std::unique_ptr<Particle>);
    void swapParticles(int x1, int y1, int x2, int y2);

    // Main simulation update
    void update(float deltaTime);

    // Rendering
    const std::uint8_t *getPixelBuffer() const { return pixelBuffer.data(); }
    int getWidth() const { return width; }
    int getHeight() const { return height; }

    // Particle placement/removal
    void addParticleCircle(int centerX, int centerY, float radius, MaterialID materialType);
    void eraseCircle(int centerX, int centerY, float radius);

    // Rigid body methods
    void addRigidBody(int centerX, int centerY, float size, RigidBodyShape shape, MaterialID materialType);
    RigidBodySystem *getRigidBodySystem() { return rigidBodySystem.get(); }

    // Factory method for creating particles by type - make this public so RigidBodySystem can use it
    std::unique_ptr<Particle> createParticleByType(MaterialID type);
    ~ParticleWorld(); 
};