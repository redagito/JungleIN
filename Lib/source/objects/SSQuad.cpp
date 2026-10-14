#include "objects/SSQuad.h"

#include <glad/glad.h>

SSQuad::SSQuad() { vbuffer = 0; }

const float SSQuad::vertices[8] = { -1.0f, -1.0f, 1.0f, -1.0f,
								   -1.0f, 1.0f,  1.0f, 1.0f };

void SSQuad::init() {
	glGenBuffers(1, &vbuffer);

	glBindBuffer(GL_ARRAY_BUFFER, vbuffer);
	glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(float), 0, GL_STATIC_DRAW);
	float* vert = (float*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

	for (int i = 0; i < 8; i++)
		vert[i] = vertices[i];

	glUnmapBuffer(GL_ARRAY_BUFFER);

	glGenVertexArrays(1, &vao);
}

void SSQuad::draw(unsigned int posIndex) {
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	glBindVertexArray(vao);
	glEnableVertexAttribArray(posIndex);
	glBindBuffer(GL_ARRAY_BUFFER, vbuffer);
	glVertexAttribPointer(posIndex, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisableVertexAttribArray(posIndex);
	glBindVertexArray(0);
}
