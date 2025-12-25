#include "ParticleWorld.hpp"
#include <algorithm>
#include <cmath>
#include <string>
#include <fstream>
#include <filesystem>
#include "Particles/Liquid.hpp"
#include "Particles/Gas.hpp"
#include "Particles/MovableSolid.hpp"
#include "Particles/ImmovableSolid.hpp"
ParticleWorld::ParticleWorld(unsigned int w, unsigned int h, const std::string &worldFile)
    : width(w), height(h), frameCounter(0)
{
    particles.resize(width * height);
    pixelBuffer.resize(width * height * 4); // RGBA
    
    rigidBodySystem = std::make_unique<RigidBodySystem>(width, height);

    if (!worldFile.empty() && std::filesystem::exists(worldFile))
    {
        if (!loadWorld(worldFile))
        {
            std::cerr << "Failed to load world file, starting with empty world" << std::endl;
            clear();
        }
    }
    else
    {
        clear();
    }
}


void ParticleWorld::clear()
{
    for (auto &p : particles)
    {
        p = std::make_unique<EmptyParticle>();
    }
    std::fill(pixelBuffer.begin(), pixelBuffer.end(), 0);
    
    // Clear rigid bodies
    if (rigidBodySystem)
    {
        rigidBodySystem->clear();
    }
}

void ParticleWorld::setParticleAt(int x, int y, std::unique_ptr<Particle> particle)
{
    if (!inBounds(x, y) || !particle) // Added null check for safety
        return;

    int idx = computeIndex(x, y);

    // 1. Update pixel buffer first
    // Since 'particle' is a pointer, we use '->' instead of '.'
    int pixelIdx = idx * 4;
    pixelBuffer[pixelIdx]     = particle->color.r;
    pixelBuffer[pixelIdx + 1] = particle->color.g;
    pixelBuffer[pixelIdx + 2] = particle->color.b;
    pixelBuffer[pixelIdx + 3] = particle->color.a;

    // 2. Move the unique_ptr into the storage vector
    // This transfers ownership. The 'particle' argument becomes null after this line.
    particles[idx] = std::move(particle);
}

void ParticleWorld::swapParticles(int x1, int y1, int x2, int y2) {
    if (!inBounds(x1, y1) || !inBounds(x2, y2)) return;

    int idx1 = computeIndex(x1, y1);
    int idx2 = computeIndex(x2, y2);

    // 1. Swap the pointers in the physics grid
    std::swap(particles[idx1], particles[idx2]);

    // 2. Swap the colors in the pixel buffer (so the screen looks right)
    int pIdx1 = idx1 * 4;
    int pIdx2 = idx2 * 4;

    for (int i = 0; i < 4; i++) {
        std::swap(pixelBuffer[pIdx1 + i], pixelBuffer[pIdx2 + i]);
    }
}

void ParticleWorld::addRigidBody(int centerX, int centerY, float size, RigidBodyShape shape, MaterialID materialType)
{
    if (!rigidBodySystem) return;
    
    // Create rigid body based on shape
    switch (shape)
    {
        case RigidBodyShape::Circle:
            rigidBodySystem->createCircle(static_cast<float>(centerX), static_cast<float>(centerY), size, materialType);
            break;
            
        case RigidBodyShape::Square:
            rigidBodySystem->createSquare(static_cast<float>(centerX), static_cast<float>(centerY), size, materialType);
            break;
            
        case RigidBodyShape::Triangle:
            rigidBodySystem->createTriangle(static_cast<float>(centerX), static_cast<float>(centerY), size, materialType);
            break;
    }
}
void ParticleWorld::update(float deltaTime)
{
    frameCounter++;
    // Alternating scan direction to prevent bias
    bool frameCounterEven = (frameCounter % 2) == 0;
    int ran = frameCounterEven ? 0 : 1;

    // Update rigid body physics first
    if (rigidBodySystem)
    {
        rigidBodySystem->update(deltaTime);
        rigidBodySystem->renderToParticleWorld(this);
    }

    // Process particles from bottom to top to prevent double updates (falling logic)
    for (int y = height - 1; y >= 0; --y)
    {
        // Alternating Left-to-Right / Right-to-Left loop
        for (int x = ran ? 0 : width - 1; ran ? x < width : x >= 0; ran ? ++x : --x)
        {
            // ASSUMPTION: getParticleAt now returns a pointer (Particle*)
            Particle* particle = getParticleAt(x, y);

            // Safety check: if you use nullptr for empty space
            if (!particle) 
                continue;

            // Logic check: skip empty particles
            if (particle->id == MaterialID::EmptyParticle)
                continue;

            // Optimization: Skip if already updated (e.g., moved into this slot this frame)
            if (particle->hasBeenUpdatedThisFrame)
                continue;

            // Skip updating rigid body particles (marked with lifetime -1.0f)
            if (particle->lifeTime == -1.0f)
                continue;

            // Update particle lifetime
            particle->lifeTime += deltaTime;
            particle->update(x, y, deltaTime, *this);
        }
    }

    for (auto &p : particles)
    {
        if (p) // Check if pointer is valid
        {
            p->hasBeenUpdatedThisFrame = false;
        }
    }
}
void ParticleWorld::addParticleCircle(int centerX, int centerY, float radius, MaterialID materialType)
{
    // Place particles in circular area around center point
    for (int dy = -static_cast<int>(radius); dy <= static_cast<int>(radius); ++dy)
    {
        for (int dx = -static_cast<int>(radius); dx <= static_cast<int>(radius); ++dx)
        {
            int x = centerX + dx;
            int y = centerY + dy;

            // Check distance from center
            float distance = std::sqrt(static_cast<float>(dx * dx + dy * dy));
            
            if (distance <= radius)
            {
                // Only place particle if cell is empty and within world bounds
                if (inBounds(x, y) && isEmpty(x, y))
                {
                    // 1. Create the specific particle class (Sand, Water, etc.)
                    auto p = createParticleByType(materialType);

                    // 2. Check if creation succeeded (returns nullptr if Empty/Invalid)
                    if (p) 
                    {
                        p->velocity = {
                            Random::randFloat(-0.5f, 0.5f), 
                            Random::randFloat(-0.5f, 0.5f)
                        };

                        // 4. Move ownership of the pointer into the grid
                        setParticleAt(x, y, std::move(p));
                    }
                }
            }
        }
    }
}

void ParticleWorld::eraseCircle(int centerX, int centerY, float radius)
{
    // Remove particles in circular area
    for (int i = -radius; i < radius; ++i)
    {
        for (int j = -radius; j < radius; ++j)
        {
            int x = centerX + j;
            int y = centerY + i;

            if (inBounds(x, y))
            {
                float distance = std::sqrt(i * i + j * j);
                if (distance <= radius)
                {
                    setParticleAt(x, y, std::make_unique<EmptyParticle>());
                }
            }
        }
    }
}

std::string ParticleWorld::getNextAvailableFilename(const std::string& baseName) 
{
    std::string filename;
    int counter = 0;
    
    do {
        filename = baseName + std::to_string(counter) + ".rrr";
        counter++;
    } while (std::filesystem::exists(filename));
    
    return filename;
}

bool ParticleWorld::saveWorld(const std::string& baseFilename) 
{
    std::string filename = getNextAvailableFilename("worlds/" + baseFilename);
    
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return false;
    }
    
    try {
        // Write header: dimensions and frame counter
        file.write(reinterpret_cast<const char*>(&width), sizeof(width));
        file.write(reinterpret_cast<const char*>(&height), sizeof(height));
        file.write(reinterpret_cast<const char*>(&frameCounter), sizeof(frameCounter));
        
        // Write all particle data sequentially
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                // Get pointer to particle (const version)
                const Particle* p = getParticleAt(x, y);
                
                // CRITICAL SAFETY: 
                // Even if you have EmptyParticle, if the grid wasn't fully initialized 
                // getParticleAt might return nullptr. We handle that gracefully.
                if (p) 
                {
                    file.write(reinterpret_cast<const char*>(&p->id), sizeof(p->id));
                    file.write(reinterpret_cast<const char*>(&p->velocity.x), sizeof(p->velocity.x));
                    file.write(reinterpret_cast<const char*>(&p->velocity.y), sizeof(p->velocity.y));
                    file.write(reinterpret_cast<const char*>(&p->lifeTime), sizeof(p->lifeTime));
                    file.write(reinterpret_cast<const char*>(&p->color.r), sizeof(p->color.r));
                    file.write(reinterpret_cast<const char*>(&p->color.g), sizeof(p->color.g));
                    file.write(reinterpret_cast<const char*>(&p->color.b), sizeof(p->color.b));
                    file.write(reinterpret_cast<const char*>(&p->color.a), sizeof(p->color.a));
                }
                else 
                {
                    // If nullptr, write as Empty (fallback)
                    MaterialID emptyId = MaterialID::EmptyParticle;
                    float zero = 0.0f;
                    uint8_t zeroByte = 0;
                    
                    file.write(reinterpret_cast<const char*>(&emptyId), sizeof(emptyId));
                    file.write(reinterpret_cast<const char*>(&zero), sizeof(zero)); // vel x
                    file.write(reinterpret_cast<const char*>(&zero), sizeof(zero)); // vel y
                    file.write(reinterpret_cast<const char*>(&zero), sizeof(zero)); // lifetime
                    file.write(reinterpret_cast<const char*>(&zeroByte), sizeof(zeroByte)); // r
                    file.write(reinterpret_cast<const char*>(&zeroByte), sizeof(zeroByte)); // g
                    file.write(reinterpret_cast<const char*>(&zeroByte), sizeof(zeroByte)); // b
                    file.write(reinterpret_cast<const char*>(&zeroByte), sizeof(zeroByte)); // a
                }
            }
        }
        
        file.close();
        std::cout << "World saved successfully as: " << filename << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error saving world: " << e.what() << std::endl;
        file.close();
        return false;
    }
}

bool ParticleWorld::loadWorld(const std::string& filename) 
{
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for reading: " << filename << std::endl;
        return false;
    }
    
    try {
        // Read and verify header dimensions
        int fileWidth, fileHeight;
        file.read(reinterpret_cast<char*>(&fileWidth), sizeof(fileWidth));
        file.read(reinterpret_cast<char*>(&fileHeight), sizeof(fileHeight));
        
        if (fileWidth != width || fileHeight != height) {
            std::cerr << "World dimensions mismatch! File: " << fileWidth << "x" << fileHeight 
                      << ", Current: " << width << "x" << height << std::endl;
            file.close();
            return false;
        }
        
        file.read(reinterpret_cast<char*>(&frameCounter), sizeof(frameCounter));
        
        // Read particle data sequentially
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                
                // 1. Read the ID first so we know WHAT class to instantiate
                MaterialID tempId;
                file.read(reinterpret_cast<char*>(&tempId), sizeof(tempId));
                
                // 2. Use the Factory to create the specific class (Sand, Water, etc.)
                // This returns a std::unique_ptr<Particle>
                auto p = createParticleByType(tempId);
                
                // Safety: If factory returned null (unknown ID), create Empty
                if (!p) {
                    p = std::make_unique<EmptyParticle>();
                }

                // 3. Read the rest of the data directly into the allocated object
                // We don't read ID again because we already set it in the constructor via the factory
                // However, the factory sets default colors/lifetime. We overwrite them here with saved data.
                
                file.read(reinterpret_cast<char*>(&p->velocity.x), sizeof(p->velocity.x));
                file.read(reinterpret_cast<char*>(&p->velocity.y), sizeof(p->velocity.y));
                file.read(reinterpret_cast<char*>(&p->lifeTime), sizeof(p->lifeTime));
                file.read(reinterpret_cast<char*>(&p->color.r), sizeof(p->color.r));
                file.read(reinterpret_cast<char*>(&p->color.g), sizeof(p->color.g));
                file.read(reinterpret_cast<char*>(&p->color.b), sizeof(p->color.b));
                file.read(reinterpret_cast<char*>(&p->color.a), sizeof(p->color.a));
                
                p->hasBeenUpdatedThisFrame = false;
                
                // 4. Move ownership to the grid
                setParticleAt(x, y, std::move(p));
            }
        }
        
        file.close();
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error loading world: " << e.what() << std::endl;
        file.close();
        return false;
    }
}
#include <memory> // Required for std::unique_ptr

std::unique_ptr<Particle> ParticleWorld::createParticleByType(MaterialID type) {
    for (const auto& props : ALL_MATERIALS) {
        if (props.id == type) {
            return props.create(); // Calls the lambda from the registry
        }
    }
    // Return empty particle for MaterialID::Empty or unknown
    return std::make_unique<EmptyParticle>(); 
}