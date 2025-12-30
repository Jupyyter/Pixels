#include "Particles/Liquid.hpp"
#include "ParticleWorld.hpp"
#include "Constants.hpp"
#include "Random.hpp"
#include "Particles/Solid.hpp"
#include <algorithm> 
#include <cmath> 

// Terminal velocity constants
static const float MAX_VEL_Y = 124.0f;
static const float BOUNCE_VEL_Y = 62.0f;

void Liquid::update(int x, int y, float dt, ParticleWorld& world) 
{
    if (hasBeenUpdatedThisFrame) return;
    hasBeenUpdatedThisFrame = true;
    
    // --- 1. Gravity & Friction ---
    velocity.y += GRAVITY * dt;
    if (velocity.y > MAX_VEL_Y) velocity.y = MAX_VEL_Y;

    if (isFreeFalling) {
        velocity.x *= 0.8f; 
    }

    // --- 2. Calculate Thresholds (Sub-pixel accumulation) ---
    float velXDeltaTimeFloat = velocity.x * dt;
    float velYDeltaTimeFloat = velocity.y * dt;

    xThreshold += velXDeltaTimeFloat;
    yThreshold += velYDeltaTimeFloat;

    int velXDeltaTime = static_cast<int>(xThreshold);
    int velYDeltaTime = static_cast<int>(yThreshold);

    xThreshold -= velXDeltaTime;
    yThreshold -= velYDeltaTime;

    int xModifier = (velXDeltaTime < 0) ? -1 : 1;
    int yModifier = (velYDeltaTime < 0) ? -1 : 1;

    int absX = std::abs(velXDeltaTime);
    int absY = std::abs(velYDeltaTime);

    // --- 3. Vector Pathing ---
    int upperBound = std::max(absX, absY);
    
    if (upperBound == 0) {
        if (world.isEmpty(x, y + 1)) {
            isFreeFalling = true; 
        } else {
            velocity.x *= 0.5f;
        }
    }

    int currentX = x;
    int currentY = y;
    
    int minBound = std::min(absX, absY);
    float slope = (upperBound == 0) ? 0.0f : ((float)(minBound) / (upperBound));
    bool xDiffIsLarger = absX > absY;
    sf::Vector2i formerLocation(x, y);

    for (int i = 1; i <= upperBound; i++) {
        int smallerCount = (int)std::floor(i * slope);
        int xIncrease, yIncrease;

        if (xDiffIsLarger) {
            xIncrease = i; yIncrease = smallerCount;
        } else {
            yIncrease = i; xIncrease = smallerCount;
        }

        int modifiedMatrixX = x + (xIncrease * xModifier);
        int modifiedMatrixY = y + (yIncrease * yModifier);

        if (world.inBounds(modifiedMatrixX, modifiedMatrixY)) {
            if (modifiedMatrixX == currentX && modifiedMatrixY == currentY) continue;

            bool isFinal = (i == upperBound);
            bool isFirst = (i == 1);
            
            bool stopped = actOnNeighbor(modifiedMatrixX, modifiedMatrixY, currentX, currentY, world, isFinal, isFirst, 0);
            if (stopped) break;
        } else {
            // Out of bounds: delete particle

            return;
        }
    }

    world.updateParticleColor(this, world);
    applyHeatToNeighborsIfIgnited(world);
    spawnSparkIfIgnited(world);
    takeEffectsDamage(world);

    stoppedMovingCount = didNotMove(formerLocation) ? stoppedMovingCount + 1 : 0;
    if (stoppedMovingCount > stoppedMovingThreshold) {
        stoppedMovingCount = stoppedMovingThreshold;
    }
}

bool Liquid::actOnNeighbor(int targetX, int targetY, int& currentX, int& currentY, 
                           ParticleWorld& world, bool isFinal, bool isFirst, int depth) 
{
    Particle* neighbor = world.getParticleAt(targetX, targetY);

    // 1. Interaction (actOnOther)
    if (neighbor) {
        if (this->actOnOther(neighbor, world)) return true; 
    }

    // 2. Empty Space (NULL)
    if (!neighbor) { 
        if (isFinal) {
            isFreeFalling = true;
            world.moveParticle(currentX, currentY, targetX, targetY);
            currentX = targetX;
            currentY = targetY;
            return false; 
        } else {
            return false; 
        }
    }
    
    // 3. Liquid Interaction (Density Swap)
    if (neighbor->getGroup() == MaterialGroup::Liquid) {
        Liquid* liquidNeighbor = static_cast<Liquid*>(neighbor);
        if (this->density > liquidNeighbor->density) {
            if (isFinal) {
                this->velocity.y = BOUNCE_VEL_Y; 
                if (Random::randFloat(0,1) > 0.8f) this->velocity.x *= -1;
                
                world.swapParticles(currentX, currentY, targetX, targetY);
                currentX = targetX;
                currentY = targetY;
                return true; 
            } else {
                return false; 
            }
        }
    }

    // 4. Blocked -> Dispersion Logic
    if (depth > 0 || isFinal) return true;

    // Friction / Bounce on impact
    if (isFreeFalling) {
        float absY = std::max(std::abs(velocity.y) / 31.0f, 105.0f);
        velocity.x = (velocity.x < 0) ? -absY : absY;
    }

    sf::Vector2f normVel = velocity;
    float len = std::sqrt(normVel.x*normVel.x + normVel.y*normVel.y);
    if (len != 0) normVel /= len;

    int additionalX = getAdditional(normVel.x);
    int dist = additionalX * (Random::randBool() ? dispersionRate + 2 : dispersionRate - 1);

    // Velocity Transfer
    if (isFirst) {
        velocity.y = getAverageVelOrGravity(velocity.y, neighbor->velocity.y);
    } else {
        velocity.y = MAX_VEL_Y; 
    }
    
    neighbor->velocity.y = velocity.y;
    velocity.x *= frictionFactor;

    // A. Try Diagonal (Down-Left or Down-Right)
    int diagX = currentX + additionalX;
    int diagY = currentY + 1; 
    if (world.inBounds(diagX, diagY)) {
        if (!iterateToAdditional(world, diagX, diagY, dist, currentX, currentY)) {
            isFreeFalling = true;
            return true; 
        }
    }

    // B. Try Adjacent (Left or Right)
    int adjX = currentX + additionalX;
    int adjY = currentY; 
    if (world.inBounds(adjX, adjY)) {
        if (iterateToAdditional(world, adjX, adjY, dist, currentX, currentY)) {
             velocity.x *= -1; // Hit a wall
        } else {
            isFreeFalling = false;
            return true; 
        }
    }

    isFreeFalling = false;
    return true; 
}

bool Liquid::iterateToAdditional(ParticleWorld& world, int startX, int startY, int distance, int& currentX, int& currentY) 
{
    int distanceModifier = (distance > 0) ? 1 : -1;
    int absDist = std::abs(distance);

    int lastValidX = currentX;
    int lastValidY = currentY;
    
    for (int i = 0; i <= absDist; i++) {
        int modifiedX = startX + (i * distanceModifier);
        if (!world.inBounds(modifiedX, startY)) return true; 

        Particle* neighbor = world.getParticleAt(modifiedX, startY);
        if (neighbor && actOnOther(neighbor, world)) return false; 

        bool isFinal = (i == absDist);

        if (!neighbor) { // Empty
            if (isFinal) {
                world.moveParticle(currentX, currentY, modifiedX, startY);
                currentX = modifiedX;
                currentY = startY;
                return false; 
            }
            lastValidX = modifiedX;
            lastValidY = startY;
        } 
        else if (neighbor->getGroup() == MaterialGroup::Liquid) {
            Liquid* liq = static_cast<Liquid*>(neighbor);
            if (isFinal && this->density > liq->density) { 
                world.swapParticles(currentX, currentY, modifiedX, startY);
                currentX = modifiedX;
                currentY = startY;
                this->velocity.y = BOUNCE_VEL_Y;
                if (Random::randFloat(0,1) > 0.8f) this->velocity.x *= -1;
                return false; 
            }
        }
        else { // Solid or non-swappable
            if (i == 0) return true; // Blocked immediately
            if (lastValidX != currentX || lastValidY != currentY) {
                world.moveParticle(currentX, currentY, lastValidX, lastValidY);
                currentX = lastValidX;
                currentY = lastValidY;
            }
            return false; 
        }
    }
    return true; 
}

int Liquid::getAdditional(float val) {
    if (val < -0.1f) return (int)std::floor(val);
    if (val > 0.1f) return (int)std::ceil(val);
    return 0;
}

float Liquid::getAverageVelOrGravity(float myVel, float otherVel) {
    if (otherVel < (MAX_VEL_Y + 1.0f)) return MAX_VEL_Y;
    float avg = (myVel + otherVel) / 2.0f;
    if (avg < 0) return avg;
    return std::min(avg, MAX_VEL_Y);
}