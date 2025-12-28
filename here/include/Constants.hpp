#pragma once
#include <SFML/Graphics/Color.hpp>
#include <string>
#include <vector>
#include <functional>
#include <memory>

class Particle; 

// Window settings
constexpr unsigned int TEXTURE_WIDTH = 1280;  
constexpr unsigned int TEXTURE_HEIGHT = 720; 

// Keep the window size standard
constexpr unsigned int WINDOW_WIDTH = 1280;
constexpr unsigned int WINDOW_HEIGHT = 720;

// Physics settings
constexpr float GRAVITY = 800.0f;
constexpr float DEFAULT_SELECTION_RADIUS = 10.0f;
constexpr float MIN_SELECTION_RADIUS = 1.0f;
constexpr float MAX_SELECTION_RADIUS = 100.0f;


// --- THE MASTER LIST ---
// Define everything here. 1st param: Class Name, 2nd param: Color
#define PARTICLE_DATA(X) \
    X(Sand,          sf::Color(150, 100, 50))  \
    X(Water,         sf::Color(20, 100, 170))  \
    X(Stone,         sf::Color(120, 110, 120)) \
    X(Wood,          sf::Color(60, 40, 20))    \
    X(Salt,          sf::Color(200, 180, 190)) \
    X(Smoke,         sf::Color(50, 50, 50))    \
    X(Steam,         sf::Color(220, 220, 250)) \
    X(Gunpowder,     sf::Color(60, 60, 60))    \
    X(Oil,           sf::Color(80, 70, 60))    \
    X(Lava,          sf::Color(200, 50, 0))    \
    X(Acid,          sf::Color(90, 200, 60))   \
    X(Snow,          sf::Color(255, 250, 250)) \
    X(Dirt,          sf::Color(182, 159, 102)) \
    X(Coal,          sf::Color(115, 116, 115)) \
    X(Ember,         sf::Color(200, 120, 20))  \
    X(Cement,         sf::Color(165, 163, 145))  \
    X(Blood,         sf::Color(136, 8, 8))  \
    X(FlammableGas,         sf::Color(0, 255, 0))  \
    X(Spark,         sf::Color(89, 35, 14))  \
    X(ExplosionSpark,         sf::Color(255, 165, 0))  \
    X(SlimeMold,         sf::Color(201, 58, 107))  \
    X(Brick,         sf::Color(188, 3, 0))  \
    X(EmptyParticle, sf::Color(0, 0, 0))

enum class MaterialGroup { MovableSolid, ImmovableSolid, Liquid, Gas, Special };

// Generate the Enum automatically
enum class MaterialID : uint8_t {
    #define X(name, color) name,
    PARTICLE_DATA(X)
    #undef X
};

struct MaterialProps {
    MaterialID id;
    std::string name;
    sf::Color color;
    MaterialGroup group;
    std::function<std::unique_ptr<Particle>()> create;
};

extern const std::vector<MaterialProps> ALL_MATERIALS;

inline const MaterialProps& GetProps(MaterialID id) {
    for (const auto& p : ALL_MATERIALS) if (p.id == id) return p;
    return ALL_MATERIALS.back(); 
}