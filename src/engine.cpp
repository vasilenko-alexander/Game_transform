#include "../include/engine.hpp"
#include "../include/glew.h"
#include "../include/SDL.h"
#include "../include/SDL_opengl.h"
#include "../include/engine_constants.hpp"
#include "../include/image_loader.hpp"
#include "../include/shader_loader.hpp"
#include <algorithm>
#include <cassert>
#include <exception>
#include <iostream>
#include <fstream>
#include <map>
#include <sstream>
#include <vector>

#define GE_GL_CHECK()                                                          \
    {                                                                          \
        const int err = glGetError();                                          \
        if (err != GL_NO_ERROR)                                                \
        {                                                                      \
            switch (err)                                                       \
            {                                                                  \
                case GL_INVALID_ENUM:                                          \
                    std::cerr << "GL_INVALID_ENUM" << std::endl;               \
                    break;                                                     \
                case GL_INVALID_VALUE:                                         \
                    std::cerr << "GL_INVALID_VALUE" << std::endl;              \
                    break;                                                     \
                case GL_INVALID_OPERATION:                                     \
                    std::cerr << "GL_INVALID_OPERATION" << std::endl;          \
                    break;                                                     \
                case GL_INVALID_FRAMEBUFFER_OPERATION:                         \
                    std::cerr << "GL_INVALID_FRAMEBUFFER_OPERATION"            \
                              << std::endl;                                    \
                    break;                                                     \
                case GL_OUT_OF_MEMORY:                                         \
                    std::cerr << "GL_OUT_OF_MEMORY" << std::endl;              \
                    break;                                                     \
            }                                                                  \
            assert(false);                                                     \
        }                                                                      \
    }

namespace ge
{
    struct bind_event
    {
        bind_event(Uint32 _sdl_type, events_t _type, std::string _event_str)
            : sdl_type(_sdl_type), type(_type), event_str(_event_str)
        {
        }

        Uint32 sdl_type;
        events_t type;
        std::string event_str;
    };

    struct bind_key
    {
        bind_key(SDL_Keycode sdl_k, keys _key, std::string key_s)
            : sdl_key(sdl_k), key(_key), key_str(key_s)
        {
        }
        SDL_Keycode sdl_key;
        keys key;
        std::string key_str;
    };

    class Engine : public IEngine
    {
        SDL_Window* window      = nullptr;
        SDL_GLContext glContext = nullptr;
        ImageLoader img_loader;
        ShaderLoader shader_program;

        const std::map<std::string, uint> defined_options{
            { ge::timer, SDL_INIT_TIMER },
            { ge::audio, SDL_INIT_AUDIO },
            { ge::video, SDL_INIT_VIDEO },
            { ge::events, SDL_INIT_EVENTS },
            { ge::joystick, SDL_INIT_JOYSTICK },
            { ge::gamecontroller, SDL_INIT_GAMECONTROLLER },
            { ge::haptic, SDL_INIT_HAPTIC },
            { ge::everything, SDL_INIT_EVERYTHING }
        };

        const std::vector<bind_event> defined_events{
            bind_event{ SDL_QUIT, events_t::shutdown, "shutdown" },
            bind_event{ SDL_KEYUP, events_t::released, "released" },
            bind_event{ SDL_KEYDOWN, events_t::pressed, "pressed" }
        };

        const std::vector<bind_key> defined_keys{
            bind_key{ SDLK_UP, keys::up, "up" },
            bind_key{ SDLK_DOWN, keys::down, "down" },
            bind_key{ SDLK_LEFT, keys::left, "left" },
            bind_key{ SDLK_RIGHT, keys::right, "right" },
            bind_key{ SDLK_SPACE, keys::pause, "pause" },
            bind_key{ SDLK_ESCAPE, keys::select, "select" },
            bind_key{ SDLK_a, keys::button1, "button1" },
            bind_key{ SDLK_d, keys::button2, "button2" }
        };

        const std::string vertex_shader_path = "./config/VertexShader.glsl";
        const std::string frag_shader_path   = "./config/FragShader.glsl";

    public:
        Engine();
        std::string init_engine(std::string init_options) override;
        bool read_event(event& event) override;
        void uninit_engine() override;
        void render(triangle& tr) override;
        void render(texture& tx) override;
        void swap_buffers() override;
        float get_time() override;
        triangle transform_triangle(const triangle& trSrc,
                                    const triangle& trDest,
                                    float alpha) override;
        void draw_texture(const std::string& path) override;

    private:
        uint parse_wnd_options(std::string init_options);
        bind_key* check_input(SDL_Keycode check_code);
        bind_event* check_event(Uint32 check_event);
        void fill_background();
        vertex
        blend_vertex(const vertex& first, const vertex& second, float alpha);
    };

    Engine::Engine()
    {
    }

    float Engine::get_time()
    {
        Uint32 milisec = SDL_GetTicks();
        return milisec * 0.001f;
    }

    vertex
    Engine::blend_vertex(const vertex& first, const vertex& second, float alpha)
    {
        vertex r;
        r.x = (1.0f - alpha) * first.x + alpha * second.x;
        r.y = (1.0f - alpha) * first.y + alpha * second.y;
        return r;
    }

    triangle Engine::transform_triangle(const triangle& trSrc,
                                        const triangle& trDest,
                                        float alpha)
    {
        triangle trRes;
        for (size_t i = 0; i < trSrc.v.size(); ++i)
        {
            trRes.v[i] = blend_vertex(trSrc.v[i], trDest.v[i], alpha);
        }
        return trRes;
    }

    bind_event* Engine::check_event(Uint32 check_event)
    {
        const auto it = std::find_if(
            defined_events.begin(),
            defined_events.end(),
            [&](const bind_event& b_e) { return b_e.sdl_type == check_event; });

        if (it != defined_events.end())
        {
            return new bind_event{ it->sdl_type, it->type, it->event_str };
        }

        return nullptr;
    }

    bind_key* Engine::check_input(SDL_Keycode check_code)
    {
        const auto it = std::find_if(
            defined_keys.begin(), defined_keys.end(), [&](const bind_key& b_k) {
                return b_k.sdl_key == check_code;
            });

        if (it != defined_keys.end())
        {
            return new bind_key{ it->sdl_key, it->key, it->key_str };
        }

        return nullptr;
    }

    uint Engine::parse_wnd_options(std::string init_options)
    {
        uint flags = 0;
        try
        {

            char delim = ' ';
            std::istringstream strStream(init_options);
            std::string option;
            while (std::getline(strStream, option, delim))
            {
                std::transform(
                    option.begin(), option.end(), option.begin(), ::tolower);
                flags |= defined_options.at(option);
            }

            return flags;
        }
        catch (const std::out_of_range& ex)
        {
            std::cerr << "Some err is occurred: " << ex.what() << std::endl;
            return flags;
        }
    }

    void Engine::swap_buffers()
    {
        if (window != nullptr)
        {
            SDL_GL_SwapWindow(window);
            fill_background();
        }
    }

    std::string Engine::init_engine(std::string init_options)
    {
        SDL_version linked_lib = { 0, 0, 0 };
        SDL_GetVersion(&linked_lib);
        if (SDL_COMPILEDVERSION != SDL_VERSIONNUM(linked_lib.major,
                                                  linked_lib.minor,
                                                  linked_lib.patch))
        {
            std::cerr << "SDL compiled and linked libs are mismatch"
                      << std::endl;
        }

        std::stringstream errMsg;
        uint wndFlags      = parse_wnd_options(init_options);
        const int init_res = SDL_Init(wndFlags);

        if (init_res != 0)
        {
            errMsg << "SDL_INIT failed " << SDL_GetError() << std::endl;
            return errMsg.str();
        }

        const char* title_wnd = "SDL window";

        window = SDL_CreateWindow(title_wnd,
                                  SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED,
                                  640,
                                  480,
                                  ::SDL_WINDOW_OPENGL);

        if (window == nullptr)
        {
            errMsg << "Window creating failed " << SDL_GetError() << std::endl;
            uninit_engine();
            return errMsg.str();
        }

        glContext = SDL_GL_CreateContext(window);

        if (glContext == nullptr)
        {
            errMsg << "Create gl context failed " << SDL_GetError()
                   << std::endl;
            return errMsg.str();
        }

        int major_ver = 0;
        int result =
            SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major_ver);

        if (result != 0)
        {
            std::clog << "Can't retrieve opengl major version" << std::endl;
        }

        int minor_ver = 0;
        result = SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor_ver);

        if (result != 0)
        {
            std::clog << "Can't retrieve opengl minor version" << std::endl;
        }

        if (major_ver <= 2 && minor_ver < 1)
        {
            errMsg << "opengl version is " << major_ver << "." << minor_ver
                   << "You need version at least 2.1" << std::endl;
            uninit_engine();
            return errMsg.str();
        }

        GLenum res = glewInit();
        if (res != GLEW_OK)
        {
            errMsg << "glew init failed" << std::endl;
            return errMsg.str();
        }

        shader_program.init_program(vertex_shader_path, frag_shader_path);

        shader_program.use();

        fill_background();

        return errMsg.str();
    }

    bool Engine::read_event(event& e)
    {
        e             = event();
        bool hasEvent = false;
        std::string event_msg;
        SDL_Event sdl_event;

        if (SDL_PollEvent(&sdl_event))
        {
            hasEvent           = true;
            bind_event* bind_e = check_event(sdl_event.type);
            if (bind_e == nullptr)
            {
                return hasEvent;
            }

            bind_key* bind_k = nullptr;
            std::stringstream sstr;
            switch (sdl_event.type)
            {
                case SDL_QUIT:
                    sstr << bind_e->event_str;
                    e.msg  = sstr.str();
                    e.type = bind_e->type;
                    break;
                case SDL_KEYUP:
                case SDL_KEYDOWN:
                    bind_k = check_input(sdl_event.key.keysym.sym);
                    if (bind_k == nullptr)
                    {
                        return hasEvent;
                    }
                    sstr << bind_k->key_str << "_" << bind_e->event_str;
                    e.msg  = sstr.str();
                    e.type = bind_e->type;
                    e.key  = bind_k->key;
                    break;
                default:
                    break;
            }
        }

        return hasEvent;
    }

    void Engine::render(triangle& tr)
    {

        GLuint coordAttrID =
            glGetAttribLocation(shader_program.get_program_id(), "coords");

        GLuint vbo_name;
        // generate name for VBO
        glGenBuffers(1, &vbo_name);
        GE_GL_CHECK();
        // bind name
        glBindBuffer(GL_ARRAY_BUFFER, vbo_name);
        GE_GL_CHECK();
        // copy data to buffer under binding name
        glBufferData(GL_ARRAY_BUFFER, sizeof(tr.v), &tr.v[0], GL_STATIC_DRAW);
        GE_GL_CHECK();

        int coord_count = 2;
        glVertexAttribPointer(
            coordAttrID, coord_count, GL_FLOAT, GL_FALSE, 0, nullptr);
        GE_GL_CHECK();
        glEnableVertexAttribArray(coordAttrID);
        GE_GL_CHECK();
        glDrawArrays(GL_TRIANGLES, 0, 3);
        GE_GL_CHECK();
        // unbind VBO
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        GE_GL_CHECK();
    }

    void Engine::render(texture& tx)
    {
        int coord_count = 2;
        GLuint coord_id =
            glGetAttribLocation(shader_program.get_program_id(), "coords");
        GLuint tex_coord_id =
            glGetAttribLocation(shader_program.get_program_id(), "tex_coords");

        GLuint buffer;
        glGenBuffers(1, &buffer);
        GE_GL_CHECK()
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        GE_GL_CHECK()

        glBufferData(GL_ARRAY_BUFFER, sizeof(tx), nullptr, GL_STATIC_DRAW);
        GE_GL_CHECK()
#define BUFFER_OFFSET(x) ((char*)NULL + (x))

        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(tx.coords), &tx.coords[0]);
        GE_GL_CHECK()
        glBufferSubData(GL_ARRAY_BUFFER,
                        sizeof(tx.coords),
                        sizeof(tx.tex_coords),
                        &tx.tex_coords[0]);
        GE_GL_CHECK()

        glVertexAttribPointer(coord_id, coord_count, GL_FLOAT, GL_FALSE, 0, 0);
        GE_GL_CHECK()
        glEnableVertexAttribArray(coord_id);
        GE_GL_CHECK()

        glVertexAttribPointer(tex_coord_id,
                              coord_count,
                              GL_FLOAT,
                              GL_FALSE,
                              0,
                              BUFFER_OFFSET(sizeof(tx.coords)));
        GE_GL_CHECK()
        glEnableVertexAttribArray(tex_coord_id);
        GE_GL_CHECK()
#undef BUFFER_OFFSET
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    void Engine::fill_background()
    {
        glClearColor(0.22f, 0.22f, 0.22f, 0.f);
        GE_GL_CHECK();
        glClear(GL_COLOR_BUFFER_BIT);
        GE_GL_CHECK();
    }

    void Engine::uninit_engine()
    {
        shader_program.delete_program();
        if (window != nullptr)
        {
            SDL_DestroyWindow(window);
            window = nullptr;
        }
        if (glContext != nullptr)
        {
            SDL_GL_DeleteContext(glContext);
            glContext = nullptr;
        }
        SDL_Quit();
    }

    void Engine::draw_texture(const std::string& path)
    {
        unsigned long width  = 0;
        unsigned long height = 0;
        std::vector<unsigned char> text =
            img_loader.load_image(path, width, height);

        if (text.empty())
            return;

        // generate texture name
        GLuint texName;
        glGenTextures(1, &texName);
        GE_GL_CHECK();

        // tell which texture unit need using
        glActiveTexture(GL_TEXTURE0);

        // create empty texture object and bind it with name
        glBindTexture(GL_TEXTURE_2D, texName);
        GE_GL_CHECK();

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        // copy data to GPU texture object
        GLint mip_level = 0;
        GLint border    = 0;
        glTexImage2D(GL_TEXTURE_2D,
                     mip_level,
                     GL_RGBA,
                     width,
                     height,
                     border,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     &text.front());
        GE_GL_CHECK();

        // send texture unit to shader uniform
        GLint location =
            glGetUniformLocation(shader_program.get_program_id(), "s_texture");
        GE_GL_CHECK();
        int text_unit = 0;

        glUniform1i(location, text_unit);
        GE_GL_CHECK();
    }

    IEngine* getInstance()
    {
        static Engine engine_inst;
        return &engine_inst;
    }

    IEngine::~IEngine()
    {
    }
}
