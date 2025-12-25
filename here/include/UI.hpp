#pragma once
#include <SFML/Graphics.hpp>
#include "Constants.hpp"

class ParticleWorld;

class UI {
public:
    UI(sf::RenderWindow& window, ParticleWorld* worldPtr);
    
    void update(sf::RenderWindow& window, sf::Time deltaTime, bool& simRunning, float frameTime);
    void render(sf::RenderWindow& window);

    // Getters for SandSimApp
    MaterialID getCurrentMaterialID() const { return currentMaterial; }
    float getSelectionRadius() const { return selectionRadius; }
    bool isMouseOverUI() const; 

    // Logic for rigid bodies (simplified for this example)
    bool isCurrentSelectionRigidBody() const { return false; } 

private:
    ParticleWorld* world;
    MaterialID currentMaterial = MaterialID::Sand;
    float selectionRadius = DEFAULT_SELECTION_RADIUS;
    
    // Internal helper to draw material tabs
    void drawMaterialTabs();
};