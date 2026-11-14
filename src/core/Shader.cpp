#include "Shader.h"
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace
{
GLuint compile(GLenum type, const std::filesystem::path &path)
{
    std::ifstream file(path);
    if (!file)
        throw std::runtime_error("Cannot open shader: " + path.u8string());
    std::ostringstream stream;
    stream << file.rdbuf();
    std::string source = stream.str();
    const char *text = source.c_str();
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &text, nullptr);
    glCompileShader(shader);
    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        GLint length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> log(length + 1);
        glGetShaderInfoLog(shader, length, nullptr, log.data());
        glDeleteShader(shader);
        throw std::runtime_error(path.u8string() + ": " + log.data());
    }
    return shader;
}
} // namespace
Shader::Shader(const std::filesystem::path &vertexPath, const std::filesystem::path &fragmentPath)
{
    GLuint vertex = compile(GL_VERTEX_SHADER, vertexPath), fragment = 0;
    try
    {
        fragment = compile(GL_FRAGMENT_SHADER, fragmentPath);
    }
    catch (...)
    {
        glDeleteShader(vertex);
        throw;
    }
    program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        GLint length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> log(length + 1);
        glGetProgramInfoLog(program, length, nullptr, log.data());
        glDeleteProgram(program);
        program = 0;
        throw std::runtime_error(std::string("Shader link failed: ") + log.data());
    }
}
Shader::~Shader()
{
    if (program)
        glDeleteProgram(program);
}
void Shader::use() const
{
    glUseProgram(program);
}
void Shader::set(const char *name, int value) const
{
    glUniform1i(glGetUniformLocation(program, name), value);
}
void Shader::set(const char *name, float value) const
{
    glUniform1f(glGetUniformLocation(program, name), value);
}
void Shader::set(const char *name, const glm::vec3 &value) const
{
    glUniform3fv(glGetUniformLocation(program, name), 1, glm::value_ptr(value));
}
void Shader::set(const char *name, const glm::mat4 &value) const
{
    glUniformMatrix4fv(glGetUniformLocation(program, name), 1, GL_FALSE, glm::value_ptr(value));
}
