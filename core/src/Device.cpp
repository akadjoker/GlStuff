
#include "pch.h"
#include "Device.hpp"
#include "Input.hpp"
#include "Texture.hpp"
#include "Shader.hpp"
#include "Mesh.hpp"
#include "glad/glad.h"


double GetTime() { return static_cast<double>(SDL_GetTicks()) / 1000; }


void glDebugOutput(u32 source, u32 type, u32 id, u32 severity, s32 length,
                   const char* message, const void* userParam)
{
    (void)length;
    (void)userParam;
    (void)severity;
    GLuint ignore_ids[2] = { 131185, 1 }; // 1 shader ???

    for (int i = 0; i < (int)ARRAY_SIZE_IN_ELEMENTS(ignore_ids); i++)
    {
        if (id == ignore_ids[i])
        {
            return;
        }
    }

    //  SDL_LogWarn(1,"!!! Debug callback !!!\n");
    SDL_LogDebug(2, "Debug message: id %d, %s", id, message);

    std::string msg = std::string(message);
    std::string src;
    std::string stype;

    switch (source)
    {
        case GL_DEBUG_SOURCE_API: src = ("API"); break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM: src = ("Window System"); break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: src = ("Shader Compiler"); break;
        case GL_DEBUG_SOURCE_THIRD_PARTY: src = ("Third Party"); break;
        case GL_DEBUG_SOURCE_APPLICATION: src = ("Application"); break;
        case GL_DEBUG_SOURCE_OTHER: src = ("Other"); break;
    }

    //  printf("Error type: ");
    switch (type)
    {
        case GL_DEBUG_TYPE_ERROR: stype = ("Error"); break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            stype = ("Deprecated Behaviour");
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            stype = ("Undefined Behaviour");
            break;
        case GL_DEBUG_TYPE_PORTABILITY: stype = ("Portability"); break;
        case GL_DEBUG_TYPE_PERFORMANCE: stype = ("Performance"); break;
        case GL_DEBUG_TYPE_MARKER: stype = ("Marker"); break;
        case GL_DEBUG_TYPE_PUSH_GROUP: stype = ("Push Group"); break;
        case GL_DEBUG_TYPE_POP_GROUP: stype = ("Pop Group"); break;
        case GL_DEBUG_TYPE_OTHER: stype = ("Other"); break;
    }

    LogError("Source: [%s]: Type: {%s}: Message: (%s) ID - %d", src.c_str(),
             stype.c_str(), msg.c_str(), id);
}


//*************************************************************************************************
// Device
//*************************************************************************************************


Device& Device::Instance()
{
    static Device instance;
    return instance;
}
Device* Device::InstancePtr() { return &Instance(); }

Device::Device(): m_width(0), m_height(0)
{
    LogInfo("[DEVICE] Initialized.");

    m_shouldclose = false;
    m_window = NULL;
    m_context = NULL;
    m_current = 0;
    m_previous = 0;
    m_update = 0;
    m_draw = 0;
    m_frame = 0;
    m_target = 0;
    m_ready = false;
    m_is_resize = false;
}

Device::~Device()
{
    LogInfo("[DEVICE] Destroyed.");
    Close();
}


void LoadDefaultShader()
{
      const char *vShader = GLSL(

        layout(location = 0) in vec3 position;
        layout(location = 1) in vec4 color;


      
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 proj;


       out vec4 vertexColor; 
       
       void main() 
        {
            gl_Position = proj * view * model * vec4(position, 1.0);
            vertexColor = color;
        });


    const char *fShader =
        GLSL(
            
            out vec4 color; 
            in vec4 vertexColor;
            void main() 
             {
                 color = vertexColor;
                 

             });


             
             if (ShaderManager::Instance().Create(vShader, fShader, "Default"))
             {         
                 Shader* shader = ShaderManager::Instance().Get("Default");
                 
                 
                 shader->Create(vShader, fShader);
                 shader->LoadDefaults();

                 shader->Use(false);
                 Logger::Instance().Info("Default Shader Created");
                 
    } else 
    {
        Logger::Instance().Error("Failed to create Default Shader");
    }
    
}

void Load2DShader()
{
      const char *vShader = GLSL(

        layout(location = 0) in vec3 position;
        layout(location = 1) in vec2 texCoord;
        layout(location = 2) in vec4 color;


        uniform mat4 mvp;

        out vec2 TexCoord; 
        out vec4 vertexColor; 
        void main() 
        {
            gl_Position = mvp * vec4(position, 1.0);
            TexCoord = texCoord;
            vertexColor = color;
        });


    const char *fShader =
        GLSL(
            in vec2 TexCoord; 
            out vec4 color; 
            in vec4 vertexColor;
             uniform sampler2D texture0; void main() 
             {
                 color = texture(texture0, TexCoord) * vertexColor;
                 

             });


             
             if (ShaderManager::Instance().Create(vShader, fShader, "2DShader"))
             {         
                 Shader* shader = ShaderManager::Instance().Get("2DShader");
                 
                 
                 shader->Create(vShader, fShader);
                 shader->LoadDefaults();

                 shader->Use(false);
                 Logger::Instance().Info("Batch Shader Created");
                 
    } else 
    {
        Logger::Instance().Error("Failed to create Batch Shader");
    }
    
}


void Load3DShader()
{
      const char *vShader = GLSL(

    
    layout(location=0) in vec3 aPosition;
    layout(location=1) in vec2 aTexCoord;
    layout(location=2) in vec3 aNormal;
    layout(location=3) in vec4 aTangent;


    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 proj;

    out mediump vec2 vUV;
    out highp   mat3 vTBN;

    void main() 
    {
        // normal matrix no shader
        mat3 Nmat = transpose(inverse(mat3(model)));

        vec3 N = normalize(Nmat * aNormal);
        vec3 T = normalize(Nmat * aTangent.xyz);
        // Re-ortogonaliza T relativamente a N
        T = normalize(T - N * dot(T, N));
        vec3 B = normalize(cross(N, T)) * aTangent.w;

        vTBN = mat3(T, B, N);  // colunas = T, B, N
        vUV  = aTexCoord;

        gl_Position = proj * view * model * vec4(aPosition, 1.0);
    }
    
    );


    const char *fShader =
        GLSL(
        
            in mediump vec2 vUV;
            in mediump mat3 vTBN;

            uniform sampler2D diffuseMap;
            uniform sampler2D normalMap;
            uniform vec3 lightDirWorld; // Já normalizado
            uniform float bumpScale;      // 0 = liso, 1 = normal, 2 = “duro”, etc.

            out vec4 FragColor;

            void main() 
            {
                vec3 albedo = texture(diffuseMap, vUV).rgb;
                vec3 n_ts = texture(normalMap, vUV).xyz * 2.0 - 1.0;


                  //n_ts.y *= 1.0; //inverte


                n_ts.xy *= bumpScale;
                vec3 N = normalize(vTBN * n_ts);
                
                float NdotL = max(dot(N, lightDirWorld), 0.0);
                vec3 color = albedo * (0.1 + 0.9 * NdotL); // ambient
                
                FragColor = vec4(color, 1.0);
            }

        );


             
             if (ShaderManager::Instance().Create(vShader, fShader, "3DShader"))
             {         
                 Shader* shader = ShaderManager::Instance().Get("3DShader");
                 
                 
                 shader->Create(vShader, fShader);
                 shader->LoadDefaults();
                 shader->SetInt("diffuseMap", 0);
                 shader->SetInt("normalMap", 1);
                 shader->SetFloat("bumpScale", 0.0f);
                 Vec3 lightDirWorld(0.4f, 0.7f, 0.2f);
                 lightDirWorld.normalize();

                 shader->SetFloat("lightDirWorld", lightDirWorld.x, lightDirWorld.y, lightDirWorld.z);
                 shader->Use(false);
                 Logger::Instance().Info("3D Shader Created");
                 
    } else 
    {
        Logger::Instance().Error("Failed to create 3D Shader");
    }
    
}


void LoadDefaultShaders() 
{
     LoadDefaultShader();
     Load2DShader();
     Load3DShader();
}


bool Device::Create(int width, int height, const char* title, bool vzync)
{

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        LogError("SDL could not initialize! SDL_Error: %s", SDL_GetError());
        return false;
    }


    m_current = 0;
    m_previous = 0;
    m_update = 0;
    m_draw = 0;
    m_frame = 0;

    SetTargetFPS(vzync ? 60 : 1000);
    m_closekey = 256;
    m_width = width;
    m_height = height;

    m_current = GetTime();
    m_draw = m_current - m_previous;
    m_previous = m_current;
    m_frame = m_update + m_draw;

    // Atributos de contexto antes de criar a janela
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);

    const int majorVersion = 3;
    const int minorVersion =2; // pede 3.2 para ter debug core (altera para 3.1 para nao ter)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, majorVersion);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minorVersion);

    // Formato de framebuffer
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

    // MSAA  
    int sampleCount = 1; // mete >0 para ativar
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, sampleCount);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, sampleCount > 0 ? 1 : 0);

    // Debug flag (respeitado se o driver suportar)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

    // Criação
    m_window = SDL_CreateWindow(
        title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!m_window)
    {
        LogError("Window! %s", SDL_GetError());
        return false;
    }

    m_context = SDL_GL_CreateContext(m_window);
    if (!m_context)
    {
        LogError("Context! %s", SDL_GetError());
        return false;
    }

    // VSync
    SDL_GL_SetSwapInterval(vzync ? 1 : 0);

    // glad (GLES)
    if (!gladLoadGLES2Loader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        LogError("Failed to load GLES with glad");
        return false;
    }

    // Verificar versão carregada (glad 0.1.x)
    if (!(GLAD_GL_ES_VERSION_3_1))
    { // ou 3_2 se pediste 3.2
        LogError("OpenGL ES 3.1 não suportado pelo driver");
        return false;
    }

    // Debug output: ativa só se disponível
    bool hasDebug = GLAD_GL_ES_VERSION_3_2 || GLAD_GL_KHR_debug;
    if (hasDebug)
    {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(glDebugOutput, nullptr);
        // Exemplo: só mensagens “high” 
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE,
                              GL_DEBUG_SEVERITY_HIGH, 0, nullptr, GL_TRUE);
    }

    LogInfo("[DEVICE] Vendor  : %s", (const char*)glGetString(GL_VENDOR));
    LogInfo("[DEVICE] Renderer: %s", (const char*)glGetString(GL_RENDERER));
    LogInfo("[DEVICE] Version : %s", (const char*)glGetString(GL_VERSION));
    LogInfo("[DEVICE] GLSL ES : %s",
            (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));


    GLfloat maxAniso = 1.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);

    LogInfo("[DEVICE] Anisotropy: %f", maxAniso);


    TextureManager::Instance().Init();
    LoadDefaultShaders();



    
    


    //    glEnable(GL_MULTISAMPLE);


    m_ready = true;


    return true;
}


void Device::Wait(float ms) { SDL_Delay(ms); }


int Device::GetFPS(void)
{
#define FPS_CAPTURE_FRAMES_COUNT 30 // 30 captures
#define FPS_AVERAGE_TIME_SECONDS 0.5f // 500 millisecondes
#define FPS_STEP (FPS_AVERAGE_TIME_SECONDS / FPS_CAPTURE_FRAMES_COUNT)

    static int index = 0;
    static float history[FPS_CAPTURE_FRAMES_COUNT] = { 0 };
    static float average = 0, last = 0;
    float fpsFrame = GetFrameTime();

    if (fpsFrame == 0) return 0;

    if ((GetTime() - last) > FPS_STEP)
    {
        last = (float)GetTime();
        index = (index + 1) % FPS_CAPTURE_FRAMES_COUNT;
        average -= history[index];
        history[index] = fpsFrame / FPS_CAPTURE_FRAMES_COUNT;
        average += history[index];
    }

    return (int)roundf(1.0f / average);
}

void Device::SetTargetFPS(int fps)
{
    if (fps < 1)
        m_target = 0.0;
    else
        m_target = 1.0 / (double)fps;
}

float Device::GetFrameTime(void) { return (float)m_frame; }

double Device::GetTime(void) { return (double)SDL_GetTicks() / 1000.0; }

u32 Device::GetTicks(void) { return SDL_GetTicks(); }


bool Device::Run()
{
    if (!m_ready) return false;

    m_current = GetTime(); // Number of elapsed seconds since InitTimer()
    m_update = m_current - m_previous;
    m_previous = m_current;


    SDL_Event event;

    while (SDL_PollEvent(&event) != 0)
    {

        switch (event.type)
        {
            case SDL_QUIT: {
                m_shouldclose = true;
                break;
            }
            case SDL_WINDOWEVENT: {
                switch (event.window.event)
                {
                    case SDL_WINDOWEVENT_RESIZED: {
                        m_width = event.window.data1;
                        m_height = event.window.data2;
                        //glViewport(0, 0, m_width, m_height);
                        m_is_resize = true;

                        break;
                    }
                }
                break;
            }
            case SDL_KEYDOWN: {


                // LogWarning("[DEVICE] Key %d
                // %d",Keyboard::toKey(event.key.keysym.scancode),m_closekey);
                if (Keyboard::toKey(event.key.keysym.scancode) == m_closekey)
                {
                    m_shouldclose = true;
                    break;
                }

                Keyboard::setKeyState(event.key.keysym.scancode, true);

                break;
            }

            case SDL_KEYUP: {

                Keyboard::setKeyState(event.key.keysym.scancode, false);
                break;
            }
            break;
            case SDL_MOUSEBUTTONDOWN: {

                int btn = event.button.button - 1;
                if (btn == 2)
                    btn = 1;
                else if (btn == 1)
                    btn = 2;
                Mouse::setMouseButton(btn, true);
            }
            break;
            case SDL_MOUSEBUTTONUP: {
                int btn = event.button.button - 1;
                if (btn == 2)
                    btn = 1;
                else if (btn == 1)
                    btn = 2;
                Mouse::setMouseButton(btn, false);

                break;
            }
            case SDL_MOUSEMOTION: {
                Mouse::setMousePosition(event.motion.x, event.motion.y,
                                        event.motion.xrel, event.motion.yrel);
                break;
            }

            case SDL_MOUSEWHEEL: {
                Mouse::setMouseWheel(event.wheel.x, event.wheel.y);
                break;
            }
        }
    }


    return !m_shouldclose;
}

void Device::Close()
{
    if (!m_ready) return;

    m_ready = false;

    ShaderManager::Instance().Clear();
    TextureManager::Instance().Clear();
    MeshManager::Instance().Clear();


    SDL_GL_DeleteContext(m_context);
    SDL_DestroyWindow(m_window);

    m_window = NULL;
    LogInfo("[DEVICE] closed!");
    SDL_Quit();
}


void Device::Flip()
{


    SDL_GL_SwapWindow(m_window);

    m_current = GetTime();
    m_draw = m_current - m_previous;
    m_previous = m_current;
    m_frame = m_update + m_draw;


    // Wait for some milliseconds...
    if (m_frame < m_target)
    {
        Wait((float)(m_target - m_frame) * 1000.0f);

        m_current = GetTime();
        double waitTime = m_current - m_previous;
        m_previous = m_current;

        m_frame += waitTime; // Total frame time: update + draw + wait
    }

    // std::string fps= std::to_string(GetFPS());
    // std::string title = std::string("FPS: ") + fps;
    // SDL_SetWindowTitle(m_window,  title.c_str());

    Keyboard::Update();
    Mouse::Update();
    m_is_resize = false;
}

