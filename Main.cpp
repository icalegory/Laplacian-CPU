//From https://www.opengl.org/discussion_boards/showthread.php/181714-Does-opengl-help-in-the-display-of-an-existing-image
//and
//http://www.songho.ca/opengl/gl_pbo.html#unpack

#include <stdlib.h>
#include <helper_gl.h>
#include <GL/freeglut.h>
#include <IL/il.h>
//#include <Windows.h>

#define WINDOW_WIDTH  640
#define WINDOW_HEIGHT 480

int width = WINDOW_WIDTH;
int height = WINDOW_HEIGHT;
const int CHANNEL_COUNT = 4;
const int DATA_SIZE = WINDOW_WIDTH * WINDOW_HEIGHT * CHANNEL_COUNT;

GLuint pbo;			                // IDs of PBO
GLuint textureId;                   // ID of texture
GLubyte* imageData = 0;             // pointer to texture buffer

/* Handler for window-repaint event. Called back when the window first appears and
whenever the window needs to be re-painted. */
void display()
{
	glBindTexture(GL_TEXTURE_2D, textureId);

	// Clear color and depth buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);     // Operate on model-view matrix

	/* Draw a quad */
	glBegin(GL_QUADS);
	glTexCoord2i(0, 0); glVertex2i(0, 0);
	glTexCoord2i(0, 1); glVertex2i(0, height);
	glTexCoord2i(1, 1); glVertex2i(width, height);
	glTexCoord2i(1, 0); glVertex2i(width, 0);
	glEnd();

	glutSwapBuffers();
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

	//DWORD nBufferLength = MAX_PATH;
	//wchar_t szCurrentDirectory[MAX_PATH + 1];
	//GetCurrentDirectory(nBufferLength, szCurrentDirectory);
	//printf("Current directory:\n");
	//wprintf(L"%s\n",szCurrentDirectory);
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
	//glutInit(&argc, /*static_cast<char**>*/(argv));            // Initialize GLUT
	//glutInit(&argc, &WideCharToMultiByte(argv));
	glutInit(&argc, (argv2));
	glutInitDisplayMode(GLUT_DOUBLE); // Enable double buffered mode
	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);   // Set the window's initial width & height
	//glutCreateWindow(argv[0]);      // Create window with the name of the executable
	glutCreateWindow("Display Image");
	glutDisplayFunc(display);       // Register callback handler for window re-paint event
	glutReshapeFunc(reshape);       // Register callback handler for window re-size event

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

	/* OpenGL texture binding of the image loaded by DevIL  */
	glGenTextures(1, &textureId); /* Texture name generation */
	glBindTexture(GL_TEXTURE_2D, textureId); /* Binding of texture name */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); /* We will use linear interpolation for magnification filter */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); /* We will use linear interpolation for minifying filter */
	//or:
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	// ilGetData() returns (unsigned char*) or (ILubyte*) or (GLvoid*)
	glTexImage2D(GL_TEXTURE_2D, 0, ilGetInteger(IL_IMAGE_BPP), ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT),
		0, ilGetInteger(IL_IMAGE_FORMAT), GL_UNSIGNED_BYTE, ilGetData()); /* Texture specification */
	//or:
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, IMAGE_WIDTH, IMAGE_HEIGHT, 0, PIXEL_FORMAT, GL_UNSIGNED_BYTE, (GLvoid*)imageData);
	glBindTexture(GL_TEXTURE_2D, 0);

	// create 1 pixel buffer objects, you need to delete them when program exits.
	// glBufferDataARB with NULL pointer reserves only memory space.
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

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

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