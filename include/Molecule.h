#pragma once
#include "Scene.h"

struct Element
{
    const char *name;
    const char *symbol;
    int electrons;
    glm::vec3 color;
    float radius;
};
inline const Element elements[] = {
    {"Hydrogen", "H", 1, {0.93f, 0.95f, 1.0f}, 0.3f}, {"Helium", "He", 2, {0.6f, 0.9f, 1.0f}, 0.35f},
    {"Carbon", "C", 6, {0.16f, 0.19f, 0.24f}, 0.48f}, {"Nitrogen", "N", 7, {0.2f, 0.4f, 1.0f}, 0.45f},
    {"Oxygen", "O", 8, {0.95f, 0.16f, 0.2f}, 0.46f},  {"Neon", "Ne", 10, {0.7f, 0.45f, 1.0f}, 0.48f}};
inline constexpr int elementCount = 6;
Object makeAtom(int element);
Object makeMolecule(bool water);
glm::mat4 bondMatrix(glm::vec3 first, glm::vec3 second, float radius);
