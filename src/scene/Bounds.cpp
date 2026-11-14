#include "Scene.h"
#include "Molecule.h"
#include <limits>

Bounds objectBounds(const Object &object)
{
    Bounds local{{-1, -1, -1}, {1, 1, 1}};
    if (object.mesh)
        local = object.mesh->bounds();
    else if (object.kind == ObjectKind::Atom)
    {
        float radius = elements[object.element].electrons > 2 ? 1.655f : 1.005f;
        local = {{-radius, -radius, -0.35f}, {radius, radius, 0.35f}};
    }
    else if (object.kind == ObjectKind::Molecule && !object.atoms.empty())
    {
        local.low = glm::vec3(std::numeric_limits<float>::max());
        local.high = -local.low;
        for (const auto &atom : object.atoms)
        {
            glm::vec3 radius(elements[atom.element].radius);
            local.low = glm::min(local.low, atom.position - radius);
            local.high = glm::max(local.high, atom.position + radius);
        }
    }
    Bounds world{glm::vec3(std::numeric_limits<float>::max()), glm::vec3(-std::numeric_limits<float>::max())};
    auto model = object.transform.matrix();
    for (int corner = 0; corner < 8; ++corner)
    {
        glm::vec3 point{corner & 1 ? local.high.x : local.low.x, corner & 2 ? local.high.y : local.low.y,
                        corner & 4 ? local.high.z : local.low.z};
        auto transformed = glm::vec3(model * glm::vec4(point, 1));
        world.low = glm::min(world.low, transformed);
        world.high = glm::max(world.high, transformed);
    }
    return world;
}
