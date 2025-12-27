#include "Particles/Particle.hpp"
#include "ParticleWorld.hpp"
#include "Random.hpp"
void Particle::die(ParticleWorld& world) {
    this->isDead = true;
    
    // Use the internal position variable
    // WARNING: 'setParticleAt' will destroy 'this' object immediately!
    // We capture coordinates in local vars just to be safe (though usually unnecessary if order is strict)
    int myX = this->position.x;
    int myY = this->position.y;

    world.setParticleAt(myX, myY, std::make_unique<EmptyParticle>());
}
bool Particle::applyHeatToNeighborsIfIgnited(ParticleWorld& world) {
        // Java: if (!isEffectsFrame() || !shouldApplyHeat()) return false;
        // (Assuming isEffectsFrame() logic is handled externally or by world)
        if (!shouldApplyHeat()) return false;

        // Loop through 3x3 neighbors
        for (int x = position.x - 1; x <= position.x + 1; x++) {
            for (int y = position.y - 1; y <= position.y + 1; y++) {
                // Skip self
                if (x == position.x && y == position.y) continue;

                if (world.inBounds(x, y)) {
                    Particle* neighbor = world.getParticleAt(x, y);
                    if (neighbor != nullptr) {
                        neighbor->receiveHeat(heatFactor);
                    }
                }
            }
        }
        return true;
    }
    void Particle::spawnSparkIfIgnited(ParticleWorld& world) {
        // Java: if (!isEffectsFrame() || !isIgnited) return;
        if (!isIgnited) return;

        // Java uses y + 1 for "UpNeighbor", implying Y-Up coordinates.
        // SFML uses Y-Down, so "Up" is y - 1.
        int upX = position.x;
        int upY = position.y - 1;

        if (world.inBounds(upX, upY)) {
            Particle* upNeighbor = world.getParticleAt(upX, upY);
            
            // Java: if (upNeighbor instanceof EmptyCell)
            // In C++, we check if the neighbor is the specific Empty type
            bool isEmpty = (upNeighbor != nullptr && upNeighbor->id == MaterialID::EmptyParticle);

            if (isEmpty) {
                // Java: Math.random() > .1 ? SPARK : SMOKE
                MaterialID elementToSpawn = (Random::randFloat(0.0f, 1.0f) > 0.1f) 
                                            ? MaterialID::Spark 
                                            : MaterialID::Smoke;

                // Java: matrix.spawnElementByMatrix(...)
                // We use the neighbor's dieAndReplace to swap the empty cell with the new particle
                upNeighbor->dieAndReplace(elementToSpawn, world);
            }
        }
    }
void Particle::dieAndReplace(MaterialID newType, ParticleWorld& world) {
    this->isDead = true;
    
    int myX = this->position.x;
    int myY = this->position.y;

    std::unique_ptr<Particle> newPart = world.createParticleByType(newType);
    
    if (newPart) {
        // Optional: Inherit velocity or temperature
        // newPart->velocity = this->velocity; 
        
        world.setParticleAt(myX, myY, std::move(newPart));
    } else {
        world.setParticleAt(myX, myY, std::make_unique<EmptyParticle>());
    }
}