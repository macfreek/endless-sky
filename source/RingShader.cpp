/* RingShader.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "RingShader.h"

#include "Color.h"
#include "pi.h"
#include "Point.h"
#include "Screen.h"
#include "Shader.h"

#include <stdexcept>

using namespace std;

namespace {
	Shader shader;
	GLint scaleI;
	GLint positionI;
	GLint radiusI;
	GLint widthI;
	GLint angleI;
	GLint dashI;
	GLint colorI;
	
	GLuint vao;
	GLuint vbo;
}



void RingShader::Init()
{
	static const char *vertexCode =
		"uniform vec2 scale;\n"
		"uniform vec2 position;\n"
		"uniform float radius;\n"
		"uniform float width;\n"
		
		"in vec2 vert;\n"
		"out vec2 coord;\n"
		
		"void main() {\n"
		"  coord = (radius + width) * vert;\n"
		"  gl_Position = vec4((coord + position) * scale, 0, 1);\n"
		"}\n";

	static const char *fragmentCode =
		"uniform vec4 color = vec4(1, 1, 1, 1);\n"
		"uniform float radius;\n"
		"uniform float width;\n"
		"uniform float angle;\n"
		"uniform float dash;\n"
		
		"in vec2 coord;\n"
		"out vec4 finalColor;\n"
		
		"void main() {\n"
		"  float arc = atan(coord.x, coord.y) + 3.141592654;\n"
		"  float arcFalloff = 1 - min(6.283185307 - arc, arc - angle) * radius;\n"
		"  if(dash != 0)\n"
		"  {\n"
		"    arc = mod(arc, dash);\n"
		"    arcFalloff = min(arcFalloff, min(arc, dash - arc) * radius);\n"
		"  }\n"
		"  float len = length(coord);\n"
		"  float lenFalloff = width - abs(len - radius);\n"
		"  float alpha = clamp(min(arcFalloff, lenFalloff), 0, 1);\n"
		"  finalColor = color * alpha;\n"
		"}\n";
	
	shader = Shader(vertexCode, fragmentCode);
	scaleI = shader.Uniform("scale");
	positionI = shader.Uniform("position");
	radiusI = shader.Uniform("radius");
	widthI = shader.Uniform("width");
	angleI = shader.Uniform("angle");
	dashI = shader.Uniform("dash");
	colorI = shader.Uniform("color");
	
	// Generate the vertex data for drawing sprites.
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	
	GLfloat vertexData[] = {
		-1.f, -1.f,
		-1.f,  1.f,
		 1.f, -1.f,
		 1.f,  1.f
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);
	
	glEnableVertexAttribArray(shader.Attrib("vert"));
	glVertexAttribPointer(shader.Attrib("vert"), 2, GL_FLOAT, GL_FALSE,
		2 * sizeof(GLfloat), NULL);
	
	// unbind the VBO and VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}



void RingShader::Draw(const Point &pos, float out, float in, const Color &color)
{
	float width = .5f * (1.f + out - in) ;
	Draw(pos, out - width, width, 1.f, color);
}



void RingShader::Draw(const Point &pos, float radius, float width, float fraction, const Color &color, float dash)
{
	Bind();
	
	Add(pos, radius, width, fraction, color, dash);
	
	Unbind();
}



void RingShader::Bind()
{
	if(!shader.Object())
		throw runtime_error("RingShader: Bind() called before Init().");
	
	glUseProgram(shader.Object());
	glBindVertexArray(vao);
	
	GLfloat scale[2] = {2.f / Screen::Width(), -2.f / Screen::Height()};
	glUniform2fv(scaleI, 1, scale);
}



void RingShader::Add(const Point &pos, float out, float in, const Color &color)
{
	float width = .5f * (1.f + out - in) ;
	Add(pos, out - width, width, 1.f, color);
}



void RingShader::Add(const Point &pos, float radius, float width, float fraction, const Color &color, float dash)
{
	GLfloat position[2] = {static_cast<float>(pos.X()), static_cast<float>(pos.Y())};
	glUniform2fv(positionI, 1, position);
	
	glUniform1f(radiusI, radius);
	glUniform1f(widthI, width);
	glUniform1f(angleI, fraction * 2. * PI);
	glUniform1f(dashI, dash ? 2. * PI / dash : 0.);
	
	glUniform4fv(colorI, 1, color.Get());
	
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}



void RingShader::Unbind()
{
	glBindVertexArray(0);
	glUseProgram(0);
}
