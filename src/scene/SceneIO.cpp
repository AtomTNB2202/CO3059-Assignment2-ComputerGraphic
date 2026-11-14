#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include "SceneIO.h"
#include "Molecule.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <chrono>
#include <cmath>
#include <stdexcept>
#include <set>

namespace
{
using Json = nlohmann::json;
Json vectorJson(glm::vec3 value)
{
    return Json::array({value.x, value.y, value.z});
}
float number(const Json &value, float low, float high)
{
    if (!value.is_number())
        throw std::runtime_error("Expected a number in scene file.");
    double result = value.get<double>();
    if (!std::isfinite(result) || result < low || result > high)
        throw std::runtime_error("Scene value outside supported range.");
    return static_cast<float>(result);
}
int integer(const Json &value, int low, int high)
{
    if (!value.is_number_integer())
        throw std::runtime_error("Expected an integer in scene file.");
    auto result = value.get<long long>();
    if (result < low || result > high)
        throw std::runtime_error("Scene index outside supported range.");
    return static_cast<int>(result);
}
glm::vec3 vectorValue(const Json &value, float low = -1000000, float high = 1000000)
{
    if (!value.is_array() || value.size() != 3)
        throw std::runtime_error("Scene vector needs three numbers.");
    return {number(value[0], low, high), number(value[1], low, high), number(value[2], low, high)};
}
std::string relativeAsset(const std::string &source, const std::filesystem::path &folder)
{
    if (source.empty())
        return {};
    auto absolute = std::filesystem::absolute(std::filesystem::u8path(source));
    std::error_code error;
    auto relative = std::filesystem::relative(absolute, folder, error);
    return error || relative.empty() ? absolute.u8string() : relative.generic_u8string();
}
std::string resolveAsset(const Json &value, const std::filesystem::path &folder)
{
    auto name = value.get<std::string>();
    if (name.empty())
        return {};
    auto path = std::filesystem::u8path(name);
    if (path.is_relative())
        path = folder / path;
    if (!std::filesystem::is_regular_file(path))
        throw std::runtime_error("Missing scene asset: " + path.u8string());
    return std::filesystem::absolute(path).lexically_normal().u8string();
}
} // namespace

void saveScene(const std::filesystem::path &path, const SceneDocument &document)
{
    if (document.scene.objects.size() > 256)
        throw std::runtime_error("Scene save limit is 256 objects.");
    auto folder = std::filesystem::absolute(path).parent_path();
    Json data;
    data["format"] = "CGAssignment1";
    data["version"] = 1;
    const auto &camera = document.camera;
    data["camera"] = {{"target", vectorJson(camera.target)},
                      {"yaw", camera.yaw},
                      {"pitch", camera.pitch},
                      {"distance", camera.distance},
                      {"moveSpeed", camera.moveSpeed},
                      {"frontView", camera.frontView}};
    data["animation"] = {{"playing", document.animation.playing},
                         {"speed", document.animation.speed},
                         {"time", document.animation.time}};
    data["background"] = vectorJson(document.background);
    data["lightPosition"] = vectorJson(document.lightPosition);
    data["showGrid"] = document.showGrid;
    data["next_instance_id"] = document.scene.nextId;
    data["selectedIndex"] = -1;
    data["objects"] = Json::array();
    for (const auto &object : document.scene.objects)
    {
        if (object.categoryId < 1 || object.categoryId > 1000000 || object.categoryName.empty() ||
            object.categoryName.size() > 128 ||
            object.categoryName.find_first_not_of(" \t\r\n") == std::string::npos)
            throw std::runtime_error("Set a category ID (1..1000000) and name before saving the scene.");
        if (object.id == document.scene.selected)
            data["selectedIndex"] = data["objects"].size();
        Json item = {{"name", object.name},
                     {"instance_id", object.id},
                     {"category_id", object.categoryId},
                     {"category_name", object.categoryName},
                     {"kind", static_cast<int>(object.kind)},
                     {"visible", object.visible},
                     {"randomize_transform", object.randomizeTransform},
                     {"position", vectorJson(object.transform.position)},
                     {"rotation", vectorJson(object.transform.rotation)},
                     {"scale", vectorJson(object.transform.scale)}};
        item["material"] = {{"color", vectorJson(object.material.color)},
                            {"shininess", object.material.shininess},
                            {"mode", static_cast<int>(object.material.mode)},
                            {"texture", relativeAsset(object.material.texturePath, folder)}};
        if (object.kind == ObjectKind::Shape)
            item["shape"] = static_cast<int>(object.shape);
        else if (object.kind == ObjectKind::Surface)
            item["surface"] = {{"expression", object.surfaceExpression},
                               {"extent", object.surfaceExtent},
                               {"resolution", object.surfaceResolution}};
        else if (object.kind == ObjectKind::Imported)
            item["model"] = relativeAsset(object.modelPath, folder);
        else if (object.kind == ObjectKind::Atom)
            item["element"] = object.element;
        else
        {
            item["atoms"] = Json::array();
            item["bonds"] = Json::array();
            for (const auto &atom : object.atoms)
                item["atoms"].push_back({{"element", atom.element}, {"position", vectorJson(atom.position)}});
            for (const auto &bond : object.bonds)
                item["bonds"].push_back({bond.first, bond.second});
        }
        data["objects"].push_back(std::move(item));
    }
    // Write a sibling temporary file, then replace only after a complete successful write.
    auto temporary = path;
    temporary += "." + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".tmp";
    try
    {
        std::ofstream output(temporary, std::ios::binary);
        if (!output)
            throw std::runtime_error("Cannot create scene file.");
        output << data.dump(2) << '\n';
        output.close();
        if (!output)
            throw std::runtime_error("Scene write failed.");
#ifdef _WIN32
        if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            throw std::runtime_error("Cannot replace scene file (it may be locked).");
#else
        std::filesystem::rename(temporary, path);
#endif
    }
    catch (...)
    {
        std::error_code error;
        std::filesystem::remove(temporary, error);
        throw;
    }
}

SceneDocument loadScene(const std::filesystem::path &path)
{
    if (std::filesystem::file_size(path) > 8 * 1024 * 1024)
        throw std::runtime_error("Scene JSON exceeds 8 MB.");
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("Cannot open scene file.");
    Json data = Json::parse(input);
    if (data.at("format") != "CGAssignment1" || integer(data.at("version"), 1, 1) != 1)
        throw std::runtime_error("Unsupported scene format/version.");
    SceneDocument result;
    auto folder = std::filesystem::absolute(path).parent_path();
    auto &camera = result.camera;
    const auto &savedCamera = data.at("camera");
    camera.target = vectorValue(savedCamera.at("target"));
    camera.yaw = number(savedCamera.at("yaw"), -1000000, 1000000);
    camera.pitch = number(savedCamera.at("pitch"), -89, 89);
    camera.distance = number(savedCamera.at("distance"), 0.3f, 100000);
    camera.moveSpeed = number(savedCamera.at("moveSpeed"), 0.5f, 20);
    camera.frontView = savedCamera.at("frontView").get<bool>();
    result.background = vectorValue(data.at("background"), 0, 1);
    result.lightPosition = vectorValue(data.at("lightPosition"));
    result.showGrid = data.at("showGrid").get<bool>();
    const auto &animation = data.at("animation");
    result.animation.playing = animation.at("playing").get<bool>();
    result.animation.speed = number(animation.at("speed"), 0, 5);
    result.animation.time = number(animation.at("time"), 0, 100000);
    const auto &objects = data.at("objects");
    if (!objects.is_array() || objects.size() > 256)
        throw std::runtime_error("Scene must contain at most 256 objects.");
    int selection = integer(data.at("selectedIndex"), -1, static_cast<int>(objects.size()) - 1);
    for (const auto &item : objects)
    {
        Object object;
        object.name = item.at("name").get<std::string>();
        // Older milestone 0/1 scenes omit labels; Scene::add infers them from the model/type.
        if (item.contains("category_id"))
            object.categoryId = integer(item.at("category_id"), 1, 1000000);
        if (item.contains("category_name"))
        {
            object.categoryName = item.at("category_name").get<std::string>();
            if (object.categoryName.empty() || object.categoryName.size() > 128)
                throw std::runtime_error("Category name must contain 1 to 128 UTF-8 bytes.");
        }
        if (object.name.size() > 1024)
            throw std::runtime_error("Object name too long.");
        object.kind = static_cast<ObjectKind>(integer(item.at("kind"), 0, 4));
        object.visible = item.at("visible").get<bool>();
        object.randomizeTransform = item.value("randomize_transform", true);
        object.transform.position = vectorValue(item.at("position"));
        object.transform.rotation = vectorValue(item.at("rotation"));
        object.transform.scale = vectorValue(item.at("scale"), 0.02f, 20);
        const auto &material = item.at("material");
        object.material.color = vectorValue(material.at("color"), 0, 1);
        object.material.shininess = number(material.at("shininess"), 2, 128);
        object.material.mode = static_cast<RenderMode>(integer(material.at("mode"), 0, 4));
        object.material.texturePath = resolveAsset(material.at("texture"), folder);
        if (!object.material.texturePath.empty())
            object.material.texture =
                std::make_shared<Texture>(std::filesystem::u8path(object.material.texturePath));
        if (object.kind == ObjectKind::Shape)
        {
            object.shape = static_cast<Shape>(integer(item.at("shape"), 0, 16));
            object.mesh = std::make_shared<Mesh>(makeShape(object.shape));
        }
        else if (object.kind == ObjectKind::Surface)
        {
            const auto &surface = item.at("surface");
            object.surfaceExpression = surface.at("expression").get<std::string>();
            object.surfaceExtent = number(surface.at("extent"), 0.1f, 10);
            object.surfaceResolution = integer(surface.at("resolution"), 4, 160);
            object.mesh = std::make_shared<Mesh>(
                makeSurface(object.surfaceExpression, object.surfaceExtent, object.surfaceResolution));
        }
        else if (object.kind == ObjectKind::Imported)
        {
            object.modelPath = resolveAsset(item.at("model"), folder);
            object.mesh = std::make_shared<Mesh>(loadObj(object.modelPath));
        }
        else if (object.kind == ObjectKind::Atom)
            object.element = integer(item.at("element"), 0, elementCount - 1);
        else
        {
            const auto &atoms = item.at("atoms"), &bonds = item.at("bonds");
            if (!atoms.is_array() || atoms.empty() || atoms.size() > 128 || !bonds.is_array() ||
                bonds.size() > 256)
                throw std::runtime_error("Invalid molecule size.");
            for (const auto &atom : atoms)
                object.atoms.push_back(
                    {integer(atom.at("element"), 0, elementCount - 1), vectorValue(atom.at("position"))});
            for (const auto &bond : bonds)
            {
                if (!bond.is_array() || bond.size() != 2)
                    throw std::runtime_error("Invalid bond.");
                object.bonds.push_back({integer(bond[0], 0, static_cast<int>(atoms.size()) - 1),
                                        integer(bond[1], 0, static_cast<int>(atoms.size()) - 1)});
            }
        }
        int nextBefore = result.scene.nextId;
        result.scene.add(std::move(object));
        if (item.contains("instance_id"))
        {
            int savedId = integer(item.at("instance_id"), 1, 0xFFFFFF);
            result.scene.objects.back().id = savedId;
            result.scene.nextId = std::max(nextBefore, savedId + 1);
        }
    }
    std::set<int> ids;
    for (const auto &object : result.scene.objects)
        if (!ids.insert(object.id).second) throw std::runtime_error("Duplicate saved instance ID.");
    if (data.contains("next_instance_id"))
        result.scene.nextId = std::max(result.scene.nextId, integer(data.at("next_instance_id"), 1, 0x1000000));
    result.scene.selected = selection < 0 ? -1 : result.scene.objects[selection].id;
    return result;
}
