cmake_minimum_required(VERSION 3.20)
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

project(ComputerGraphicsAssignment1 VERSION 1.0 LANGUAGES C CXX)

if(MSVC)
    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:/EHsc>")
endif()

include(FetchContent)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw URL https://github.com/glfw/glfw/archive/refs/tags/3.4.tar.gz
    URL_HASH SHA256=c038d34200234d071fae9345bc455e4a8f2f544ab60150765d7704e08f3dac01)
FetchContent_Declare(glm URL https://github.com/g-truc/glm/archive/refs/tags/1.0.1.tar.gz
    URL_HASH SHA256=9f3174561fd26904b23f0db5e560971cbf9b3cbda0b280f04d5c379d03bf234c)
FetchContent_MakeAvailable(glfw glm)

FetchContent_Declare(imgui URL https://github.com/ocornut/imgui/archive/refs/tags/v1.91.9b.tar.gz
    URL_HASH SHA256=8e1bbc76c71d74fef2fb85db7e7ca8eba13d6a86623c54992b60162db554ffdb)
FetchContent_Declare(stb URL https://github.com/nothings/stb/archive/2c980bb59875b0d32144a71867fbdebb2f77cd20.tar.gz
    URL_HASH SHA256=9a955b1b49a4410088a2e0ee2a9c057c3c907d0c1d75454144cb980aca0ba515)
FetchContent_Declare(tinyobjloader URL https://github.com/tinyobjloader/tinyobjloader/archive/refs/tags/v2.0.0rc13.tar.gz
    URL_HASH SHA256=0feb92b838f8ce4aa6eb0ccc32dff30cb64a891e0ec3bde837fca49c78d44334)
FetchContent_MakeAvailable(imgui stb tinyobjloader)
add_library(imgui_backend STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
target_include_directories(imgui_backend PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui_backend PUBLIC glfw)

FetchContent_Declare(imguizmo
    URL https://github.com/CedricGuillemet/ImGuizmo/archive/18cef5e031d8c6973d80284c67f60549fafd78c1.tar.gz
    URL_HASH SHA256=6ad626f0687be12c2f3ba6542c0f1bdda9e71e395d0645e4cda37695354406d8
    SOURCE_SUBDIR unused)
FetchContent_Declare(json URL https://github.com/nlohmann/json/archive/refs/tags/v3.11.3.tar.gz
    URL_HASH SHA256=0d8ef5af7f9794e3263480193c491549b2ba6cc74bb018906202ada498a79406)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(imguizmo json)
add_library(gizmo STATIC ${imguizmo_SOURCE_DIR}/src/ImGuizmo.cpp)
target_include_directories(gizmo PUBLIC ${imguizmo_SOURCE_DIR}/src)
target_link_libraries(gizmo PUBLIC imgui_backend)

add_library(geometry STATIC src/shapes/Geometry.cpp src/shapes/Expression.cpp src/graphics/ModelImport.cpp)
target_include_directories(geometry PUBLIC include)
target_link_libraries(geometry PUBLIC glm::glm PRIVATE tinyobjloader)
target_compile_features(geometry PUBLIC cxx_std_17)

add_library(glad STATIC third_party/glad/src/gl.c)
target_include_directories(glad PUBLIC third_party/glad/include)
target_compile_definitions(glad PUBLIC GLFW_INCLUDE_NONE)

add_executable(ComputerGraphicsAssignment1 src/main.cpp
    src/scene/Annotation.cpp src/scene/Depth.cpp src/scene/Batch.cpp src/core/DatasetBatch.cpp
    src/core/Application.cpp src/core/Camera.cpp src/core/Renderer.cpp src/core/Shader.cpp
    src/graphics/Mesh.cpp src/graphics/Texture.cpp src/graphics/Grid.cpp src/scene/Scene.cpp
    src/molecule/Molecule.cpp src/gui/GuiLayer.cpp src/gui/FileDialog.cpp src/gui/Editor.cpp
    src/scene/Bounds.cpp src/scene/SceneIO.cpp src/core/EditorChecks.cpp src/gui/ExportUi.cpp src/scene/Preview.cpp src/scene/DatasetView.cpp src/gui/DatasetUi.cpp)
target_include_directories(ComputerGraphicsAssignment1 PRIVATE include ${stb_SOURCE_DIR})
target_link_libraries(ComputerGraphicsAssignment1 PRIVATE geometry glfw glad glm::glm imgui_backend gizmo nlohmann_json::nlohmann_json)
if(WIN32)
    target_link_libraries(ComputerGraphicsAssignment1 PRIVATE comdlg32 ole32 shell32 uuid)
endif()
target_compile_definitions(ComputerGraphicsAssignment1 PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
if(MSVC)
    target_compile_options(ComputerGraphicsAssignment1 PRIVATE /utf-8)
    target_compile_options(geometry PRIVATE /utf-8)
endif()

# Resource paths are relative to the executable, so launching from any directory works.
add_custom_target(copy_resources ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:ComputerGraphicsAssignment1>/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:ComputerGraphicsAssignment1>/assets)
add_dependencies(ComputerGraphicsAssignment1 copy_resources)

include(CTest)
if(BUILD_TESTING)
    add_executable(annotation_tests tests/AnnotationTests.cpp src/scene/Annotation.cpp)
    target_include_directories(annotation_tests PRIVATE include)
    target_link_libraries(annotation_tests PRIVATE nlohmann_json::nlohmann_json)
    target_compile_features(annotation_tests PRIVATE cxx_std_17)
    add_test(NAME annotations COMMAND annotation_tests)
    add_executable(geometry_tests tests/GeometryTests.cpp src/core/Camera.cpp src/molecule/Molecule.cpp src/scene/Scene.cpp src/scene/Bounds.cpp)
    target_link_libraries(geometry_tests PRIVATE geometry glad)
    add_test(NAME geometry COMMAND geometry_tests assets/models/pyramid.obj)
    set_tests_properties(geometry PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    add_test(NAME graphics_smoke COMMAND ComputerGraphicsAssignment1 --smoke-test)
    set_tests_properties(graphics_smoke PROPERTIES LABELS "graphics" TIMEOUT 60)
endif()

set_target_properties(ComputerGraphicsAssignment1 PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
