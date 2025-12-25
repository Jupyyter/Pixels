#include "Random.hpp"

    std::random_device Random::rd{};
    std::mt19937 Random::gen{};
    bool Random::initialized = false;