#pragma once
#include "Particle.hpp"
#include "ParticleWorld.hpp"

class Gas : public Particle {
protected:
    float buoyancy;
    float chaosLevel;
    
public:
    Gas(MaterialID id, float buoy, float chaos);
    static MaterialGroup getStaticGroup() { return MaterialGroup::Gas; }
    MaterialGroup getGroup() const override { return getStaticGroup(); }
    
    // Core Update
    void update(int x, int y, float dt, ParticleWorld& world) override;

    // Interaction Logic
    bool actOnNeighbor(int targetX, int targetY, int& currentX, int& currentY, ParticleWorld& world, bool isFinal, bool isFirst, int depth) override;
    
    // Helpers
    bool compareGasDensities(Particle* neighbor);
    void swapGasForDensities(ParticleWorld& world, Particle* neighbor, int neighborX, int neighborY, int& currentX, int& currentY);
    
    // Default overrides
    void spawnSparkIfIgnited(ParticleWorld& world) override {}
    bool corrode(ParticleWorld& world) override { return false; }
};

class Steam : public Gas {
public:
    Steam() : Gas(MaterialID::Steam, 1.0f, 1.8f) {
        density = 5;
        dispersionRate = 2;
        mass = 1;
        frictionFactor = 1.0f;
        lifeSpan = Random::randInt(0, 2000) + 1000;
    }
    std::unique_ptr<Particle> clone() const override { return std::make_unique<Steam>(*this); }
    
    void checkLifeSpan(ParticleWorld& world) override;
    bool receiveHeat(int heat) override { return false; }
};

class FlammableGas : public Gas {
public:
    FlammableGas() : Gas(MaterialID::FlammableGas, 1.0f, 1.8f) {
        density = 1;
        dispersionRate = 2;
        lifeSpan = Random::randInt(0, 500) + 3000;
        flammabilityResistance = 10;
        resetFlammabilityResistance = 10;
        health = 100;
        mass = 1;
    }
    std::unique_ptr<Particle> clone() const override { return std::make_unique<FlammableGas>(*this); }
};

class Spark : public Gas {
public:
    Spark() : Gas(MaterialID::Spark, 1.0f, 1.8f) {
        density = 4;
        dispersionRate = 4;
        lifeSpan = Random::randInt(0, 20); 
        flammabilityResistance = 25;
        isIgnited = true;
        temperature = 3;
        mass = 10;
    }
    std::unique_ptr<Particle> clone() const override { return std::make_unique<Spark>(*this); }

    bool actOnNeighbor(int targetX, int targetY, int& currentX, int& currentY, ParticleWorld& world, bool isFinal, bool isFirst, int depth) override;
    bool receiveHeat(int heat) override { return false; }
};

class ExplosionSpark : public Gas {
public:
    ExplosionSpark() : Gas(MaterialID::ExplosionSpark, 1.0f, 2.0f) {
        density = 4;
        dispersionRate = 4;
        lifeSpan = Random::randInt(0, 20);
        flammabilityResistance = 25;
        isIgnited = true;
        temperature = 3;
        mass = 10;
    }
    std::unique_ptr<Particle> clone() const override { return std::make_unique<ExplosionSpark>(*this); }

    bool actOnNeighbor(int targetX, int targetY, int& currentX, int& currentY, ParticleWorld& world, bool isFinal, bool isFirst, int depth) override;
    bool receiveHeat(int heat) override { return false; }
    void modifyColor() {} 
};

class Smoke : public Gas {
public:
    Smoke() : Gas(MaterialID::Smoke, 0.8f, 1.2f) {
        density = 3;
        dispersionRate = 2;
        lifeSpan = Random::randInt(0, 250) + 450;
        mass = 1;
    }
    std::unique_ptr<Particle> clone() const override { return std::make_unique<Smoke>(*this); }
    
    bool receiveHeat(int heat) override { return false; }
};