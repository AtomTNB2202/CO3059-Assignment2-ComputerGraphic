#pragma once
#include "Mesh.h"
#include "Texture.h"
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <string>
#include <vector>

enum class RenderMode
{
    Flat,
    Gouraud,
    Phong,
    Texture,
    Wireframe
};
inline constexpr const char *renderModeNames[] = {"Flat color", "Gouraud", "Phong", "Texture", "Wireframe"};

struct Transform
{
    glm::vec3 position{0}, rotation{0}, scale{1};
    glm::mat4 matrix() const
    {
        glm::mat4 model = glm::translate(glm::mat4(1), position);
        model = glm::rotate(model, glm::radians(rotation.z), {0, 0, 1});
        model = glm::rotate(model, glm::radians(rotation.y), {0, 1, 0});
        model = glm::rotate(model, glm::radians(rotation.x), {1, 0, 0});
        return glm::scale(model, scale);
    }
};
struct Material
{
    glm::vec3 color{0.15f, 0.68f, 0.82f};
    float shininess = 32;
    RenderMode mode = RenderMode::Phong;
    std::shared_ptr<Texture> texture;
    std::string texturePath;
};
enum class ObjectKind
{
    Shape,
    Surface,
    Imported,
    Atom,
    Molecule
};
struct AtomSite
{
    int element = 0;
    glm::vec3 position{0};
};
struct Bond
{
    int first = 0, second = 1;
};
struct Object
{
    int id = 0;
    std::string name;
    // Zero/empty requests automatic category assignment when adding an object.
    int categoryId = 0;
    std::string categoryName;
    ObjectKind kind = ObjectKind::Shape;
    Transform transform;
    Material material;
    std::shared_ptr<Mesh> mesh;
    Shape shape = Shape::Cube;
    std::string surfaceExpression = "sin(x)*cos(y)", modelPath;
    float surfaceExtent = 2;
    int surfaceResolution = 48;
    bool visible = true;
    bool randomizeTransform = true;
    int element = 0;
    std::vector<AtomSite> atoms;
    std::vector<Bond> bonds;
};
Bounds objectBounds(const Object &object);
struct Scene
{
    std::vector<Object> objects;
    int selected = -1, nextId = 1;
    Object *selection();
    void add(Object object);
    void addAt(Object object, glm::vec3 position);
    void translateSelected(glm::vec3 delta);
    void eraseSelected();
};
