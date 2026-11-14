#include "Geometry.h"
#include "Expression.h"
#include "Camera.h"
#include "Molecule.h"
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <functional>
#include <filesystem>
#include <fstream>

namespace
{
void require(bool condition, const char *message)
{
    if (!condition)
        throw std::runtime_error(message);
}
void rejects(const std::function<void()> &action)
{
    bool rejected = false;
    try
    {
        action();
    }
    catch (const std::exception &)
    {
        rejected = true;
    }
    require(rejected, "Expected invalid input to be rejected.");
}
void near(double actual, double expected)
{
    require(std::abs(actual - expected) < 1e-4, "Numerical check failed.");
}
} // namespace

int main(int argc, char **argv)
{
    try
    {
        for (int shape = 0; shape < 17; ++shape)
        {
            auto mesh = makeShape(static_cast<Shape>(shape));
            validateMesh(mesh);
            for (size_t i = 0; i < mesh.indices.size(); i += 3)
            {
                auto a = mesh.vertices[mesh.indices[i]], b = mesh.vertices[mesh.indices[i + 1]],
                     c = mesh.vertices[mesh.indices[i + 2]];
                auto cross = glm::cross(b.position - a.position, c.position - a.position);
                require(glm::length(cross) > 1e-7f, "Degenerate triangle.");
                require(glm::dot(cross, a.normal + b.normal + c.normal) > 0,
                        "Triangle winding disagrees with normals.");
            }
            if (shape < 9)
            {
                require(mesh.indices.size() == (mesh.vertices.size() - 2) * 3,
                        "Polygon triangle count incorrect.");
                double polygonArea = 0, triangleArea = 0;
                for (size_t i = 0; i < mesh.vertices.size(); ++i)
                {
                    auto a = mesh.vertices[i].position,
                         b = mesh.vertices[(i + 1) % mesh.vertices.size()].position;
                    polygonArea += (a.x * b.y - a.y * b.x) / 2;
                }
                for (size_t i = 0; i < mesh.indices.size(); i += 3)
                {
                    auto a = mesh.vertices[mesh.indices[i]].position,
                         b = mesh.vertices[mesh.indices[i + 1]].position,
                         c = mesh.vertices[mesh.indices[i + 2]].position;
                    triangleArea += glm::cross(b - a, c - a).z / 2;
                }
                near(triangleArea, polygonArea);
            }
        }
        near(Expression("2+3*4").evaluate(0, 0), 14);
        near(Expression("2^3^2").evaluate(0, 0), 512);
        near(Expression("-2^2").evaluate(0, 0), -4);
        near(Expression("2^-2").evaluate(0, 0), 0.25);
        near(Expression("sin(pi/2)+cos(0)+sqrt(4)+abs(-2)+log(e)").evaluate(0, 0), 7);
        near(Expression("x^2+y^2").evaluate(2, 3), 13);
        rejects([] { Expression("sin("); });
        rejects([] { Expression("unknown(x)"); });
        rejects([] { Expression("x;y"); });
        rejects([] { Expression("x y"); });
        rejects([] { Expression("1/0").evaluate(0, 0); });
        rejects([] { Expression("sqrt(-1)").evaluate(0, 0); });
        rejects([] { Expression("exp(1000)").evaluate(0, 0); });
        rejects([] { makeSurface("x", 0, 10); });
        rejects([] { makeSurface("x", 2, 10000); });
        auto surface = makeSurface("sin(x)*cos(y)", 2, 32);
        validateMesh(surface);
        require(surface.vertices.size() == 33 * 33 && surface.indices.size() == 32 * 32 * 6,
                "Surface grid dimensions incorrect.");
        auto plane = makeSurface("2*x+3*y", 2, 8);
        auto expected = glm::normalize(glm::vec3(-2, -3, 1));
        for (auto &v : plane.vertices)
            require(glm::length(v.normal - expected) < 1e-4, "Surface normal incorrect.");
        require(argc == 2, "Sample OBJ path missing.");
        auto imported = loadObj(argv[1]);
        validateMesh(imported);
        require(imported.indices.size() == 18, "Pyramid should have six triangles.");
        rejects([] { loadObj("missing-file-should-not-exist.obj"); });
        auto invalid = std::filesystem::temp_directory_path() / "cg-assignment-invalid-test.obj";
        {
            std::ofstream file(invalid);
            file << "v 0 0 0\nf 99 100 101\n";
        }
        rejects([&] { loadObj(invalid.u8string()); });
        std::filesystem::remove(invalid);
        for (auto end : {glm::vec3(1, 2, 3), glm::vec3(0, -2, 0), glm::vec3(0, 2, 0)})
        {
            glm::vec3 start(0);
            auto model = bondMatrix(start, end, 0.1f);
            require(glm::length(glm::vec3(model * glm::vec4(0, -1, 0, 1)) - start) < 1e-4,
                    "Bond start not aligned.");
            require(glm::length(glm::vec3(model * glm::vec4(0, 1, 0, 1)) - end) < 1e-4,
                    "Bond end not aligned.");
        }
        Camera camera;
        camera.front();
        near(camera.position().z, 7);
        camera.orbit(0, 100000);
        near(camera.pitch, 89);
        camera.zoom(100000);
        near(camera.distance, 0.3);
        camera.zoom(-100000);
        near(camera.distance, 100000);
        Transform transform;
        transform.position = {1, 2, 3};
        transform.scale = {2, 3, 4};
        auto point = transform.matrix() * glm::vec4(1, 1, 1, 1);
        near(point.x, 3);
        near(point.y, 5);
        near(point.z, 7);
        // WASD movement is camera-relative and independent of frame rate.
        Camera slowFrames, fastFrames;
        slowFrames.front();
        fastFrames.front();
        for (int i = 0; i < 30; ++i)
            slowFrames.move({0, 0, 1}, 1.0f / 30);
        for (int i = 0; i < 120; ++i)
            fastFrames.move({0, 0, 1}, 1.0f / 120);
        require(glm::length(slowFrames.target - fastFrames.target) < 1e-4,
                "Camera movement depends on frame rate.");
        near(slowFrames.target.z, -5);
        near(slowFrames.position().z, 2);
        near(slowFrames.distance, 7);
        Camera diagonal;
        diagonal.front();
        diagonal.move({1, 0, 1}, 0.1f);
        near(glm::length(diagonal.target), 0.5f);
        Camera rotated;
        rotated.yaw = 90;
        rotated.pitch = 0;
        rotated.move({0, 0, 1}, 0.1f);
        near(rotated.target.x, -0.5);
        rotated.move({0, 1, 0}, 0.1f);
        near(rotated.target.y, 0.5);

        Camera dragging;
        dragging.front();
        auto delta = dragging.screenDelta(80, -40, 7, 800);
        require(delta.x > 0 && delta.y > 0, "Drag direction is inverted.");
        near(delta.z, 0);
        require(glm::length(dragging.screenDelta(80, -40, 14, 800) - 2.0f * delta) < 1e-4,
                "Drag sensitivity must match the object's depth.");
        dragging.pan(80, -40, 800);
        require(glm::length(dragging.target + delta) < 1e-4, "Camera pan sign changed.");

        Scene scene;
        for (auto object : {Object{}, makeAtom(0), makeMolecule(true), makeMolecule(false)})
            scene.addAt(std::move(object), rotated.target);
        for (const auto &object : scene.objects)
        {
            auto clip = rotated.projection(1.2f) * rotated.view() * glm::vec4(object.transform.position, 1);
            near(clip.x / clip.w, 0);
            near(clip.y / clip.w, 0);
            require(clip.w > 0, "New object spawned behind the camera.");
        }
        auto original = scene.objects.front().transform.position;
        auto originalCamera = rotated.position();
        scene.translateSelected(delta);
        require(glm::length(scene.objects.front().transform.position - original) < 1e-4,
                "Dragging selected object moved a different object.");
        require(glm::length(scene.selection()->transform.position - rotated.target - delta) < 1e-4,
                "Selected object was not translated.");
        require(glm::length(rotated.position() - originalCamera) < 1e-4, "Object dragging moved the camera.");
        scene.selection()->visible = false;
        auto hiddenPosition = scene.selection()->transform.position;
        scene.translateSelected(delta);
        require(glm::length(scene.selection()->transform.position - hiddenPosition) < 1e-4,
                "Hidden object should not drag.");
        scene.selected = -1;
        scene.translateSelected(delta); // No selection must be a safe no-op.
        std::cout << "PASS: all shapes, winding, polygon area, formulas, surface normals, OBJ, camera, "
                     "transforms, bond endpoints, WASD, center spawning, selected-object dragging.\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
