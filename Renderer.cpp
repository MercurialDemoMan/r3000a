#include "Renderer.hpp"

void Renderer::init()
{
    SDL_Init(SDL_INIT_EVERYTHING);
    
    //SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    //SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    
    m_window = SDL_CreateWindow("Dumbstation", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, m_width, m_height, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
    
    m_gl_context = SDL_GL_CreateContext(m_window);
    SDL_GL_MakeCurrent(m_window, m_gl_context);
    
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    //enable vsync
    SDL_GL_SetSwapInterval(1);
    
#if defined __WIN__ || defined __LNX__
    //initialize GLEW
    if(glewInit() != GLEW_OK)
    {
        //msg_error(glewGetErrorString());
        return;
    }
#endif
    
    std::printf("OpenGL version: %s\n", glGetString(GL_VERSION));
    
    glClearColor(0, 0, 0, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    m_primitive_shader.init(m_primitive_shader_vert, m_primitive_shader_frag);
    
	m_primitive_shader.use();
	
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);
    
    m_vertices.init();
    
    s32 vert_idx = m_primitive_shader.get_attribute_index("vertex_position");
    glEnableVertexAttribArray(vert_idx);
    glVertexAttribIPointer(vert_idx, 2, GL_SHORT, 0, nullptr);
    
    m_colors.init();
    
    s32 col_idx = m_primitive_shader.get_attribute_index("vertex_color");
    glEnableVertexAttribArray(col_idx);
    glVertexAttribIPointer(col_idx, 3, GL_UNSIGNED_BYTE, 0, nullptr);
    
    glBindVertexArray(0);
}

void Renderer::clear()
{
    glDeleteVertexArrays(1, &m_vao);
    SDL_GL_DeleteContext(m_gl_context);
    SDL_DestroyWindow(m_window);
}

void Renderer::poll_events()
{
    static SDL_Event e;
    
    while(SDL_PollEvent(&e))
    {
        switch(e.type)
        {
            case SDL_QUIT: { assert(false); break; }
            default: { break; }
        }
    }
}

void Renderer::draw_shaded_triangle(Vertex (&vertices)[3], Color (&colors)[3])
{
	if(m_vertices_count + 3 >= VertexBufferLength)
	{
		draw();
	}
	
    m_vertices[m_vertices_count] = vertices[0];
    m_colors[m_vertices_count++] = colors[0];
    m_vertices[m_vertices_count] = vertices[1];
    m_colors[m_vertices_count++] = colors[1];
    m_vertices[m_vertices_count] = vertices[2];
    m_colors[m_vertices_count++] = colors[2];
}

void Renderer::draw_shaded_quad(Vertex (&vertices)[4], Color (&colors)[4])
{
	if(m_vertices_count + 6 >= VertexBufferLength)
	{
		draw();
	}
	
	m_vertices[m_vertices_count] = vertices[0];
    m_colors[m_vertices_count++] = colors[0];
    m_vertices[m_vertices_count] = vertices[1];
    m_colors[m_vertices_count++] = colors[1];
    m_vertices[m_vertices_count] = vertices[2];
    m_colors[m_vertices_count++] = colors[2];
	
	
	m_vertices[m_vertices_count] = vertices[1];
    m_colors[m_vertices_count++] = colors[1];
    m_vertices[m_vertices_count] = vertices[2];
    m_colors[m_vertices_count++] = colors[2];
    m_vertices[m_vertices_count] = vertices[3];
    m_colors[m_vertices_count++] = colors[3];
}

void Renderer::set_draw_offset(s16 x, s16 y)
{
	draw();
	
	m_primitive_shader.set2i(glm::vec2(x, y), "offset");
}

void Renderer::draw()
{
    poll_events();
	
	m_primitive_shader.use();
    glBindVertexArray(m_vao);
	//m_vertices.use();
	//m_colors.use();
	
    glDrawArrays(GL_TRIANGLES, 0, m_vertices_count);
    
	//m_colors.unuse();
	//m_vertices.unuse();
	glBindVertexArray(0);
	m_primitive_shader.unuse();
	    
    m_vertices_count = 0;
    
    SDL_GL_SwapWindow(m_window);
}
