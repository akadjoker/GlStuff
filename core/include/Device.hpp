#pragma once


#include <SDL2/SDL.h>
#include "Config.hpp"


class  CORE_PUBLIC  Device 
{
public:
  
    bool Create(int width, int height,const char* title,bool vzync=false);
 
    bool Run();

    void Close();

    void Flip();

   
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }


    void Wait(float ms);
    void SetTargetFPS(int fps);
    int  GetFPS(void);
    float GetFrameTime(void);
    double GetTime(void);
    u32  GetTicks(void) ;

    void SetShouldClose(bool close) { m_shouldclose = close; }

    bool ShouldClose() const { return m_shouldclose; }

    void SetCloseKey(Sint32 key) { m_closekey = key; }
    bool IsReady() const { return m_ready; }
    bool IsResize() const { return m_is_resize; }


    SDL_Window*   GetWindow() const { return m_window; }

    static Device& Instance();
    static Device* InstancePtr();
   
private:
    int m_width;
    int m_height;    
    bool m_shouldclose ;
    bool m_is_resize;
    SDL_Window   *m_window;
    SDL_GLContext m_context;

    double m_current;                 
    double m_previous;                  
    double m_update;                    
    double m_draw;                       
    double m_frame;   
    double m_target;
    bool m_ready;
    Sint32 m_closekey;

    Device();
    ~Device();
     
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;
    Device(Device&&) = delete;
    Device& operator=(Device&&) = delete;

};



