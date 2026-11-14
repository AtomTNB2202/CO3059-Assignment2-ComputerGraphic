#include "Molecule.h"
#include <glm/gtc/quaternion.hpp>
#include <stdexcept>

Object makeAtom(int element)
{
    if (element < 0 || element >= elementCount)
        throw std::runtime_error("Unknown element.");
    Object object;
    object.name = std::string(elements[element].name) + " (Bohr)";
    object.kind = ObjectKind::Atom;
    object.element = element;
    return object;
}
Object makeMolecule(bool water)
{
    Object object;
    object.kind = ObjectKind::Molecule;
    if (water)
    {
        object.name = "H2O - water";
        // Approximate 104.5-degree H-O-H bond angle, scaled for readability.
        object.atoms = {{4, {0, 0, 0}}, {0, {-1.03f, 0.80f, 0}}, {0, {1.03f, 0.80f, 0}}};
        object.bonds = {{0, 1}, {0, 2}};
    }
    else
    {
        object.name = "CO2 - carbon dioxide";
        object.atoms = {{2, {0, 0, 0}}, {4, {-1.45f, 0, 0}}, {4, {1.45f, 0, 0}}};
        object.bonds = {{0, 1}, {0, 2}};
    }
    return object;
}
glm::mat4 bondMatrix(glm::vec3 first, glm::vec3 second, float radius)
{
    glm::vec3 delta = second - first;
    float length = glm::length(delta);
    if (length < 1e-5f)
        return glm::mat4(0);
    glm::vec3 direction = delta / length;
    glm::quat rotation;
    if (direction.y < -0.9999f)
        rotation = glm::angleAxis(glm::radians(180.0f), glm::vec3(1, 0, 0));
    else
        rotation = glm::normalize(glm::quat(1 + direction.y, glm::cross(glm::vec3(0, 1, 0), direction)));
    // Cylinder source radius is 0.7 and its height is 2.
    return glm::translate(glm::mat4(1), (first + second) * 0.5f) * glm::mat4_cast(rotation) *
           glm::scale(glm::mat4(1), {radius / 0.7f, length / 2, radius / 0.7f});
}
