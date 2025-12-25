#include "ParticleWorld.hpp"
#include "Constants.hpp"
#include "Random.hpp"
#include <algorithm> 
#include <cmath> 
#include "Particles/Gas.hpp"
#include "Particles/Liquid.hpp"
void Gas::update(int x, int y, float dt, ParticleWorld& world) 
{

    velocity.y = std::clamp(velocity.y - (GRAVITY * dt * buoyancy), -5.0f, 2.0f);

    // Apply Chaos (Horizontal Drift)
    velocity.x += Random::randFloat(-chaosLevel, chaosLevel);
    velocity.x = std::clamp(velocity.x, -3.0f, 3.0f);

    // Occasional Turbulence
    if (Random::chance(5)) {
        velocity.x += Random::randFloat(-1.0f, 1.0f);
        velocity.y += Random::randFloat(-0.5f, 0.5f);
    }

    // Calculate Target
    int targetX = x + static_cast<int>(std::round(velocity.x));
    int targetY = y + static_cast<int>(std::round(velocity.y));

    // Helper lambda to check if gas can move into a cell
    // Gases can move into Empty, Water, or Oil (pass through liquids)
    auto canMoveInto = [&](int tx, int ty) -> bool {
        if (!world.inBounds(tx, ty)) return false;
        Particle* p = world.getParticleAt(tx, ty);
        return (p->id == MaterialID::EmptyParticle || 
                p->id == MaterialID::Water || 
                p->id == MaterialID::Oil);
    };

    // 1. Try Direct Movement
    if (canMoveInto(targetX, targetY)) {
        world.swapParticles(x, y, targetX, targetY);
        return;
    }

    // 2. Try Upward Movement
    if (canMoveInto(x, y - 1)) {
        world.swapParticles(x, y, x, y - 1);
        return;
    }

    // 3. Try Horizontal Movement (Drift)
    int direction = (velocity.x > 0) ? 1 : -1;
    // Add randomness if velocity is low
    if (std::abs(velocity.x) < 0.1f) direction = Random::randBool() ? 1 : -1;

    if (canMoveInto(x + direction, y)) {
        velocity.x += direction * 0.5f;
        world.swapParticles(x, y, x + direction, y);
        return;
    }
    
    // Try opposite if blocked
    if (canMoveInto(x - direction, y)) {
        velocity.x -= direction * 0.5f;
        world.swapParticles(x, y, x - direction, y);
        return;
    }

    // 4. Try Diagonal Upward
    if (canMoveInto(x + 1, y - 1)) {
         world.swapParticles(x, y, x + 1, y - 1);
         return;
    }
    if (canMoveInto(x - 1, y - 1)) {
         world.swapParticles(x, y, x - 1, y - 1);
         return;
    }

    // 5. Friction
    velocity.x *= 0.8f;
    velocity.y *= 0.9f;
}

    void Steam::update(int x, int y, float dt, ParticleWorld& world) {
        if (hasBeenUpdatedThisFrame) return;
        hasBeenUpdatedThisFrame = true;

        // 1. Lifetime & Fading
        if (lifeTime > 12.0f) {
            world.setParticleAt(x, y, std::make_unique<EmptyParticle>());
            return;
        }

        // Fade alpha
        float lifeFactor = std::clamp((12.0f - lifeTime) / 12.0f, 0.1f, 1.0f);
        color.a = static_cast<uint8_t>(lifeFactor * 255 * 0.8f);

        // 2. Condensation (Steam -> Water)
        if (lifeTime > 8.0f && Random::chance(200)) {
            world.setParticleAt(x, y, std::make_unique<Water>());
            return;
        }

        // 3. Movement
        // Uses the base Gas implementation which contains updateGasMovement logic
        Gas::update(x, y, dt, world);
    }