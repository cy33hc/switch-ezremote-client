#pragma once

#include <glad/glad.h>
#include <switch.h>
#include <vector>

typedef struct {
    GLuint id = 0;
    int width = 0;
    int height = 0;
    int delay = 0;
} Tex;

namespace Textures {
    bool LoadImageFile(const std::string &path, Tex &texture);
    void Free(Tex &texture);
    void Init(void);
    void Exit(void);
}