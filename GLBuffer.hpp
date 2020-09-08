#pragma once

#if defined  __WIN__
#define NOMINMAX
#include <Windows.h>
#include <OpenGL/glew.h>
#include <SDL/SDL.h>
#elif defined __OSX__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl.h>
#include <SDL2/SDL.h>
#elif defined __LNX__
#include <OpenGL/glew.h>
#include <SDL2/SDL.h>
#endif


template<typename T>
class GLBuffer
{
public:
    
    static constexpr u32 VertexBufferLength = 60 * 1024;
    
    GLBuffer() {}
    
    void init()
    {
        m_raw = new T[VertexBufferLength];
        
        glGenBuffers(1, &m_id);
        glBindBuffer(GL_ARRAY_BUFFER, m_id);
        
        memset(m_raw, 0, sizeof(T) * VertexBufferLength);

        upload();
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        
        m_init = true;
    }
    
    ~GLBuffer()
    {
        assert(m_init);
        glDeleteBuffers(1, &m_id);
        delete[] m_raw;
    }
    
    T& operator[](u32 index)
    {
        assert(index < VertexBufferLength);
        return m_raw[index];
    }
    
    void upload()
    {
        glBufferData(GL_ARRAY_BUFFER, sizeof(T) * VertexBufferLength, m_raw, GL_DYNAMIC_DRAW);
    }
    
    const GLuint id() const { return m_id; }
    T* data()         const { return m_raw; }
    
private:
    GLuint m_id   { 0 };
    T*     m_raw  { nullptr };
    bool   m_init { false };
};
