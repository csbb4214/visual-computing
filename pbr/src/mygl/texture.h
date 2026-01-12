#pragma once
#include <string>
#include <glad/glad.h>

// Lädt eine 2D-Textur via stb_image.
// srgb=true: Albedo/BaseColor in sRGB anlegen (OpenGL konvertiert zu linear beim Sampling).
// srgb=false: Daten-Texturen (metal/rough/ao/normal) linear.
GLuint textureLoad2D(const std::string& filepath, bool srgb);

// 1x1 Fallback-Textur (z.B. wenn Datei fehlt)
GLuint textureCreate1x1(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
