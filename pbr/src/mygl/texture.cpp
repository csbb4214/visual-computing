#include "texture.h"

#include <iostream>
#include <vector>

#define STBI_NO_SIMD
#include <stb_image/stb_image.h>

GLuint textureCreate1x1(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    unsigned char pixel[4] = { r, g, b, a };

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

GLuint textureLoad2D(const std::string& filepath, bool srgb)
{
    int w = 0, h = 0, comp = 0;
    stbi_set_flip_vertically_on_load(true);

    unsigned char* data = stbi_load(filepath.c_str(), &w, &h, &comp, 0);
    if (!data)
    {
        std::cerr << "[textureLoad2D] Could not load: " << filepath << " -> using fallback\n";
        return textureCreate1x1(255, 0, 255, 255); // Magenta
    }

    GLenum srcFormat = GL_RGB;
    GLenum internalFormat = GL_RGB8;

    if (comp == 1) { srcFormat = GL_RED;  internalFormat = GL_R8; }
    if (comp == 3) { srcFormat = GL_RGB;  internalFormat = srgb ? GL_SRGB8 : GL_RGB8; }
    if (comp == 4) { srcFormat = GL_RGBA; internalFormat = srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8; }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, srcFormat, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
    return tex;
}
