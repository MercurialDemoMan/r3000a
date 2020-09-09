#pragma once

#if defined  __WIN__
#include <GL/glew.h>
#include <SDL/SDL.h>
#elif defined __OSX__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <SDL2/SDL.h>
#elif defined __LNX__
#include <OpenGL/glew.h>
#include <SDL2/SDL.h>
#endif

#include <string>
#include <cstdlib>

#include "glm/glm.hpp"
#include "Types.hpp"

/**
 * \brief compile and use glsl program
 */
class ShaderProgram
{
public:
    
    /**
     * \brief constructor/destructor
     */
    ShaderProgram() {}
   ~ShaderProgram();
    
    /**
     * \brief use/unuse fragment/vertex shader program
     */
    void use()   { glUseProgram(m_program); }
    void unuse() { glUseProgram(0);         }
    
    /**
     * \brief compile glsl program
     */
    void init(const char* vertex_source, const char* fragment_source);
    u32 compile(const char* source, u32 mode);
    
    /**
     * \brief set uniform
     */
    void set1i(int         value, const char* uniform_name);
    void set1f(float       value, const char* uniform_name);
    void set2f(glm::vec2   value, const char* uniform_name);
	void set2i(glm::ivec2  value, const char* uniform_name);
    void set3f(glm::vec3   value, const char* uniform_name);
    void set4f(glm::vec4   value, const char* uniform_name);
    void set1b(bool        value, const char* uniform_name);
    void set4x4m(glm::mat4 value, const char* uniform_name);
    void set1fv(float*     array, u32 size, const char* uniform_name);
    void set2fv(glm::vec2* array, u32 size, const char* uniform_name);
    void set3fv(glm::vec3* array, u32 size, const char* uniform_name);
    void set1iv(int*       array, u32 size, const char* uniform_name);
    
    /**
     * \brief get uniform index
     */
    GLuint get_uniform_index(const char* uniform_name);
	
	GLuint get_attribute_index(const char* attribute_name);
    
private:
    u32 m_vertex_shader, m_fragment_shader, m_program;
    
};

