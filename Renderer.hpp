#pragma once

#include "Types.hpp"
#include "GLBuffer.hpp"
#include "ShaderProgram.hpp"

#if defined  __WIN__
#define NOMINMAX
#include <Windows.h>
#include <GL/glew.h>
#include <SDL/SDL.h>
#elif defined __OSX__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl.h>
#include <SDL2/SDL.h>
#define glGenVertexArrays glGenVertexArraysAPPLE
#define glBindVertexArray glBindVertexArrayAPPLE
#define glDeleteVertexArrays glDeleteVertexArraysAPPLE
#elif defined __LNX__
#include <OpenGL/glew.h>
#include <SDL2/SDL.h>
#endif

#include <cstdio>

class Renderer
{
public:
    
    struct Vertex
    {
        Vertex() {}
        Vertex(s16 x, s16 y) : x(x), y(y) {}
        Vertex(u32 packed)
        {
            x = (packed >>  0) & 0xFFFF;
            y = (packed >> 16) & 0xFFFF;
        }
        
        s16 x { 0 };
        s16 y { 0 };
    };
    
    struct Color
    {
        Color() {}
        Color(u8 r, u8 g, u8 b) : r(r), g(g), b(b) {}
        Color(u32 packed)
        {
            r = (packed >>  0) & 0xFF;
            g = (packed >>  8) & 0xFF;
            b = (packed >> 16) & 0xFF;
        }
        
        u8 r { 0 };
        u8 g { 0 };
        u8 b { 0 };
    };
    
    Renderer() { init(); }
    Renderer(u16 width, u16 height) : m_width(width), m_height(height) { init(); }
    ~Renderer() { clear(); }
    
    
    void init();
    void clear();
    void poll_events();
    
    void draw_shaded_triangle(Vertex (&vertices)[3], Color (&colors)[3]);
    
    void draw();
    
private:
    
    u16 m_width  { 1024 };
    u16 m_height {  512 };
    
    SDL_Window*   m_window { nullptr };
    SDL_GLContext m_gl_context;
    
    GLuint m_vao { 0 };
    u32 m_vertices_count { 0 };
    
    GLBuffer<Vertex> m_vertices;
    GLBuffer<Color>  m_colors;
    
    ShaderProgram m_primitive_shader;
    
    static constexpr const char* m_primitive_shader_frag =
    R"(
	#version 120
	
    varying vec3 color;
    
    void main()
    {
        gl_FragColor = vec4(color, 1.0);
    }
    )";
    
    static constexpr const char* m_primitive_shader_vert =
    R"(
	#version 120
	
    attribute vec2 vertex_position;
    attribute vec3 vertex_color;
    
    varying vec3 color;
    
    void main()
    {
        float xpos = (float(vertex_position.x) / 512.0) - 1.0;
        float ypos = 1.0 - (float(vertex_position.y) / 256.0);
        
        gl_Position = vec4(xpos, ypos, 0.0, 1.0);
        
        color = vec3(vertex_color) / 255.0;
    }
    )";
};
