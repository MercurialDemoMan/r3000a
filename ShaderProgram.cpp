#include "ShaderProgram.hpp"


//destructor
ShaderProgram::~ShaderProgram()
{
    glDetachShader(m_program, m_vertex_shader);
    glDetachShader(m_program, m_fragment_shader);
    glDeleteShader(m_vertex_shader);
    glDeleteShader(m_fragment_shader);
    glDeleteProgram(m_program);
}

void ShaderProgram::init(const char* vertex_source, const char* fragment_source)
{
    //compile vertex shader
    m_vertex_shader = compile(vertex_source, GL_VERTEX_SHADER);
    //compile vertex shader
    m_fragment_shader = compile(fragment_source, GL_FRAGMENT_SHADER);
    
    //create program
    m_program = glCreateProgram();
    
    //link shaders
    glAttachShader(m_program, m_vertex_shader);
    glAttachShader(m_program, m_fragment_shader);
    
    glLinkProgram(m_program);
}

//compile GLSL shader source
u32 ShaderProgram::compile(const char* source, u32 mode)
{
    //create shader
    u32 id = glCreateShader(mode);
    
    //compile shader
    glShaderSource (id, 1, &source, NULL);
    glCompileShader(id);
    
    //error checking
    char* error = (char*)calloc(256, 1);
    
    glGetShaderInfoLog(id, 256, NULL, error);
    
    if(error[0] != 0)
    {
        std::printf("ShaderProgram::compile() error: compilation error: %s\n", error);
    }
    else
    {
        std::printf("ShaderProgram::compile() log: compilation successful\n");
    }
    
    free(error);
    
    return id;
}

//setting uniform variables in shader program
void ShaderProgram::set1i(int value, const char* uniform_name) {
    glUniform1i(glGetUniformLocation(m_program, uniform_name), value);
}
void ShaderProgram::set1f(float value, const char* uniform_name) {
    glUniform1f(glGetUniformLocation(m_program, uniform_name), value);
}
void ShaderProgram::set2f(glm::vec2 value, const char* uniform_name) {
    glUniform2f(glGetUniformLocation(m_program, uniform_name), value.x, value.y);
}
void ShaderProgram::set2i(glm::ivec2 value, const char* uniform_name)
{
	glUniform2i(glGetUniformLocation(m_program, uniform_name), value.x, value.y);
}
void ShaderProgram::set3f(glm::vec3 value, const char* uniform_name) {
    glUniform3f(glGetUniformLocation(m_program, uniform_name), value.x, value.y, value.z);
}
void ShaderProgram::set4f(glm::vec4 value, const char* uniform_name) {
    glUniform4f(glGetUniformLocation(m_program, uniform_name), value.x, value.y, value.z, value.w);
}
void ShaderProgram::set1b(bool value, const char* uniform_name) {
    glUniform1i(glGetUniformLocation(m_program, uniform_name), value);
}
void ShaderProgram::set4x4m(glm::mat4 value, const char* uniform_name) {
    glUniformMatrix4fv(glGetUniformLocation(m_program, uniform_name), 1, false, &value[0][0]);
}
void ShaderProgram::set1fv(float* array, u32 size, const char* uniform_name) {
    glUniform1fv(glGetUniformLocation(m_program, uniform_name), size, array);
}
void ShaderProgram::set2fv(glm::vec2* array, u32 size, const char* uniform_name) {
    glUniform2fv(glGetUniformLocation(m_program, uniform_name), size, (float*)array);
}
void ShaderProgram::set3fv(glm::vec3* array, u32 size, const char* uniform_name) {
    glUniform3fv(glGetUniformLocation(m_program, uniform_name), size, (float*)array);
}
void ShaderProgram::set1iv(int* array, u32 size, const char* uniform_name) {
    glUniform1iv(glGetUniformLocation(m_program, uniform_name), size, array);
}

GLuint ShaderProgram::get_uniform_index(const char* uniform_name)
{
    return glGetUniformLocation(m_program, uniform_name);
}

GLuint ShaderProgram::get_attribute_index(const char* attribute_name)
{
	return glGetAttribLocation(m_program, attribute_name);
}