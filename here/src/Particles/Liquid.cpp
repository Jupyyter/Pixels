#include "ParticleWorld.hpp"
#include "Constants.hpp"
#include "Random.hpp"
#include "Particles/Solid.hpp"
#include "Particles/Liquid.hpp"
#include "Particles/Gas.hpp"
#include <algorithm> 
#include <cmath> 

// Terminal velocity constant (Positive because Y is Down)
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

    // Accumulate fractional movement
    xThreshold += velXDeltaTimeFloat;
    yThreshold += velYDeltaTimeFloat;

    int velXDeltaTime = static_cast<int>(xThreshold);
    int velYDeltaTime = static_cast<int>(yThreshold);

    // Only subtract the whole pixels we are about to move
    xThreshold -= velXDeltaTime;
    yThreshold -= velYDeltaTime;

    // Directions
    int xModifier = (velXDeltaTime < 0) ? -1 : 1;
    int yModifier = (velYDeltaTime < 0) ? -1 : 1;

    int absX = std::abs(velXDeltaTime);
    int absY = std::abs(velYDeltaTime);

    // --- 3. Vector Pathing ---
    int upperBound = std::max(absX, absY);
    
    // FIX: If velocity is too low to move a pixel, check if we ARE actually in the air.
    // This prevents "hanging" particles.
    if (upperBound == 0) {
        if (world.inBounds(x, y + 1) && world.isEmpty(x, y + 1)) {
            isFreeFalling = true; // Ensure gravity keeps building
        } else {
            // Not in air, and not moving? Kill horizontal momentum slightly
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
            xIncrease = i;
            yIncrease = smallerCount;
        } else {
            yIncrease = i;
            xIncrease = smallerCount;
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
            world.setParticleAt(currentX, currentY, nullptr); 
            return;
        }
    }
    world.updateParticleColor(this,world);
    applyHeatToNeighborsIfIgnited(world);
    spawnSparkIfIgnited(world);
    //checkLifeSpan(world);
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
    if (!neighbor) return true; // Safety

    // --- 0. Chemical/Elemental Interaction ---
    // This matches Java: "boolean acted = actOnOther(neighbor, matrix);"
    bool acted = this->actOnOther(neighbor, world);
    if (acted) {
        // If the interaction resulted in death or significant change, 
        // we stop physical movement for this step.
        return true; 
    }

    // 1. Empty or Gas (Free Fall)
    if (neighbor->id == MaterialID::EmptyParticle) { 
        if (isFinal) {
            isFreeFalling = true;
            world.swapParticles(currentX, currentY, targetX, targetY);
            currentX = targetX;
            currentY = targetY;
            return false; // Moved successfully
        } else {
            return false; 
        }
    }
    
    // 2. Liquid Interaction
    Liquid* liquidNeighbor = dynamic_cast<Liquid*>(neighbor);
    if (liquidNeighbor) {
        // Density Check
        bool denser = this->density > liquidNeighbor->density; 
        
        if (denser) {
            if (isFinal) {
                // FIXED: Positive velocity is DOWN. 
                // We set a bounce velocity (slightly slower than max)
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

    // 3. Blocked -> DISPERSION Logic
    if (depth > 0) return true; 
    if (isFinal) return true;

    // --- Friction / Bounce ---
    if (isFreeFalling) {
        float absY = std::max(std::abs(velocity.y) / 31.0f, 105.0f); // Keep constants logic, just ensures high X spread on impact
        velocity.x = (velocity.x < 0) ? -absY : absY;
    }

    // Normalize velocity to get direction
    sf::Vector2f normVel = velocity;
    float len = std::sqrt(normVel.x*normVel.x + normVel.y*normVel.y);
    if (len != 0) normVel /= len;

    int additionalX = getAdditional(normVel.x);
    // int additionalY = getAdditional(normVel.y); // Not strictly needed for the simplified slide logic

    int dist = additionalX * (Random::randBool() ? dispersionRate + 2 : dispersionRate - 1);

    // Velocity Transfer (Hydraulic Pressure)
    if (isFirst) {
        velocity.y = getAverageVelOrGravity(velocity.y, neighbor->velocity.y);
    } else {
        // FIXED: Cap to terminal velocity (Positive 124)
        velocity.y = MAX_VEL_Y; 
    }
    
    // Transfer momentum to neighbor (pushes water sideways/down)
    neighbor->velocity.y = velocity.y;
    velocity.x *= frictionFactor;

    // A. Try Diagonal (Down-Left or Down-Right)
    // In SFML Y-Down, "Down" is +1.
    int diagX = currentX + additionalX;
    int diagY = currentY + 1; 
    
    if (world.inBounds(diagX, diagY)) {
        bool stoppedDiagonally = iterateToAdditional(world, diagX, diagY, dist, currentX, currentY);
        if (!stoppedDiagonally) {
            isFreeFalling = true;
            return true; // Main loop stops, action handled in iterate
        }
    }

    // B. Try Adjacent (Left or Right)
    int adjX = currentX + additionalX;
    int adjY = currentY; 
    
    if (world.inBounds(adjX, adjY)) {
        bool stoppedAdjacently = iterateToAdditional(world, adjX, adjY, dist, currentX, currentY);
        
        if (stoppedAdjacently) {
             velocity.x *= -1; // Hit a wall, flip X
        } else {
            isFreeFalling = false;
            return true; 
        }
    }

    isFreeFalling = false;
    return true; // Blocked
}

bool Liquid::iterateToAdditional(ParticleWorld& world, int startX, int startY, int distance, int& currentX, int& currentY) 
{
    int distanceModifier = (distance > 0) ? 1 : -1;
    int absDist = std::abs(distance);

    int slideX = startX;
    int slideY = startY;
    
    // The "start" coordinates passed in are already offset by 1 step.
    // However, the loop logic in Java assumes checking from the offset outwards.

    for (int i = 0; i <= absDist; i++) {
        // Calculate the next spot in the slide
        int modifiedX = startX + (i * distanceModifier);
        
        if (!world.inBounds(modifiedX, startY)) return true; 

        Particle* neighbor = world.getParticleAt(modifiedX, startY);
        bool acted = actOnOther(neighbor, world);
        if (acted) return false; 

        bool isFinal = (i == absDist);
        bool isFirst = (i == 0);

        if (neighbor->id == MaterialID::EmptyParticle) {
            if (isFinal) {
                // Perform the physical move
                world.swapParticles(currentX, currentY, modifiedX, startY);
                currentX = modifiedX;
                currentY = startY;
                return false; // Success, not stopped
            }
            // If not final, we track this as a valid potential spot
            slideX = modifiedX;
            slideY = startY;
        } 
        else if (Liquid* liq = dynamic_cast<Liquid*>(neighbor)) {
            if (isFinal) {
                if (this->density > liq->density) { 
                    world.swapParticles(currentX, currentY, modifiedX, startY);
                    currentX = modifiedX;
                    currentY = startY;
                    
                    // FIXED: Bounce velocity is positive (Down)
                    this->velocity.y = BOUNCE_VEL_Y;
                    if (Random::randFloat(0,1) > 0.8f) this->velocity.x *= -1;
                    
                    return false; 
                }
            }
        }
        else if (dynamic_cast<Solid*>(neighbor)) { 
            if (isFirst) return true; // Blocked immediately
            
            // We hit a solid partway through the slide.
            // Move to the last valid Empty spot we saw.
            if (slideX != currentX || slideY != currentY) {
                world.swapParticles(currentX, currentY, slideX, slideY);
                currentX = slideX;
                currentY = slideY;
            }
            return false; // Handled
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
    // If neighbor is falling slower than terminal velocity (e.g. < 125),
    // set to Terminal Velocity (124).
    if (otherVel < (MAX_VEL_Y + 1.0f)) {
        return MAX_VEL_Y;
    }
    
    // Average
    float avg = (myVel + otherVel) / 2.0f;
    
    // If moving UP (negative), allow it (splash).
    if (avg < 0) return avg;
    
    // Else clamp to Max Downward Velocity
    return std::min(avg, MAX_VEL_Y);
}
