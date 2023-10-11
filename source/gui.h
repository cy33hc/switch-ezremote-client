#ifndef LAUNCHER_GUI_H
#define LAUNCHER_GUI_H

#include <string>
#include "imgui_impl_switch.h"

#define GUI_MODE_BROWSER 0
#define GUI_MODE_IME 1

extern bool done;
extern int gui_mode;

namespace GUI
{
    int RenderLoop(void);
    bool Init(FontType font_type);
    bool SwapBuffers(void);
    void Render(void);
    void Exit(void);

}

#endif
