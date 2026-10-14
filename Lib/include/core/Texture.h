#pragma once

#include <string>
#include <vector>

#include <glad/gl.h>

#include "helpers/Utils.h"

/*!
 * Texture
 * Data structure storing the index, GL_TEXTUREX and uid
 * of an OpenGL texture.
 * May also hold a QImage if loaded externally.
 */
class Texture {
public:
	Texture();
	Texture(std::string path, unsigned int _index = 0);
	Texture(unsigned int _index);

	~Texture();

	void init();
	void bind();
	void update();
	void setupForFramebuffer(unsigned int _width, unsigned int _height,
		unsigned int _format,
		unsigned int _globalFormat = GL_RGBA);
	bool load(std::string path);
	void resize(unsigned int _width, unsigned int _height);

	void setFilters(GLint min, GLint mag);

	static void resetUnit(int textureUnitOffset = 0);
	static Texture fromNextUnit();
	static Texture* newFromNextUnit();
	static unsigned int unitFromIndex(unsigned int index);

	int getHeight();
	int getWidth();

	// Needs to be public to be accessed by GL calls
	unsigned int gluid = 0;
	unsigned int glunit = 0;
	unsigned int index = 0;

	static bool needsUpdate;

private:
	static unsigned int unitCount;
	static float borderColor[];
	static float borderColorB[];

	std::vector<unsigned char> image;
	bool hasImage = false;
	unsigned int height = 0;
	unsigned int width = 0;
	GLint minFilter = GL_LINEAR;
	GLint magFilter = GL_LINEAR;
	unsigned int format = 0;
	unsigned int globalFormat = 0;
};
