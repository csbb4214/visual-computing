#include "texture.h"

#include <vector>
#include <stdexcept>
#include <iostream>

#include <stb_image/stb_image.h>

Texture textureLoad(const std::string &path)
{
    int width = 0, height = 0, components = 0;

    /* flip image to match opengl's texture coordinates */
    stbi_set_flip_vertically_on_load(true);

    /* load image (required components=4 -> always RGBA returned)*/
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &components, 4);
    if(data == nullptr)
    {
        std::cerr << "[Texture] couldn't load image file " << path << std::endl;
        std::cerr.flush();
        throw std::runtime_error("[Texture] couldn't load image file " + path);
    }

    /* upload data */
    GLuint id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glCheckError();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glCheckError();

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);

    return Texture{id, (unsigned int) width, (unsigned int) height};
}

Texture textureCreateSingleColor(unsigned int width, unsigned int height, const Vector3D& color)
{
    // create texture with size widht height and color
    std::vector<unsigned char> data(width * height * 4);
    for(unsigned int i = 0; i < width * height; i++)
    {
        data[i * 4 + 0] = static_cast<unsigned char>(color.x * 255);
        data[i * 4 + 1] = static_cast<unsigned char>(color.y * 255);
        data[i * 4 + 2] = static_cast<unsigned char>(color.z * 255);
        data[i * 4 + 3] = 255;
    }

    /* upload data */
    GLuint id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    glCheckError();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glCheckError();

    glBindTexture(GL_TEXTURE_2D, 0);

    return Texture{id, (unsigned int) width, (unsigned int) height};
}

void textureDelete(const Texture &texture)
{
    glDeleteTextures(1, &texture.id);
}
