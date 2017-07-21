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

//#define WINDOW_WIDTH  1920
//#define WINDOW_HEIGHT 1080
#define WINDOW_WIDTH  640
#define WINDOW_HEIGHT 360  //480

int width = WINDOW_WIDTH;
int height = WINDOW_HEIGHT;
const int IMAGE_WIDTH = 1920;
const int IMAGE_HEIGHT = 1080;
const int CHANNEL_COUNT = 4;
const int DATA_SIZE = IMAGE_WIDTH * IMAGE_HEIGHT * CHANNEL_COUNT;
const GLenum PIXEL_FORMAT = GL_BGRA;

GLuint pbo;			                // IDs of PBO
GLuint textureId;                   // ID of texture
GLubyte* imageData = 0;             // pointer to texture buffer

///////////////////////////////////////////////////////////////////////////////
// copy an image data to texture buffer
///////////////////////////////////////////////////////////////////////////////
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

/* Handler for window-repaint event. Called back when the window first appears and
whenever the window needs to be re-painted. */
void display()
{
	// The following code is added to be able to update the display image
	// on-the-fly.

	// Bind the texture and PBO
	glBindTexture(GL_TEXTURE_2D, textureId);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, pbo);

	// Copy pixels from PBO to texture object
	// Use offset instead of pointer
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, IMAGE_WIDTH, IMAGE_HEIGHT, PIXEL_FORMAT, GL_UNSIGNED_BYTE, 0);

	// Mesaure the time copying data from PBO to texture object
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

	// The following code is the original display() code:
	// Clear color and depth buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//or:
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);     // Operate on model-view matrix

	/* Draw a quad */
	glBegin(GL_QUADS);
	glTexCoord2i(0, 0); glVertex2i(0, 0);
	glTexCoord2i(0, 1); glVertex2i(0, height);
	glTexCoord2i(1, 1); glVertex2i(width, height);
	glTexCoord2i(1, 0); glVertex2i(width, 0);
	glEnd();

	// This was added here after adding the beginning block of clode to display()
	// unbind texture
	glBindTexture(GL_TEXTURE_2D, 0);

	glutSwapBuffers();
	glutPostRedisplay();
}

/* Handler for window re-size event. Called back when the window first appears and
whenever the window is re-sized with its new width and height */
void reshape(GLsizei newwidth, GLsizei newheight)
{
	// Set the viewport to cover the new window
	glViewport(0, 0, width = newwidth, height = newheight);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, width, height, 0.0, 0.0, 100.0);
	glMatrixMode(GL_MODELVIEW);

	glutPostRedisplay();
}


/* Initialize OpenGL Graphics */
void initGL(int w, int h)
{
	glViewport(0, 0, w, h); // use a screen size of WIDTH x HEIGHT
	glEnable(GL_TEXTURE_2D);     // Enable 2D texturing

	glMatrixMode(GL_PROJECTION);     // Make a simple 2D projection on the entire window
	glLoadIdentity();
	glOrtho(0.0, w, h, 0.0, 0.0, 100.0);

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

	if (success) /* If no error occured: */
	{
		/* Convert every colour component into unsigned byte. If your image contains alpha channel you can replace IL_RGB with IL_RGBA */
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
	argv2[0] = "OpenGL_app\0";

	if (argc < 1)
	{
		/* no image file to  display */
		return -1;
	}

	/* GLUT init */
	//glutInit(&argc, /*static_cast<char**>*/(argv));
	//glutInit(&argc, &WideCharToMultiByte(argv));
	// Initialize GLUT
	glutInit(&argc, (argv2));
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);      // Enable double buffered mode
	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);   // Set the window's initial width & height
	//glutCreateWindow(argv[0]);      // Create window with the name of the executable
	glutCreateWindow("Display Image");

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

	glutDisplayFunc(display);       // Register callback handler for window re-paint event
	glutReshapeFunc(reshape);       // Register callback handler for window re-size event
	//glutKeyboardFunc(keyboard);
	//glutTimerFunc(REFRESH_DELAY, timerEvent, 0);

	/* OpenGL 2D generic init */
	initGL(WINDOW_WIDTH, WINDOW_HEIGHT);

	/* Initialization of DevIL */
	if (ilGetInteger(IL_VERSION_NUM) < IL_VERSION)
	{
		printf("wrong DevIL version \n");
		return -1;
	}
	ilInit();

	/* load the file picture with DevIL */
	//image = LoadImage(argv[1]);
	//image = LoadImage(TEXT("ref_example.jpg"));
	//image = LoadImage(TEXT("ref_example.ppm"));
	image = LoadImage(L"ref_example.jpg");
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
	if (glBindBuffer)
		printf("glBindBuffer() is activated and available\n");

	glGenBuffers(1, &pbo);  //2 instead of 1 for two PBO's
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, pbo);
	glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, DATA_SIZE, 0, GL_STREAM_DRAW_ARB);
	// for 2nd POB (not needed, as there is negligible performance benefit):
	//glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, pboIds[1]);
	//glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, DATA_SIZE, 0, GL_STREAM_DRAW_ARB);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

	//From Box Filter demo:
	// create pixel buffer object
	//glGenBuffers(1, &pbo);
	//glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, pbo);
	//glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, width*height * sizeof(GLubyte) * 4, h_img, GL_STREAM_DRAW_ARB);
	//glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

	/* Main loop */
	glutMainLoop();

	/* Delete used resources and quit */
	const ILuint image2 = image;
	//ilDeleteImages(1, &image); /* Because we have already copied image data into texture data we can release memory used by image. */
	ilDeleteImages(1, &image2); /* Because we have already copied image data into texture data we can release memory used by image. */
	glDeleteTextures(1, &textureId);

	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	//glFlush();  //don't need this with GLUT_DOUBLE and glutSwapBuffers

	free(argv2[0]);
	free(argv2);

	return 0;
}