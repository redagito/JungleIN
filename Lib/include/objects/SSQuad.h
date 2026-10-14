#pragma once

#include <glad/gl.h>

/*!
 * SSQuad
 * Initializes and displays a ScreenSpaceQuad to render a texture fullscreen
 * for post processing.
 */
class SSQuad
{
public:
	SSQuad();

	void init();
	void draw(unsigned int posIndex);

private:
	static const float vertices[8];
	GLuint vbuffer = 0;
	GLuint vao = 0;
};