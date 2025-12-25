#include "Particles/MovableSolid.hpp"
#include "ParticleWorld.hpp"
#include <cmath>
#include <algorithm>

// Terminal velocity (Positive for Down)
static const float MAX_VEL_Y = 124.0f;

void MovableSolid::update(int x, int y, float dt, ParticleWorld& world) {
    if (hasBeenUpdatedThisFrame) return;
    hasBeenUpdatedThisFrame = true;

    // 1. Gravity (SFML: Add to Y to go Down)
    velocity.y += (GRAVITY * dt);
    
    // Cap falling speed
    if (velocity.y > MAX_VEL_Y) velocity.y = MAX_VEL_Y;
    
    if (isFreeFalling) velocity.x *= 0.9f;

    // 2. Thresholds (FIXED: Proper accumulation)
    xThreshold += std::abs(velocity.x * dt);
    yThreshold += std::abs(velocity.y * dt);

    int velXInt = static_cast<int>(xThreshold);
    int velYInt = static_cast<int>(yThreshold);

    // Subtract only the whole pixels we move this frame
    xThreshold -= static_cast<float>(velXInt);
    yThreshold -= static_cast<float>(velYInt);

    int xMod = (velocity.x < 0) ? -1 : 1;
    int yMod = (velocity.y < 0) ? -1 : 1;

    // 3. Bresenham Vector Pathing
    int upperBound = std::max(velXInt, velYInt);
    
    // FIXED: Hanging Logic
    // If we aren't moving enough to trigger the loop, check if we are in the air.
    if (upperBound == 0) {
        if (world.inBounds(x, y + 1) && world.getParticleAt(x, y + 1)->id == MaterialID::EmptyParticle) {
            isFreeFalling = true; // Build up gravity to fall next frame
        }
    }

    int currentX = x;
    int currentY = y;

    int minBound = std::min(velXInt, velYInt);
    float slope = (upperBound == 0) ? 0.0f : ((float)(minBound) / (upperBound));
    bool xDiffIsLarger = velXInt > velYInt;

    for (int i = 1; i <= upperBound; i++) {
        int smallerCount = (int)std::floor(i * slope);
        int xInc = xDiffIsLarger ? i : smallerCount;
        int yInc = xDiffIsLarger ? smallerCount : i;

        int targetX = x + (xInc * xMod);
        int targetY = y + (yInc * yMod);

        if (world.inBounds(targetX, targetY)) {
            if (targetX == currentX && targetY == currentY) continue;

            bool isFinal = (i == upperBound);
            bool isFirst = (i == 1);
            
            if (actOnNeighbor(targetX, targetY, currentX, currentY, world, isFinal, isFirst, 0)) {
                break; 
            }
        } else {
            // Out of bounds -> Kill particle
            world.setParticleAt(currentX, currentY, nullptr); 
            return;
        }
    }
}

bool MovableSolid::actOnNeighbor(int targetX, int targetY, int& currentX, int& currentY, ParticleWorld& world, bool isFinal, bool isFirst, int depth) {
    Particle* neighbor = world.getParticleAt(targetX, targetY);

    // Case 1: Empty
    if (neighbor->id == MaterialID::EmptyParticle) {
        setAdjacentNeighborsFreeFalling(currentX, currentY, world, depth);
        if (isFinal) {
            isFreeFalling = true;
            world.swapParticles(currentX, currentY, targetX, targetY);
            currentX = targetX;
            currentY = targetY;
        }
        return false; 
    }

    // Case 2: Liquid Displacement
    bool isLiquid = (neighbor->id == MaterialID::Water || neighbor->id == MaterialID::Oil || 
                     neighbor->id == MaterialID::Lava || neighbor->id == MaterialID::Acid);
    if (isLiquid) {
        isFreeFalling = true;
        world.swapParticles(currentX, currentY, targetX, targetY);
        currentX = targetX;
        currentY = targetY;
        return true; 
    }

    // Case 3: Solid Collision & Sliding
    if (depth > 0) return true;
    if (isFinal) return true;

    if (isFreeFalling) {
        float absY = std::max(std::abs(velocity.y) / 31.0f, 105.0f);
        velocity.x = (velocity.x < 0) ? -absY : absY;
    }

    sf::Vector2f normVel = velocity;
    float len = std::sqrt(normVel.x*normVel.x + normVel.y*normVel.y);
    if (len != 0) normVel /= len;

    int addX = getAdditional(normVel.x);
    int addY = getAdditional(normVel.y);

    if (isFirst) {
        velocity.y = getAverageVelOrGravity(velocity.y, neighbor->velocity.y);
    } else {
        velocity.y = 124.0f; // SFML: Positive push down
    }

    neighbor->velocity.y = velocity.y;
    velocity.x *= frictionFactor * neighbor->frictionFactor;

    // A. Diagonal Sliding (e.g., forming a pile)
    int diagX = currentX + addX;
    int diagY = currentY + addY;
    if (world.inBounds(diagX, diagY)) {
        Particle* diagNeighbor = world.getParticleAt(diagX, diagY);
        bool stoppedDiag = actOnNeighbor(diagX, diagY, currentX, currentY, world, true, false, depth + 1);
        if (!stoppedDiag) {
            isFreeFalling = true;
            return true; 
        }
    }

    // B. Adjacent Sliding (Horizontal shift)
    int adjX = currentX + addX;
    if (world.inBounds(adjX, currentY)) {
        Particle* adjNeighbor = world.getParticleAt(adjX, currentY);
        bool stoppedAdj = actOnNeighbor(adjX, currentY, currentX, currentY, world, true, false, depth + 1);
        if (stoppedAdj) {
            velocity.x *= -1; 
        } else {
            isFreeFalling = false;
            return true;
        }
    }

    isFreeFalling = false;
    return true; 
}

void MovableSolid::setAdjacentNeighborsFreeFalling(int x, int y, ParticleWorld& world, int depth) {
    if (depth > 0) return;
    int checks[2] = {1, -1};
    for (int dx : checks) {
        if (world.inBounds(x + dx, y)) {
            Particle* p = world.getParticleAt(x + dx, y);
            if (p->id != MaterialID::EmptyParticle) {
                if (Random::randFloat(0, 1) > p->inertialResistance) {
                    p->isFreeFalling = true;
                }
            }
        }
    }
}

int MovableSolid::getAdditional(float val) {
    if (val < -0.1f) return -1;
    if (val > 0.1f) return 1;
    return 0;
}

float MovableSolid::getAverageVelOrGravity(float myVel, float otherVel) {
    // SFML logic: if neighbor is falling slower than max (124), use 124
    if (otherVel < 125.0f) return 124.0f;
    float avg = (myVel + otherVel) / 2.0f;
    // Cap to terminal velocity
    return (avg < 0) ? avg : std::min(avg, 124.0f);
}

// --- Specific Particle Logic ---

void Gunpowder::update(int x, int y, float dt, ParticleWorld& world) {
    MovableSolid::update(x, y, dt, world);
    if (isIgnited) {
        ignitedCount++;
        // if (ignitedCount >= 7) { world.explode(x, y, 15); }
    }
}

void Snow::update(int x, int y, float dt, ParticleWorld& world) {
    MovableSolid::update(x, y, dt, world);
    // SFML: If falling faster than 62 down, jitter speed
    if (velocity.y > 62.0f) {
        velocity.y = (Random::randFloat(0, 1) > 0.3f) ? 62.0f : 124.0f;
    }
}