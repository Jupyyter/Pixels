#include "Solid.hpp"
#pragma once
class ImmovableSolid : public Solid
{
public:
    ImmovableSolid(MaterialID id) : Solid(id) { this->isFreeFalling = false; }
    static MaterialGroup getStaticGroup() { return MaterialGroup::ImmovableSolid; }
    MaterialGroup getGroup() const override
    {
        return getStaticGroup();
    }
    // Even static particles get an update as requested
    void update(int x, int y, float dt, ParticleWorld &world) override
    {
        world.updateParticleColor(this, world);
        applyHeatToNeighborsIfIgnited(world);
        takeEffectsDamage(world);
        spawnSparkIfIgnited(world);

    }
};
class Stone : public ImmovableSolid
{
public:
    Stone() : ImmovableSolid(MaterialID::Stone)
    {
        frictionFactor = 0.5f;
        inertialResistance = 1.1f;
        mass = 500;
        explosionResistance = 4;
    }
    bool receiveHeat(int heat) override {
        return false; 
    }

    std::unique_ptr<Particle> clone() const override { return std::make_unique<Stone>(*this); }
};
class Brick : public ImmovableSolid
{
public:
    Brick() : ImmovableSolid(MaterialID::Brick)
    {
        frictionFactor = 0.5f;
        inertialResistance = 1.1f;
        mass = 500;
        explosionResistance = 4;
    }
    bool receiveHeat(int heat) override {
        return false; 
    }

    std::unique_ptr<Particle> clone() const override { return std::make_unique<Brick>(*this); }
};
class SlimeMold : public ImmovableSolid
{
public:
    SlimeMold() : ImmovableSolid(MaterialID::SlimeMold)
    {
        frictionFactor = 0.5f;
        inertialResistance = 1.1f;
        mass = 500;
        flammabilityResistance = 10;
        resetFlammabilityResistance = 0;
        health = 40;
    }
    std::unique_ptr<Particle> clone() const override { return std::make_unique<SlimeMold>(*this); }
};
class Wood : public ImmovableSolid
{
public:
    Wood() : ImmovableSolid(MaterialID::Wood) { 
        frictionFactor = 0.5f;
        inertialResistance = 1.1f;
        mass = 500;
        health = (std::rand() % 100) + 100;
        flammabilityResistance = 40;
        resetFlammabilityResistance = 25; 
    }
    std::unique_ptr<Particle> clone() const override { return std::make_unique<Wood>(*this); }

    void checkIfDead(ParticleWorld &world) override
    {
        if (health <= 0)
        {
            if (isIgnited && (static_cast<float>(std::rand()) / RAND_MAX) > 0.95f)
            {
                dieAndReplace(MaterialID::Ember, world);
            }
            else
            {
                die(world);
            }
        }
    }
};