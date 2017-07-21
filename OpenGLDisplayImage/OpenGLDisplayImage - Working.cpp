// OpenGLDisplayImage.cpp : Defines the entry point for the console application.
//

//From https://www.opengl.org/discussion_boards/showthread.php/181714-Does-opengl-help-in-the-display-of-an-existing-image
//and
//http://www.songho.ca/opengl/gl_pbo.html#unpack

//#include "stdafx.h"
#include <stdlib.h>
#include <helper_gl.h>
#include <GL/freeglut.h>
//#include <GL/glew.h>
//#include <GL/glext.h>
#include <IL/il.h>
#include <Windows.h>

#include <memory> // for unique_ptr
#include <array>

//const int WINDOW_WIDTH = 1920;
//const int WINDOW_HEIGHT = 1080;
const int WINDOW_WIDTH = 640;
const int WINDOW_HEIGHT = 360;  //480
const int GL_WINDOW_WIDTH = WINDOW_WIDTH + 16;
const int GL_WINDOW_HEIGHT = WINDOW_HEIGHT * 2 + 17;

int width = WINDOW_WIDTH;
int height = WINDOW_HEIGHT;
const int IMAGE_WIDTH = 1920;
const int IMAGE_HEIGHT = 1080;
//const int IMAGE_WIDTH = 640;
//const int IMAGE_HEIGHT = 360;
const int CHANNEL_COUNT = 4;
const int PBO_COUNT = 2;
const int DATA_SIZE = IMAGE_WIDTH * IMAGE_HEIGHT * CHANNEL_COUNT;
const GLenum PIXEL_FORMAT = GL_BGRA;
const float CAMERA_DISTANCE = 5.0f;

GLuint pboIds[PBO_COUNT];		    // IDs of PBOs
//GLuint pbo;			            // ID of PBO
GLuint textureId;                   // ID of texture
//GLubyte* imageData = 0;           // pointer to texture buffer
GLubyte* colorBuffer = 0;
//std::unique_ptr<GLubyte> colorBuffer(new GLubyte[DATA_SIZE]());
int numberOfPasses = 0;

const byte square3x3[][3] =  {	{ 1, 1, 1 },
								{ 1, 1, 1 },
								{ 1, 1, 1 } };

const byte disk5x5[][5] = { { 0, 1, 1, 1, 0 },
							{ 1, 1, 1, 1, 1 },
							{ 1, 1, 1, 1, 1 },
							{ 1, 1, 1, 1, 1 },
							{ 0, 1, 1, 1, 0 } };

const byte ring5x5[][5] = { { 0, 1, 1, 1, 0 },
							{ 1, 0, 0, 0, 1 },
							{ 1, 0, 0, 0, 1 },
							{ 1, 0, 0, 0, 1 },
							{ 0, 1, 1, 1, 0 } };

//std::array<std::array<int, 3>, 2> a{ {
//	{ { 1,2,3 } },
//	{ { 4,5,6 } }
//	} };

// Use C++ 11 style arrays
std::array<std::array<int, 5>, 5> structuringElement { {
	{ { 0, 1, 1, 1, 0 } },
	{ { 1, 1, 1, 1, 1 } },
	{ { 1, 1, 1, 1, 1 } },
	{ { 1, 1, 1, 1, 1 } },
	{ { 0, 1, 1, 1, 0 } }
} };

// Use C++ 11 style arrays
//std::array<std::array<int, 3>, 3> structuringElement{ {
//	{ { 0, 1, 0 } },
//	{ { 1, 1, 1 } },
//	{ { 0, 1, 0 } }
//	} };


// Copy an image data to texture buffer - for unpacking (writing) to PBO
void updatePixels(GLubyte* dst, int size)
{
	static int color = 0;

	if (!dst)
		return;

	int* ptr = (int*)dst;

	// copy 4 bytes at once
	for (int i = 0; i < IMAGE_HEIGHT; ++i)
	{
		for (int j = 0; j < IMAGE_WIDTH; ++j)
		{
			*ptr = color;
			++ptr;
		}
		color += 257;   // add an arbitary number (no meaning)
	}
	++color;            // scroll down
}

// Change the brightness
void add(unsigned char* src, int width, int height, int shift, unsigned char* dst)
{
	if (!src || !dst || numberOfPasses > 2)
		return;

	int value;
	for (int i = 0; i < height; ++i)
	{
		//printf("r=%d, ", (*src));
		//printf("g=%d, ", (*(src+1)));
		//printf("b=%d, ", (*(src+2)));
		for (int j = 0; j < width; ++j)
		{
			//printf("r=%d, ", (*src));
			value = *src + shift;
			if (value > 255) *dst = (unsigned char)255;
			else            *dst = (unsigned char)value;
			++src;
			++dst;

			//printf("g=%d, ", (*src));
			value = *src + shift;
			if (value > 255) *dst = (unsigned char)255;
			else            *dst = (unsigned char)value;
			++src;
			++dst;

			//printf("b=%d, ", (*src));
			value = *src + shift;
			if (value > 255) *dst = (unsigned char)255;
			else            *dst = (unsigned char)value;
			++src;
			++dst;

			++src;    // skip alpha
			++dst;
		}
	}
	numberOfPasses++;
}

GLubyte clamp(GLubyte value, GLubyte min, GLubyte max)
{
	/*
	int max = 0;
	if (value > minIn)
		max = value;
	else
		max = minIn;
	int min = 255;
	if (max < maxIn)
		min = max;
	else
		min = maxIn;
	return min;
	*/
	return min(max(value, min), max);
}

// Apply the Morphological Laplacian Operator
void morphologicalLaplacian(unsigned char* src, int width, int height, int shift, unsigned char* dst)
{
	if (!src || !dst || numberOfPasses > 2)
		return;

	//Actually, don't have to use extra buffers, as this can be computed on a point-by-point basis
	//GLubyte* morphologicallyDilatedBuffer = 0;
	//GLubyte* morphologicallyErodedBuffer = 0;
	//morphologicallyDilatedBuffer = new GLubyte[DATA_SIZE];
	//memset(morphologicallyDilatedBuffer, 255, DATA_SIZE);
	//morphologicallyErodedBuffer = new GLubyte[DATA_SIZE];
	//memset(morphologicallyErodedBuffer, 255, DATA_SIZE);
	//GLubyte* eroded = morphologicallyErodedBuffer;
	//GLubyte* dilated = morphologicallyDilatedBuffer;
	unsigned char* index = src;
	for (int i = 0; i < height; ++i)
	{
		for (int j = 0; j < width; ++j)
		{	
			// Now cycle through every pixel of the structuring element, & process
			// both dilation and erosion of the original image.
			GLubyte rMin = 255, gMin = 255, bMin = 255;
			GLubyte rMax = 0, gMax = 0, bMax = 0;
			int maxVert = structuringElement.size() / 2;

			//printf("\nx=%d, y=%d\n", j, i);
			// Tread k as the structuring element's x coordinate
			for (int k = -maxVert; k <= maxVert; k++) {
				//printf("size=%d, k=%d\n", structuringElement.size(), k);
				int maxHoriz = structuringElement[k+maxVert].size() / 2;
				// Treat l as the structuring element's y coordinate
				for (int l = -maxHoriz; l <= maxHoriz; l++)
				{
					//printf("size=%d, l=%d\n", structuringElement[k+maxHoriz].size(), l);
					// Make sure that the structuring element has a value of 1 in the position being processed,
					// and that the point with the structuring element overlaid is also inside the bounds of the image.
					if (structuringElement[k+maxVert][l+maxHoriz] == 1 && i + k >= 0 && i + k < height && j + l >= 0 && j + l < width)
					{
						// Determine offset
						int offset = k*width*CHANNEL_COUNT + l*CHANNEL_COUNT;

						//int sx = (j + l), sy = (i + k);
						//int ax = ((index - src) + offset) % width;
						//int ay = ((index - src) + offset) / width;
						//printf("k=%2d, l=%2d, sx=%4d, sy=%4d; ax=%4d, ay=%4d\n", k, l, sx, sy, ax, ay);

						// Checks for dilation
						if ((*(index + offset)) > rMax)
							rMax = (*(index + offset));
						if ((*(index + offset + 1)) > gMax)
							gMax = (*(index + offset + 1));
						if ((*(index + offset + 2)) > bMax)
							bMax = (*(index + offset + 2));

						// Checks for erosion
						if ((*(index + offset)) < rMin)
							rMin = (*(index + offset));
						if ((*(index + offset + 1)) < gMin)
							gMin = (*(index + offset + 1));
						if ((*(index + offset + 2)) < bMin)
							bMin = (*(index + offset + 2));
					}
				}
			}
			// Wow, psychedelic lines!!!
			//*dst = clamp((rMax + rMin) / 2 - *index, 0, 255);
			//(*(dst + 1)) = clamp((gMax + gMin) / 2 - (*(index+1)), 0, 255);
			//(*(dst + 2)) = clamp((bMax + bMin) / 2 - (*(index+2)), 0, 255);

			// Almost same as above
			//*dst = (unsigned char)((rMax + rMin) / 2 - *index);
			//(*(dst + 1)) = (unsigned char)((gMax + gMin) / 2 - (*(index+1)));
			//(*(dst + 2)) = (unsigned char)((bMax + bMin) / 2 - (*(index+2)));

			// Almost a black and white result
			//*dst = (unsigned char)(((rMax + rMin) / 2 - *index) / 2 + 255);
			//(*(dst + 1)) = (unsigned char)(((gMax + gMin) / 2 - (*(index + 1))) / 2 + 255);
			//(*(dst + 2)) = (unsigned char)(((bMax + bMin) / 2 - (*(index + 2))) / 2 + 255);

			// Excellent and very succinct outlines!  Colorizes to blue and yellow 
			// This is the Laplacian according to http://www.mif.vu.lt/atpazinimas/dip/FIP/fip-Morpholo.html
			// which defines it as ½(dilation+erosion-2*source).  
			//*dst = (unsigned char)((rMax + rMin - 2*(*index))/2);
			//(*(dst + 1)) = (unsigned char)(gMax + gMin - 2*(*(index+1))/2);
			//(*(dst + 2)) = (unsigned char)(bMax + bMin - 2*(*(index+2))/2);

			// This clamping mechanism was found in imageJ
			//unsigned char rExternalGradientDilation = clamp(rMax - *index, 0, 255);
			//unsigned char gExternalGradientDilation = clamp(gMax - *(index + 1), 0, 255);
			//unsigned char bExternalGradientDilation = clamp(bMax - *(index + 2), 0, 255);
			//unsigned char rInternalGradientErosion = clamp(rMin - *index, 0, 255);
			//unsigned char gInternalGradientErosion = clamp(gMin - *(index + 1), 0, 255);
			//unsigned char bInternalGradientErosion = clamp(bMin - *(index + 2), 0, 255);
			//*dst = (unsigned char)clamp(rExternalGradientDilation - rInternalGradientErosion + 128, 0, 255);
			//(*(dst + 1)) = (unsigned char)clamp(gExternalGradientDilation - gInternalGradientErosion + 128, 0, 255);
			//(*(dst + 2)) = (unsigned char)clamp(bExternalGradientDilation - bInternalGradientErosion + 128, 0, 255);

			// From imageJ (very similar result as above)
			//*dst = clamp(rMax - rMin + 128, 0, 255);
			//(*(dst + 1)) = clamp(gMax - gMin + 128, 0, 255);
			//(*(dst + 2)) = clamp(bMax - bMin + 128, 0, 255);

			// Excellent results--mostly black except the outlines
			//*dst = ((rMax - rMin) / 2);
			//(*(dst + 1)) = ((gMax - gMin) / 2);
			//(*(dst + 2)) = ((bMax - bMin) / 2);

			// Good results, and is very similar to the one below (so these are both SECOND BEST)
			//*dst = clamp((((rMax + rMin) - 2*(*index))/2 + 255)/2, 0, 255);
			//(*(dst + 1)) = clamp((((gMax + gMin) -2*(*(index + 1)))/2 + 255)/2, 0, 255);
			//(*(dst + 2)) = clamp((((bMax + bMin) -2*(*(index + 2)))/2 + 255)/2, 0, 255);

			//**** Wow, very good, all gray scale SECOND BEST
			//*dst = ((rMax + rMin) / 2 - *index) / 2 + 128;
			//(*(dst + 1)) = ((gMax + gMin) / 2 - (*(index + 1))) / 2 + 128;
			//(*(dst + 2)) = ((bMax + bMin) / 2 - (*(index + 2))) / 2 + 128;

			// This is wrong--uses src instead of index, but it produces a unique result--
			// good gray outlines, though rest of image is fuzzy
			//*dst = ((rMax + rMin) / 2 - *src) >= 0 ? ((rMax + rMin) / 2 - *src) : 0;
			//(*(dst + 1)) = ((gMax + gMin) / 2 - (*(src + 1))) >= 0 ? ((gMax + gMin) / 2 - (*(src + 1))) : 0;
			//(*(dst + 2)) = ((bMax + bMin) / 2 - (*(src + 2))) >= 0 ? ((bMax + bMin) / 2 - (*(src + 2))) : 0;

			// This is very succinct and crisp and clear!  Mostly black, which outlines etched in sharp white
			// THE BEST OUT OF ALL OF THEM
			*dst = ((rMax + rMin) / 2 - *index) >= 0 ? ((rMax + rMin) / 2 - *index) : 0;
			(*(dst + 1)) = ((gMax + gMin) / 2 - (*(index + 1))) >= 0 ? ((gMax + gMin) / 2 - (*(index + 1))) : 0;
			(*(dst + 2)) = ((bMax + bMin) / 2 - (*(index + 2))) >= 0 ? ((bMax + bMin) / 2 - (*(index + 2))) : 0;

			/*
			unsigned char red = (unsigned char)(((rMax + rMin) / 2 - *index)*0.21);
			unsigned char green = (unsigned char)(((gMax + gMin) / 2 - (*(index + 1)))*0.72);
			unsigned char blue = (unsigned char)(((bMax + bMin) / 2 - (*(index + 2)))*0.07);
			//*dst = ((rMax + rMin) / 2 - *index) >= 0 ? red+green+blue : 0;
			//(*(dst + 1)) = ((gMax + gMin) / 2 - (*(index + 1))) >= 0 ? red+green+blue : 0;
			//(*(dst + 2)) = ((bMax + bMin) / 2 - (*(index + 2))) >= 0 ? red+green+blue : 0;
			*dst = red + green + blue;
			(*(dst + 1)) = red + green + blue;
			(*(dst + 2)) = red + green + blue;
			*/

			// Looks like very succinct three shades of gray
			// This is a luminosity-type conversion to grayscale
			//unsigned char red = (unsigned char)((((rMax + rMin) / 2 - *index)/2 + 255)*0.21);
			//unsigned char green = (unsigned char)((((gMax + gMin) / 2 - (*(index + 1)))/2 + 255)*0.72);
			//unsigned char blue = (unsigned char)((((bMax + bMin) / 2 - (*(index + 2)))/2 + 255)*0.07);
			////*dst = ((rMax + rMin) / 2 - *index) >= 0 ? red+green+blue : 0;
			////(*(dst + 1)) = ((gMax + gMin) / 2 - (*(index + 1))) >= 0 ? red+green+blue : 0;
			////(*(dst + 2)) = ((bMax + bMin) / 2 - (*(index + 2))) >= 0 ? red+green+blue : 0;
			//*dst = red + green + blue;
			//(*(dst + 1)) = red + green + blue;
			//(*(dst + 2)) = red + green + blue;

			//src += 4;
			index += 4;
			dst += 4;
		}
	}
	numberOfPasses++;
	//delete[] morphologicallyDilatedBuffer;
	//morphologicallyErodedBuffer = 0;
	//delete[] morphologicallyErodedBuffer;
	//morphologicallyErodedBuffer = 0;
}

void toOrtho();
void toPerspective();

/* Handler for window-repaint event. Called back when the window first appears and
whenever the window needs to be re-painted. */
void display()
{
	/*
	// The following code is added to be able to update the display image
	// on-the-fly, via unpacking (writing).

	// Bind the texture and PBO
	glBindTexture(GL_TEXTURE_2D, textureId);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, pbo);

	// Copy pixels from PBO to texture object
	// Use offset instead of pointer
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, IMAGE_WIDTH, IMAGE_HEIGHT, PIXEL_FORMAT, GL_UNSIGNED_BYTE, 0);

	// Measure the time copying data from PBO to texture object
	//t1.stop();
	//copyTime = t1.getElapsedTimeInMilliSec();

	// Start to modify pixel values
	//t1.start();

	// Bind PBO to update pixel values
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, pbo);

	// map the buffer object into client's memory
	// Note that glMapBufferARB() causes sync issue.
	// If GPU is working with this buffer, glMapBufferARB() will wait(stall)
	// for GPU to finish its job. To avoid waiting (stall), you can call
	// first glBufferDataARB() with NULL pointer before glMapBufferARB().
	// If you do that, the previous data in PBO will be discarded and
	// glMapBufferARB() returns a new allocated pointer immediately
	// even if GPU is still working with the previous data.
	glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, DATA_SIZE, 0, GL_STREAM_DRAW_ARB);
	GLubyte* ptr = (GLubyte*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
	if (ptr)
	{
	// update data directly on the mapped buffer
	updatePixels(ptr, DATA_SIZE);
	glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB); // release pointer to mapping buffer
	}

	// measure the time modifying the mapped buffer
	//t1.stop();
	//updateTime = t1.getElapsedTimeInMilliSec();
	///////////////////////////////////////////////////

	// it is good idea to release PBOs with ID 0 after use.
	// Once bound with 0, all pixel operations behave normal ways.
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
	*/

	//glViewport(0, 0, (GLsizei)WINDOW_WIDTH, (GLsizei)WINDOW_HEIGHT);
	glViewport(0, (GLsizei)WINDOW_HEIGHT, (GLsizei)WINDOW_WIDTH, (GLsizei)WINDOW_HEIGHT);
	static int shift = 0;
	static int index = 0;
	int nextIndex = 0;		//pbo index used for next frame
	// Brightness shift amount
	shift = ++shift % 200;
	// increment current index first then get the next index
	// "index" is used to read pixels from a framebuffer to a PBO
	// "nextIndex" is used to process pixels in the other PBO
	index = (index + 1) % 2;
	nextIndex = (index + 1) % 2;
	// set the framebuffer to read
	glReadBuffer(GL_FRONT);
	//t1.start();
	// copy pixels from framebuffer to PBO
	// Use offset instead of ponter.
	// OpenGL should perform asynch DMA transfer, so glReadPixels() will return immediately.
	glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, pboIds[index]);
	//glReadPixels(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, PIXEL_FORMAT, GL_UNSIGNED_BYTE, 0);
	glReadPixels(0, WINDOW_HEIGHT, WINDOW_WIDTH, WINDOW_HEIGHT, PIXEL_FORMAT, GL_UNSIGNED_BYTE, 0);
	// measure the time reading framebuffer
	//t1.stop();
	//readTime = t1.getElapsedTimeInMilliSec();
	// process pixel data /////////////////////////////
	//t1.start();
	// map the PBO that contains framebuffer pixels before processing it
	glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, pboIds[nextIndex]);
	GLubyte* src = (GLubyte*)glMapBuffer(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY_ARB);
	if (src)
	{
		// change brightness
		//add(src, WINDOW_WIDTH, WINDOW_HEIGHT, shift, colorBuffer);
		morphologicalLaplacian(src, WINDOW_WIDTH, WINDOW_HEIGHT, shift, colorBuffer);
		glUnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB);     // release pointer to the mapped buffer
	}
	// measure the time reading framebuffer
	//t1.stop();
	//processTime = t1.getElapsedTimeInMilliSec();
	///////////////////////////////////////////////////
	glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0);

	// render to the framebuffer //////////////////////////
	glDrawBuffer(GL_BACK);

	//glPushMatrix();
	//toPerspective(); // set to perspective to the top of the window

	// The following code is the original display() code:
	// Clear color and depth buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	//or:
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);     // Operate on model-view matrix

	// tramsform camera
	//glTranslatef(0, 0, -CAMERA_DISTANCE);
	///glRotatef(cameraAngleX, 1, 0, 0); // pitch
	//glRotatef(0, 1, 0, 0);   // pitch
	///glRotatef(cameraAngley, 0, 1, 0);
	//glRotatef(0, 0, 1, 0);   // heading

	/* Draw a quad */
	glBegin(GL_QUADS);
	glTexCoord2i(0, 0); glVertex2i(0, 0);
	glTexCoord2i(0, 1); glVertex2i(0, height/2);
	glTexCoord2i(1, 1); glVertex2i(width, height/2);
	glTexCoord2i(1, 0); glVertex2i(width, 0);
	glEnd();
	//glPopMatrix();

	//glPushMatrix();
	// draw the read color buffer to the bottom half of the window
	toOrtho();      // set to orthographic on the bottom half of the window
	//had to change this from 0,0 to 0,WINDOW_HEIGHT:
	glRasterPos2i(0, WINDOW_HEIGHT);
	//glRasterPos2i(0, 0);
	//glRasterPos2i(0, -360);
	glDrawPixels(WINDOW_WIDTH, WINDOW_HEIGHT, PIXEL_FORMAT, GL_UNSIGNED_BYTE, colorBuffer);
	//glPopMatrix();
	
	// This is required when using the unpack method, above.
	// Unbind texture
	//glBindTexture(GL_TEXTURE_2D, 0);

	//exit(1);

	glutSwapBuffers();
	// Put this in the idle() callback instead
	glutPostRedisplay();
}

///////////////////////////////////////////////////////////////////////////////
// set projection matrix as orthogonal
///////////////////////////////////////////////////////////////////////////////
void toOrtho()
{
	// set viewport to be the entire window
	glViewport(0, 0, (GLsizei)WINDOW_WIDTH, (GLsizei)WINDOW_HEIGHT);
	//glViewport(0, (GLsizei)WINDOW_HEIGHT, (GLsizei)WINDOW_WIDTH, (GLsizei)WINDOW_HEIGHT);

	// set orthographic viewing frustum
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	//glOrtho(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT, -1, 1);
	//Had to swap 0 and WINDOW_HEIGHT, because the view was upside down
	glOrtho(0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, -1, 1);

	// switch to modelview matrix in order to set scene
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

///////////////////////////////////////////////////////////////////////////////
// set the projection matrix as perspective
///////////////////////////////////////////////////////////////////////////////
void toPerspective()
{
	// set viewport to be the entire window
	glViewport(0, 0, (GLsizei)WINDOW_WIDTH, (GLsizei)WINDOW_HEIGHT);
	//glViewport(0, (GLsizei)WINDOW_HEIGHT, (GLsizei)WINDOW_WIDTH, (GLsizei)WINDOW_HEIGHT);

	// set perspective viewing frustum
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0f, (float)(WINDOW_WIDTH) / WINDOW_HEIGHT, 0.1f, 1000.0f); // FOV, AspectRatio, NearClip, FarClip

	// switch to modelview matrix in order to set scene
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void displayFirst()
{
	//glPushMatrix();
	// normal drawing to the framebuffer, so, the first call of glReadPixels()
	// in display() will get valid content

	//toPerspective();
	// clear buffer
	/*
	glViewport(0, 0, (GLsizei)WINDOW_WIDTH, (GLsizei)WINDOW_HEIGHT);
	glMatrixMode(GL_PROJECTION);     // Make a simple 2D projection on the entire window
	glLoadIdentity();
	glOrtho(0.0, (GLsizei)WINDOW_WIDTH, (GLsizei)WINDOW_HEIGHT, 0.0, -1, 1);
	*/

	glViewport(0, (GLsizei)WINDOW_HEIGHT, (GLsizei)WINDOW_WIDTH, (GLsizei)WINDOW_HEIGHT);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// tramsform camera
	//glTranslatef(0, 0, -cameraDistance);
	//glRotatef(cameraAngleX, 1, 0, 0);   // pitch
	//glRotatef(cameraAngleY, 0, 1, 0);   // heading

	// draw a cube
	glMatrixMode(GL_MODELVIEW);     // Operate on model-view matrix
	// Draw a quad
	glBegin(GL_QUADS);
	glTexCoord2i(0, 0); glVertex2i(0, 0);
	glTexCoord2i(0, 1); glVertex2i(0, height/2);
	glTexCoord2i(1, 1); glVertex2i(width, height/2);
	glTexCoord2i(1, 0); glVertex2i(width, 0);
	glEnd();
	//glPopMatrix();

	//glutPostRedisplay();

	//glPushMatrix();
	// draw the read color buffer to the bottom half of the window
	//glViewport(0, (GLsizei)WINDOW_HEIGHT, (GLsizei)WINDOW_WIDTH, (GLsizei)WINDOW_HEIGHT);
	toOrtho();      // set to orthographic on the bottom half of the window
	//had to change this from 0,0 to 0,WINDOW_HEIGHT:
	glRasterPos2i(0, WINDOW_HEIGHT);
	//glRasterPos2i(0, WINDOW_HEIGHT);
	glDrawPixels(WINDOW_WIDTH, WINDOW_HEIGHT, PIXEL_FORMAT, GL_UNSIGNED_BYTE, colorBuffer);
	//glPopMatrix();
	
	// draw info messages
	//showInfo();
	//printTransferRate();

	glutSwapBuffers();
	// Put this in the idle() callback instead
	//glutPostRedisplay();

	// switch to read-back drawing callback function
	glutDisplayFunc(display);
}

/* Handler for window re-size event. Called back when the window first appears and
whenever the window is re-sized with its new width and height */
void reshape(GLsizei newwidth, GLsizei newheight)
{
	// Set the viewport to cover the new window
	//glViewport(0, 0, width = newwidth, height = newheight*2);
	glViewport(0, 0, width = newwidth, height = newheight);
	//glViewport(0, height = newheight, width = newwidth, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, width, height, 0.0, -1, 1);
	//glOrtho(0.0, width, 0.0, height, -1, 1);
	glMatrixMode(GL_MODELVIEW);

	// Attempting to reshape the window to the original dimensions
	// (the textures get all garbled on resize, so I attempted to
	// prevent the window from being resized) seems to have no effect.
	glutReshapeWindow(GL_WINDOW_WIDTH, GL_WINDOW_HEIGHT);
	glutPostRedisplay();
}

void idle()
{
	glutPostRedisplay();
}

/* Initialize OpenGL Graphics */
void initGL(int w, int h)
{
	glViewport(0, 0, w, h); // use a screen size of WIDTH x HEIGHT
	//glViewport(0, 0, w, h / 2);
	glEnable(GL_TEXTURE_2D);     // Enable 2D texturing
	//glEnable(GL_CULL_FACE);

	glMatrixMode(GL_PROJECTION);     // Make a simple 2D projection on the entire window
	glLoadIdentity();
	glOrtho(0.0, w, h, 0.0, -1, 1);
	//glOrtho(0.0, w, 0, h, -1, 1);

	glMatrixMode(GL_MODELVIEW);    // Set the matrix mode to object modeling

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the window
}

/* Load an image using DevIL and return the devIL handle (-1 if failure) */
//int LoadImage(char *filename)
int LoadImage(wchar_t *filename)
{
	ILboolean success;
	ILuint image;

	DWORD nBufferLength = MAX_PATH;
	wchar_t szCurrentDirectory[MAX_PATH + 1];
	GetCurrentDirectory(nBufferLength, szCurrentDirectory);
	printf("Current directory:\n");
	wprintf(L"%s\n",szCurrentDirectory);
	//CurrentDirectory[MAX_PATH + 1] = '\0';

	ilGenImages(1, &image); /* Generation of one image name */
	ilBindImage(image); /* Binding of image name */
	success = ilLoadImage(filename); /* Loading of the image filename by DevIL */
	//success = ilLoadImage("ref_example.jpg");
	if (success)
	{
		// Convert every colour component into unsigned byte. If your image
		//contains alpha channel you can replace IL_RGB with IL_RGBA
		success = ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
		if (!success)
		{
			return -1;
		}
	}
	else
		return -1;
	return image;
}

//int main(int argc, char **argv)
//int wmain(int argc, wchar_t* argv[])
int wmain(int argc, wchar_t** argv)
{
	//GLuint texid;
	//int    image;
	ILuint image;

	//Trick glut:
	char** argv2 = 0;
	argv2 = static_cast<char**>(malloc(sizeof(char*) * 1));
	argv2[0] = static_cast<char*>(malloc(sizeof(char) * 11));
	argv2[0] = "OpenGL App\0";
	if (argc < 1)
	{
		// No image file to display
		return -1;
	}

	colorBuffer = new GLubyte[DATA_SIZE];
	memset(colorBuffer, 255, DATA_SIZE);

	//glutInit(&argc, /*static_cast<char**>*/(argv));
	//glutInit(&argc, &WideCharToMultiByte(argv));
	// Initialize GLUT
	glutInit(&argc, (argv2));
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_ALPHA);      // Enable double buffered mode
	//On Windows 10 the window is not quite the right width for some reason, so add to compensate
	glutInitWindowSize(GL_WINDOW_WIDTH, GL_WINDOW_HEIGHT);   // Set the window's initial width & height
	glutInitWindowPosition(100, 100);
	glutCreateWindow(argv2[0]);      // Create window with the name of the executable
	//glutCreateWindow("Display Image");

	if (!isGLVersionSupported(2, 0) ||
		!areGLExtensionsSupported("GL_ARB_vertex_buffer_object GL_ARB_pixel_buffer_object"))
	{
		printf("Error: failed to get minimal extensions for demo\n");
		printf("This sample requires:\n");
		printf("  OpenGL version 2.0\n");
		printf("  GL_ARB_vertex_buffer_object\n");
		printf("  GL_ARB_pixel_buffer_object\n");
		exit(EXIT_FAILURE);
	}

	//if (!glfwInit() || !glewInit())
	//if (!glewInit)
	//	return -1;

	// OpenGL 2D generic init
	initGL(WINDOW_WIDTH, WINDOW_HEIGHT);

	// Initialization of DevIL
	if (ilGetInteger(IL_VERSION_NUM) < IL_VERSION)
	{
		printf("wrong DevIL version \n");
		return -1;
	}
	ilInit();

	// Load the file picture with DevIL
	//image = LoadImage(argv[1]);
	//image = LoadImage(TEXT("ref_example.jpg"));
	//image = LoadImage(TEXT("ref_example.ppm"));
	//image = LoadImage(L"ref_example.jpg");
	//image = LoadImage(L"ref_example_upsidedown.jpg");
	image = LoadImage(L"ref_example_640x360.jpg");
	if (image == -1)
	{
		printf("Can't load picture file %s by DevIL \n", argv[1]);
		return -1;
	}

	printf("bpp=%d, width=%d, height=%d, format=%d\n", ilGetInteger(IL_IMAGE_BPP), ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT),
		0, ilGetInteger(IL_IMAGE_FORMAT));

	if (glGenTextures)
		printf("glGenTextures() is activated and available\n");
	if (glBindTexture)
		printf("glBindTexture() is activated and available\n");

	// OpenGL texture binding of the image loaded by DevIL 
	glGenTextures(1, &textureId); // Texture name generation
	glBindTexture(GL_TEXTURE_2D, textureId); // Binding of texture name
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // We will use linear interpolation for magnification filter
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // We will use linear interpolation for minifying filter
	//or:
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	// ilGetData() returns (unsigned char*) or (ILubyte*) or (GLvoid*)
	glTexImage2D(GL_TEXTURE_2D, 0, ilGetInteger(IL_IMAGE_BPP), ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT),
		0, ilGetInteger(IL_IMAGE_FORMAT), GL_UNSIGNED_BYTE, ilGetData()); // Texture specification
	//or:
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, IMAGE_WIDTH, IMAGE_HEIGHT, 0, PIXEL_FORMAT, GL_UNSIGNED_BYTE, (GLvoid*)imageData);
	
	//Uncommenting this will blank out the picture
	//glBindTexture(GL_TEXTURE_2D, 0);

	//glLoad::LoadFunctions();
	// create 1 pixel buffer objects, you need to delete them when program exits.
	// glBufferDataARB with NULL pointer reserves only memory space.
	
	if (glGenBuffers)
		printf("glGenBuffers() is activated and available\n");
	else
		printf("glGenBuffers() is NOT activated and available\n");
	if (glBindBuffer)
		printf("glBindBuffer() is activated and available\n");
	else
		printf("glBindBuffer() is NOT activated and available\n");
	if (glReadPixels)
		printf("glReadPixels() is activated and available\n");
	else
		printf("glReadPixels() is NOT activated and available\n");

	//glGenBuffers(1, &pbo);  //2 instead of 1 for two PBO's
	glGenBuffers(2, pboIds);  //2 instead of 1 for two PBO's
	//glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, pbo);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, pboIds[0]);
	glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, DATA_SIZE, 0, GL_STREAM_DRAW_ARB);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, pboIds[1]);
	glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, DATA_SIZE, 0, GL_STREAM_DRAW_ARB);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

	//From Box Filter demo:
	// create pixel buffer object
	//glGenBuffers(1, &pbo);
	//glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, pbo);
	//glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, width*height * sizeof(GLubyte) * 4, h_img, GL_STREAM_DRAW_ARB);
	//glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

	//Set this only after displayFirst() is called (so I call it from there instead):
	glutDisplayFunc(displayFirst);       // Register callback handler for window re-paint event
	//glutDisplayFunc(display);
	glutIdleFunc(idle);
	glutReshapeFunc(reshape);         // Register callback handler for window re-size event
	//glutKeyboardFunc(keyboard);
	//glutTimerFunc(REFRESH_DELAY, timerEvent, 0);

	// Main loop
	glutMainLoop();

	// Delete used resources and quit
	const ILuint image2 = image;
	//ilDeleteImages(1, &image); /* Because we have already copied image data into texture data we can release memory used by image. */
	ilDeleteImages(1, &image2); /* Because we have already copied image data into texture data we can release memory used by image. */
	glDeleteTextures(1, &textureId);

	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	//glFlush();  //don't need this with GLUT_DOUBLE and glutSwapBuffers

	delete[] colorBuffer;
	colorBuffer = 0;

	free(argv2[0]);
	free(argv2);

	return 0;
}
