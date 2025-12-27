#pragma once
#include <vector>
#include "Constants.hpp"

class MaterialRegistry {
public:
    static const std::vector<MaterialProps> materials;
};

// Global helper to access props easily
const MaterialProps& GetProps(MaterialID id);