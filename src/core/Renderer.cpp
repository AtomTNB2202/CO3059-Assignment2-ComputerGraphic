#include "Renderer.h"
#include "Molecule.h"
#include "Depth.h"
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <stdexcept>

namespace
{
MeshData thinRing()
{
    auto mesh = makeShape(Shape::Torus);
    for (auto &vertex : mesh.vertices)
    {
        glm::vec3 center = vertex.position - 0.25f * vertex.normal;
        vertex.position = center + 0.012f * vertex.normal;
    }
    return mesh;
}
} // namespace

Renderer::Renderer(const std::filesystem::path &root)
    : basic(root / "shaders/basic.vert", root / "shaders/basic.frag"),
      gouraud(root / "shaders/gouraud.vert", root / "shaders/gouraud.frag"),
      outline(root / "shaders/outline.vert", root / "shaders/outline.frag"), sphere(makeShape(Shape::Sphere)),
      cylinder(makeShape(Shape::Cylinder)), ring(thinRing()), grid(root),
      fallback(std::make_shared<Texture>(root / "assets/textures/checker.png"))
{
    glGenFramebuffers(1, &pickFramebuffer);
    glGenTextures(1, &pickTexture);
    glGenRenderbuffers(1, &pickDepth);
    glGenVertexArrays(1, &outlineVao);
}

void Renderer::draw(const Mesh &mesh, const glm::mat4 &model, const Material &material, const Camera &camera,
                    float aspect)
{
    auto &shader = !pickingId && material.mode == RenderMode::Gouraud ? gouraud : basic;
    shader.use();
    shader.set("model", model);
    shader.set("view", camera.view());
    shader.set("projection", camera.projection(aspect));
    glm::vec3 color =
        pickingId ? glm::vec3(pickingId & 255, (pickingId >> 8) & 255, (pickingId >> 16) & 255) / 255.0f
                  : material.color;
    shader.set("objectColor", color);
    shader.set("cameraPosition", camera.position());
    shader.set("lightPosition", lightPosition);
    shader.set("shininess", material.shininess);
    shader.set("renderMode", pickingId ? 0 : static_cast<int>(material.mode));
    shader.set("imageTexture", 0);
    (material.texture ? material.texture : fallback)->bind();
    glPolygonMode(GL_FRONT_AND_BACK,
                  !pickingId && material.mode == RenderMode::Wireframe ? GL_LINE : GL_FILL);
    mesh.draw();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void Renderer::renderScene(const Scene &scene, const Camera &camera, int x, int y, int width, int height,
                           bool picking)
{
    glEnable(GL_SCISSOR_TEST);
    glScissor(x, y, width, height);
    glViewport(x, y, width, height);
    auto clear = picking ? glm::vec3(0) : background;
    glClearColor(clear.r, clear.g, clear.b, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    float aspect = float(width) / height;
    for (const auto &object : scene.objects)
    {
        if (!object.visible)
            continue;
        pickingId = picking ? object.id : 0;
        auto parent = object.transform.matrix();
        if (object.mesh)
        {
            draw(*object.mesh, parent, object.material, camera, aspect);
            continue;
        }
        auto drawSphere = [&](glm::vec3 position, float radius, glm::vec3 color)
        {
            auto material = object.material;
            material.color = color;
            draw(sphere,
                 parent * glm::translate(glm::mat4(1), position) *
                     glm::scale(glm::mat4(1), glm::vec3(radius)),
                 material, camera, aspect);
        };
        if (object.kind == ObjectKind::Atom)
        {
            const auto &element = elements[object.element];
            drawSphere({0, 0, 0}, 0.35f, element.color);
            int remaining = element.electrons;
            for (int shell = 0; remaining > 0; ++shell)
            {
                int count = std::min(remaining, shell == 0 ? 2 : 8);
                float radius = 0.9f + shell * 0.65f;
                Material orbit = object.material;
                orbit.color = {0.25f, 0.43f, 0.6f};
                // A thin torus in the XY plane marks each shell.
                glm::mat4 orbitModel = glm::rotate(glm::mat4(1), glm::radians(90.0f), {1, 0, 0});
                orbitModel = glm::scale(orbitModel, glm::vec3(radius / 0.75f));
                draw(ring, parent * orbitModel, orbit, camera, aspect);
                for (int electron = 0; electron < count; ++electron)
                {
                    float angle = 2 * glm::pi<float>() * electron / count + shell * 0.3f +
                                  static_cast<float>(animationTime / (shell + 1));
                    drawSphere({radius * std::cos(angle), radius * std::sin(angle), 0}, 0.105f,
                               {0.2f, 0.9f, 1.0f});
                }
                remaining -= count;
            }
        }
        else if (object.kind == ObjectKind::Molecule)
        {
            for (const auto &atom : object.atoms)
                drawSphere(atom.position, elements[atom.element].radius, elements[atom.element].color);
            for (const auto &bond : object.bonds)
            {
                auto first = object.atoms[bond.first].position, second = object.atoms[bond.second].position;
                if (glm::length(second - first) < 1e-5f)
                    continue;
                Material material = object.material;
                material.color = {0.55f, 0.62f, 0.72f};
                draw(cylinder, parent * bondMatrix(first, second, 0.1f), material, camera, aspect);
            }
        }
    }
    pickingId = 0;
    if (showGrid && !picking)
        grid.draw(camera, aspect);
    glBindVertexArray(0);
    glUseProgram(0);
    glDisable(GL_SCISSOR_TEST);
}

Renderer::~Renderer()
{
    glDeleteFramebuffers(1, &pickFramebuffer);
    glDeleteTextures(1, &pickTexture);
    glDeleteRenderbuffers(1, &pickDepth);
    glDeleteVertexArrays(1, &outlineVao);
}

void Renderer::resizePickTarget(int width, int height)
{
    if (width == pickWidth && height == pickHeight)
        return;
    pickWidth = width;
    pickHeight = height;
    glBindFramebuffer(GL_FRAMEBUFFER, pickFramebuffer);
    glBindTexture(GL_TEXTURE_2D, pickTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pickTexture, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, pickDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, pickDepth);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("Cannot allocate selection framebuffer.");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

int Renderer::pick(int x, int y) const
{
    if (x < 0 || y < 0 || x >= pickWidth || y >= pickHeight)
        return -1;
    unsigned char pixel[4]{};
    glBindFramebuffer(GL_READ_FRAMEBUFFER, pickFramebuffer);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glReadBuffer(GL_BACK);
    int id = pixel[0] | (pixel[1] << 8) | (pixel[2] << 16);
    return id ? id : -1;
}

void Renderer::render(const Scene &scene, const Camera &camera, int x, int y, int width, int height)
{
    resizePickTarget(width, height);
    glDisable(GL_BLEND);
    glDisable(GL_DITHER);
    glBindFramebuffer(GL_FRAMEBUFFER, pickFramebuffer);
    renderScene(scene, camera, 0, 0, width, height, true);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    renderScene(scene, camera, x, y, width, height, false);
    if (scene.selected > 0)
    {
        glEnable(GL_SCISSOR_TEST);
        glScissor(x, y, width, height);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        outline.use();
        outline.set("objectIds", 0);
        outline.set("selectedId", scene.selected);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, pickTexture);
        glBindVertexArray(outlineVao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
        glUseProgram(0);
        glDisable(GL_BLEND);
        glDisable(GL_SCISSOR_TEST);
        glEnable(GL_DEPTH_TEST);
    }
    glEnable(GL_DITHER);
}

DatasetFrame Renderer::captureDataset(const Scene &scene, const Camera &camera, int width, int height)
{
    if (width < 1 || height < 1 || width > 4096 || height > 4096)
        throw std::runtime_error("Dataset dimensions must be between 1 and 4096.");
    for (const auto &object : scene.objects)
        if (object.id < 1 || object.id > 0xFFFFFF)
            throw std::runtime_error("Instance IDs must fit in 24 bits; zero is background.");

    // Restore editor state even if framebuffer allocation fails.
    struct State
    {
        GLint draw, read, readBuffer, viewport[4], scissor[4], pack, depthFunc;
        GLfloat clear[4];
        GLdouble clearDepth, depthRange[2];
        GLboolean depthMask, colorMask[4];
        const GLenum caps[7] = {GL_BLEND, GL_DITHER, GL_MULTISAMPLE, GL_FRAMEBUFFER_SRGB,
                                GL_DEPTH_TEST, GL_SCISSOR_TEST, GL_CULL_FACE};
        GLboolean enabled[7];
        bool &grid, oldGrid;
        explicit State(bool &grid) : grid(grid), oldGrid(grid)
        {
            glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw);
            glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read);
            glGetIntegerv(GL_READ_BUFFER, &readBuffer);
            glGetIntegerv(GL_VIEWPORT, viewport);
            glGetIntegerv(GL_SCISSOR_BOX, scissor);
            glGetIntegerv(GL_PACK_ALIGNMENT, &pack);
            glGetIntegerv(GL_DEPTH_FUNC, &depthFunc);
            glGetFloatv(GL_COLOR_CLEAR_VALUE, clear);
            glGetDoublev(GL_DEPTH_CLEAR_VALUE, &clearDepth);
            glGetDoublev(GL_DEPTH_RANGE, depthRange);
            glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
            glGetBooleanv(GL_COLOR_WRITEMASK, colorMask);
            for (int i = 0; i < 7; ++i) enabled[i] = glIsEnabled(caps[i]);
        }
        ~State()
        {
            grid = oldGrid;
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, read);
            glReadBuffer(readBuffer);
            glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
            glScissor(scissor[0], scissor[1], scissor[2], scissor[3]);
            glPixelStorei(GL_PACK_ALIGNMENT, pack);
            glDepthFunc(depthFunc);
            glDepthMask(depthMask);
            glColorMask(colorMask[0], colorMask[1], colorMask[2], colorMask[3]);
            glClearColor(clear[0], clear[1], clear[2], clear[3]);
            glClearDepth(clearDepth);
            glDepthRange(depthRange[0], depthRange[1]);
            for (int i = 0; i < 7; ++i)
                if (enabled[i]) glEnable(caps[i]); else glDisable(caps[i]);
        }
    } state(showGrid);

    DatasetFrame frame{width, height, std::vector<unsigned char>(size_t(width) * height * 3),
                        std::vector<unsigned char>(size_t(width) * height * 3)};
    frame.depth.resize(size_t(width) * height);
    frame.nearPlane = camera.nearPlane();
    frame.farPlane = camera.farPlane();
    resizePickTarget(width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, pickFramebuffer);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glDisable(GL_BLEND);
    glDisable(GL_DITHER);
    glDisable(GL_MULTISAMPLE);
    glDisable(GL_FRAMEBUFFER_SRGB);
    glDepthFunc(GL_LESS);
    glClearDepth(1.0);
    glDepthRange(0.0, 1.0);
    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    showGrid = false;

    // A dataset represents surfaces. Fill wireframe objects in both passes.
    Scene surfaces = scene;
    for (auto &object : surfaces.objects)
        if (object.material.mode == RenderMode::Wireframe)
            object.material.mode = RenderMode::Flat;
    renderScene(surfaces, camera, 0, 0, width, height, false);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, frame.rgb.data());
    renderScene(surfaces, camera, 0, 0, width, height, true);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, frame.instanceIds.data());
    glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, frame.depth.data());

    // OpenGL starts at bottom-left; exported arrays and PNGs start at top-left.
    const size_t row = size_t(width) * 3;
    for (auto *pixels : {&frame.rgb, &frame.instanceIds})
        for (int y = 0; y < height / 2; ++y)
            std::swap_ranges(pixels->begin() + y * row, pixels->begin() + (y + 1) * row,
                             pixels->begin() + (height - 1 - y) * row);
    for (int y = 0; y < height / 2; ++y)
        std::swap_ranges(frame.depth.begin() + size_t(y) * width,
                         frame.depth.begin() + size_t(y + 1) * width,
                         frame.depth.begin() + size_t(height - 1 - y) * width);
    for (size_t p = 0; p < frame.depth.size(); ++p)
    {
        bool backgroundPixel = !frame.instanceIds[3*p] && !frame.instanceIds[3*p+1] && !frame.instanceIds[3*p+2];
        frame.depth[p] = backgroundPixel ? 0.0f : linearCameraDepth(frame.depth[p], frame.nearPlane, frame.farPlane);
    }
    return frame;
}
