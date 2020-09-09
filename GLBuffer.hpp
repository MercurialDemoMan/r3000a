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


template<typename T, u32 size>
class GLBuffer
{
public:
    
    GLBuffer() {}
    
    void init()
    {
		constexpr const u32 buffer_size = size * sizeof(T);
		
        glGenBuffers(1, &m_id);
        glBindBuffer(GL_ARRAY_BUFFER, m_id);
		glBufferStorage(GL_ARRAY_BUFFER, buffer_size, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
        
		m_raw = reinterpret_cast<T*>(glMapBufferRange(GL_ARRAY_BUFFER, 0, buffer_size, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT));
		
		memset(m_raw, 0, buffer_size);
		
		//glBindBuffer(GL_ARRAY_BUFFER, 0);
		
		m_inited = true;
    }

	void use()
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_id);
	}
	
	void unuse()
	{
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}	

    ~GLBuffer()
    {
		if(m_inited)
		{
			glBindBuffer(GL_ARRAY_BUFFER, m_id);
			glUnmapBuffer(GL_ARRAY_BUFFER);
			glDeleteBuffers(1, &m_id);
		}
    }
    
    T& operator[](u32 index)
    {
        assert(index < size);
        return m_raw[index];
    }
    
    const GLuint id() const { return m_id; }
    T* data()         const { return m_raw; }
    
private:
    GLuint m_id   { 0 };
    T*     m_raw  { nullptr };
	bool   m_inited { false };
};
