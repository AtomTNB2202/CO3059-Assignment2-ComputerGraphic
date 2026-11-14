#include "Scene.h"
#include <algorithm>
#include <filesystem>

Object *Scene::selection()
{
    for (auto &object : objects)
        if (object.id == selected)
            return &object;
    return nullptr;
}
void Scene::add(Object object)
{
    if (object.categoryName.empty())
    {
        if (object.kind == ObjectKind::Imported)
            object.categoryName = std::filesystem::u8path(object.modelPath).stem().u8string();
        else if (object.kind == ObjectKind::Shape)
            object.categoryName = shapeNames[static_cast<int>(object.shape)];
        else if (object.kind == ObjectKind::Surface) object.categoryName = "surface";
        else if (object.kind == ObjectKind::Atom) object.categoryName = "atom";
        else object.categoryName = "molecule";
        if (object.categoryName.empty()) object.categoryName = "object";
    }
    if (!object.categoryId)
    {
        int nextCategory = 1;
        for (const auto &existing : objects)
        {
            nextCategory = std::max(nextCategory, existing.categoryId + 1);
            if (existing.categoryName == object.categoryName)
                object.categoryId = existing.categoryId;
        }
        if (!object.categoryId) object.categoryId = nextCategory;
    }
    object.id = nextId++;
    selected = object.id;
    objects.push_back(std::move(object));
}
void Scene::eraseSelected()
{
    objects.erase(std::remove_if(objects.begin(), objects.end(),
                                 [this](const Object &object) { return object.id == selected; }),
                  objects.end());
    selected = objects.empty() ? -1 : objects.back().id;
}

void Scene::addAt(Object object, glm::vec3 position)
{
    object.transform.position = position;
    add(std::move(object));
}

void Scene::translateSelected(glm::vec3 delta)
{
    if (auto *object = selection(); object && object->visible)
        object->transform.position += delta;
}
