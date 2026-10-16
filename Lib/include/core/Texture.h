#pragma once

#include <string>
#include <vector>

#include <glad/gl.h>

#include "helpers/Utils.h"

/*!
 * Texture
 * Data structure storing the index, GL_TEXTUREX and uid
 * of an OpenGL texture.
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
	// HDR Phase 1: color render targets default to RGBA16F half float.
	// 16F is texture-filterable (linear blur/bloom passes sample it)
	// at half the bandwidth of 32F. Depth callers use
	// setupForDepthShadow; image uploads keep their byte path in init().
	void setupForFramebuffer(unsigned int _width, unsigned int _height,
		unsigned int _format = GL_RGBA16F,
		unsigned int _globalFormat = GL_RGBA);
	void setupForDepthShadow(unsigned int _width, unsigned int _height);
	void enableShadowCompare();
	bool load(std::string path);
	void resize(unsigned int _width, unsigned int _height);

	void setFilters(GLint min, GLint mag);

	// Wrap mode for image textures (default REPEAT). Non-seamless assets
	// tiled on world-derived UVs (e.g. the six ground maps) show
	// filtering seams at every tile edge under REPEAT; MIRRORED_REPEAT
	// blends each edge with itself and kills the seam with no new assets.
	void setWrap(GLint wrapS, GLint wrapT);
	GLint wrapS() const;
	GLint wrapT() const;

	// Texture-unit map: every texture names its unit explicitly at
	// construction (or via setUnit), so co-live samplers never share
	// one. Units may repeat across passes because each bind() rebinds
	// before drawing; only one draw's set must be distinct.
	//   0  full-res post chain (texScene) / material diffuse slot
	//   1  half-res post chain (texScene), mainFBO texThreshold /
	//      material alpha slot
	//   2  luminance target + lens dirt
	//   3-5  ground extra maps (ground spans 0-5)
	//   7-8  shadow maps
	// (Cube maps such as the skybox sit on a separate GL binding point
	// per unit, so sharing an index with a 2D texture samples fine.)
	void setUnit(unsigned int unitIndex);
	unsigned int unit() const;

	static unsigned int unitFromIndex(unsigned int index);

	int getHeight();
	int getWidth();

	// Needs to be public to be accessed by GL calls
	unsigned int gluid = 0;
	unsigned int glunit = 0;
	unsigned int index = 0;

	static bool needsUpdate;

private:
	static float borderColor[];
	static float borderColorB[];

	std::vector<unsigned char> image;
	bool hasImage = false;
	unsigned int height = 0;
	unsigned int width = 0;
	GLint minFilter = GL_LINEAR;
	GLint magFilter = GL_LINEAR;
	GLint wrapModeS = GL_REPEAT;
	GLint wrapModeT = GL_REPEAT;
	unsigned int format = 0;
	unsigned int globalFormat = 0;
	unsigned int pixelType = GL_UNSIGNED_BYTE;
	bool shadowCompare = false;
};
