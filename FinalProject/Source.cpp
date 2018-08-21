// Gregory S. Fellis
// CS-330 | SNHU | 7-1 Final
// 8/7/2018
/* Description:
Creates a perspective OpenGL view and renders
a 3D low-poly harmonica. The user can navigate 
around the object by using the controls listed below.

WARNING: Don't hover over glfwCreateWindow function!!!!!
Long function descriptions cause VS2017 to lock up.
*/
/* Controls:
	F:			Resets view to starting position
	O:			Toggles orthographic viewing
	L:			Toggles drawing of light objects
	Space:		Toggles wireframe mode

	ALT + Left Mouse Button:	Orbits the camera, clamped at +-90degrees
	Scroll Wheel:				Zooms the camera in and out (changes FOV)
*/

#include <GLEW/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>

// GLM libraries
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <SOIL2\SOIL2.h>

using namespace std;

/* Constants */
const GLfloat FOV_MAX = 46.0f;
const GLfloat FOV_MIN = 44.25f;
const GLfloat SCROLL_SPEED = 0.05f;

/* Global Variables */
int width, height;			// Screen dimensions

bool keys[1024];			// Key press array
bool mouseButton[3];		// Mouse button press array
bool isOrbiting = false;	// Sets mouse movement to orbiting
bool wireFrame = false;		// Toggles wireframe mode
bool firstMouseMove = true;	// Detect initial mouse movement
bool ortho = false;			// Sets orthographic projection
bool lightDraw = false;		// Disable drawing of light objects

// Zoom
GLfloat fov = 45.0f;		// Initial fov value

// Orbiting
GLfloat degYaw, degPitch;	// degree values of Yaw and Pitch
GLfloat radius = 10.0f;		// Sets the radius for obiting
GLfloat rawYaw = 0.0f;		// raw value of Yaw
GLfloat rawPitch = 0.0f;	// raw value of Pitch
GLfloat deltaTime = 0.0f;	// deltaTime for consistent performance
GLfloat lastFrame = 0.0f;	// Last frame accessed for deltaTime calculation

GLfloat xChange, yChange;	// Difference in X,Y coordinates
GLfloat lastX;		// last X mouse position
GLfloat lastY;		// last Y mouse position

// Define camera attributes
glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, 10.0f);
glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 cameraDirection = glm::normalize(cameraPosition - target);
glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 cameraRight = glm::normalize(glm::cross(worldUp, cameraDirection));
glm::vec3 cameraUp = glm::normalize(glm::cross(cameraDirection, cameraRight));
glm::vec3 cameraFront = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f));

// Declare View Matrix
glm::mat4 viewMatrix;

// Lamp position
glm::vec3 lamp1Position(0.0f, 0.0f, 5.0f);
glm::vec3 lamp2Position(-3.0f, 1.0f, 6.0f);
glm::vec3 lamp3Position(3.0f, 1.0f, 6.0f);

/* Input Callback prototypes */
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

/* Camera transformation prototypes */
void TransformCamera();
void initCamera();

// Draw Primitive(s)
void draw(GLsizei indices)
{
	GLenum mode = GL_TRIANGLES;	
	glDrawElements(mode, indices, GL_UNSIGNED_BYTE, nullptr);
}

// Create and Compile Shaders
static GLuint CompileShader(const string& source, GLuint shaderType)
{
	// Create Shader object
	GLuint shaderID = glCreateShader(shaderType);
	const char* src = source.c_str();

	// Attach source code to Shader object
	glShaderSource(shaderID, 1, &src, nullptr);

	// Compile Shader
	glCompileShader(shaderID);

	// Return ID of Compiled shader
	return shaderID;
}

// Create Program Object
static GLuint CreateShaderProgram(const string& vertexShader, const string& fragmentShader)
{
	// Compile vertex shader
	GLuint vertexShaderComp = CompileShader(vertexShader, GL_VERTEX_SHADER);

	// Compile fragment shader
	GLuint fragmentShaderComp = CompileShader(fragmentShader, GL_FRAGMENT_SHADER);

	// Create program object
	GLuint shaderProgram = glCreateProgram();

	// Attach vertex and fragment shaders to program object
	glAttachShader(shaderProgram, vertexShaderComp);
	glAttachShader(shaderProgram, fragmentShaderComp);

	// Link shaders to create executable
	glLinkProgram(shaderProgram);

	// Delete compiled vertex and fragment shaders
	glDeleteShader(vertexShaderComp);
	glDeleteShader(fragmentShaderComp);

	// Return Shader Program
	return shaderProgram;
}

int main(void)
{
	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;

	/* Setup full screen window */
	GLFWmonitor* monitor = glfwGetPrimaryMonitor(); // Get primary monitor of system
	const GLFWvidmode* mode = glfwGetVideoMode(monitor); // Process primary monitor's video mode

	/* Setup the Window hints based on current monitor settings */
	glfwWindowHint(GLFW_RED_BITS, mode->redBits);
	glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
	glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
	glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

	/* Set width and height (half size of monitor resolution) */
	width = mode->width / 2;
	height = mode->height / 2;

	// Set lastX and lastY to middle of screen size
	lastX = width / 2;
	lastY = height / 2;

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(width, height, "Gregory S. Fellis", NULL, NULL);

	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	/* Setup input callback functions */
	glfwSetKeyCallback(window, key_callback); // Keyboard
	glfwSetCursorPosCallback(window, cursor_position_callback); // Mouse position
	glfwSetMouseButtonCallback(window, mouse_button_callback); // Mouse button
	glfwSetScrollCallback(window, scroll_callback); // Scroll wheel

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	if (glewInit() != GLEW_OK) {
		cout << "GLEW failed to initalize!" << endl;

		glfwTerminate();
		return -1;
	}

	/* Setup vertices and indices for objects */
	GLfloat reedV[] = {
		/* Top Reed */
		// Height	= 0.1cm
		// Width	= 10cm
		// Depth	= 2.5cm
		// color : 0.80, 0.65, 0.20, bronze

		// Vertex				// Color			// Texture	// Normal Z
		//X     Y		Z		//R    G     B		S    T		A	 B	  C
		-5.0,	+0.3,	0.0,	0.80, 0.65, 0.20,	0.0, 0.0,	0.0, 0.0, 1.0,	// 0 low-left back
		-5.0,	+0.4,	0.0,	1.00, 0.85, 0.40,	0.0, 1.0,	0.0, 0.0, 1.0,	// 1 top-left back
		+5.0,	+0.4,	0.0,	1.00, 0.85, 0.40,	1.0, 1.0,	0.0, 0.0, 1.0,	// 2 top-right back
		+5.0,	+0.3,	0.0,	0.80, 0.65, 0.20,	1.0, 0.0,	0.0, 0.0, 1.0,	// 3 low-right back

		-5.0,	+0.3,  +2.5,	0.80, 0.65, 0.20,	0.0, 1.0,	0.0, 0.0, 1.0,	// 4 low-right left
		-5.0,	+0.4,  +2.5,	1.00, 0.85, 0.40,	1.0, 0.0,	0.0, 0.0, 1.0,	// 5 top-right left

		+5.0,	+0.3,  +2.5,	0.80, 0.65, 0.20,	1.0, 1.0,	0.0, 0.0, 1.0,	// 6 low-left right
		+5.0,	+0.4,  +2.5,	1.00, 0.85, 0.40,	0.0, 0.0,	0.0, 0.0, 1.0,	// 7 top-left right
	};

	GLubyte reedI[] = {
		/* Top Reed */
		0,		1,		2,			2,		3,		0,			// back
		0,		4,		5,			5,		1,		0,			// left
		3,		6,		7,			7,		2,		3,			// right
		7,		6,		4,			4,		5,		7,			// front
		0,		3,		4,			4,		6,		3,			// bottom
		1,		2,		5,			5,		7,		2,			// top
	};
	
	GLfloat coverV[] = {
		/* Top Cover Plate v2 */
		// Height	= 0.5cm
		// Width	= 8.2cm (not counting flaps)
		// Depth	= 2.4cm
		// color : 0.65, 0.65, 0.85, silver

		/* Strip 1 */
		// Vertex				// Color			// Texture	// Normal Z
		//X     Y		Z		//R    G     B		S    T		A	 B	  C	
		-4.1,	+0.40,	+0.3,	0.65, 0.65, 0.85,	0.0, 0.0,	0.0, 0.0, 1.0,	// 0 low-left back 
		-4.1,	+0.525, +0.3,	0.65, 0.65, 0.85,	1.0, 0.0,	0.0, 0.0, 1.0,	// 1 top-left back 
		+4.1,	+0.525, +0.3,	0.65, 0.65, 0.85,	1.0, 1.0,	0.0, 0.0, 1.0,	// 2 top-right back 
		+4.1,	+0.40,	+0.3,	0.65, 0.65, 0.85,	0.0, 1.0,	0.0, 0.0, 1.0,	// 3 low-right back 
		-4.1,	+0.40,  +2.45,	0.65, 0.65, 0.85,	1.0, 0.0,	0.0, 0.0, 1.0,	// 4 low-right left 
		-4.1,	+0.525,	+2.39,	0.65, 0.65, 0.85,	1.0, 1.0,	0.0, 0.0, 1.0,	// 5 top-right left 
		+4.1,	+0.40,  +2.45,	0.65, 0.65, 0.85,	0.0, 0.0,	0.0, 0.0, 1.0,	// 6 low-left right 
		+4.1,	+0.525, +2.39,	0.65, 0.65, 0.85,	0.0, 1.0,	0.0, 0.0, 1.0,	// 7 top-left right 

		/* Strip 2 */
		// Vertex				// Color			// Texture	// Normal Z
		//X     Y		Z		//R    G     B		S    T		A	 B	  C		
		-4.1,	+0.525, +0.3,	0.65, 0.65, 0.85,	0.0, 0.0,	0.0, 0.0, 1.0,	// 8 low-left back 
		-4.1,	+0.65,	+0.3,	0.65, 0.65, 0.85,	1.0, 0.0,	0.0, 0.0, 1.0,	// 9 top-left back 
		+4.1,	+0.65,	+0.3,	0.65, 0.65, 0.85,	1.0, 1.0,	0.0, 0.0, 1.0,	// 10 top-right back 
		+4.1,	+0.525,	+0.3,	0.65, 0.65, 0.85,	0.0, 1.0,	0.0, 0.0, 1.0,	// 11 low-right back 
		-4.1,	+0.525, +2.39,	0.65, 0.65, 0.85,	1.0, 0.0,	0.0, 0.0, 1.0,	// 12 low-right left 
		-4.1,	+0.65,  +2.30,	0.65, 0.65, 0.85,	1.0, 1.0,	0.0, 0.0, 1.0,	// 13 top-right left 
		+4.1,	+0.525, +2.39,	0.65, 0.65, 0.85,	0.0, 0.0,	0.0, 0.0, 1.0,	// 14 low-left right 
		+4.1,	+0.65,  +2.30,	0.65, 0.65, 0.85,	0.0, 1.0,	0.0, 0.0, 1.0,	// 15 top-left right 

		/* Strip 3 */
		// Vertex				// Color			// Texture	// Normal Z
		//X     Y		Z		//R    G     B		S    T		A	 B	  C		
		-4.1,	+0.65,	+0.3,	0.65, 0.65, 0.85,	0.0, 0.0,	0.0, 0.0, 1.0,	// 16 low-left back 
		-4.1,	+0.775,	+0.3,	0.65, 0.65, 0.85,	1.0, 0.0,	0.0, 0.0, 1.0,	// 17 top-left back 
		+4.1,	+0.775,	+0.3,	0.65, 0.65, 0.85,	1.0, 1.0,	0.0, 0.0, 1.0,	// 18 top-right back 
		+4.1,	+0.65,	+0.3,	0.65, 0.65, 0.85,	0.0, 1.0,	0.0, 0.0, 1.0,	// 19 low-right back 
		-4.1,	+0.65,  +2.30,	0.65, 0.65, 0.85,	1.0, 0.0,	0.0, 0.0, 1.0,	// 20 low-right left 
		-4.1,	+0.775, +2.175,	0.65, 0.65, 0.85,	1.0, 1.0,	0.0, 0.0, 1.0,	// 21 top-right left 
		+4.1,	+0.65,  +2.30,	0.65, 0.65, 0.85,	0.0, 0.0,	0.0, 0.0, 1.0,	// 22 low-left right 
		+4.1,	+0.775, +2.175,	0.65, 0.65, 0.85,	0.0, 1.0,	0.0, 0.0, 1.0,	// 23 top-left right 

		/* Strip 4 */
		// Vertex				// Color			// Texture	// Normal Z
		//X     Y		Z		//R    G     B		S    T		A	 B	  C		
		-4.1,	+0.775,	+0.3,	0.65, 0.65, 0.85,	0.0, 0.0,	0.0, 0.0, 1.0,	// 24 low-left back 
		-4.1,	+0.80,	+0.3,	0.65, 0.65, 0.85,	1.0, 0.0,	0.0, 0.0, 1.0,	// 25 top-left back 
		+4.1,	+0.80,	+0.3,	0.65, 0.65, 0.85,	1.0, 1.0,	0.0, 0.0, 1.0,	// 26 top-right back 
		+4.1,	+0.775,	+0.3,	0.65, 0.65, 0.85,	0.0, 1.0,	0.0, 0.0, 1.0,	// 27 low-right back 
		-4.1,	+0.775, +2.175,	0.65, 0.65, 0.85,	1.0, 0.0,	0.0, 0.0, 1.0,	// 28 low-right left 
		-4.1,	+0.90,  +2.0,	0.85, 0.85, 1.00,	1.0, 1.0,	0.0, 0.0, 1.0,	// 29 top-right left 
		+4.1,	+0.775, +2.175,	0.65, 0.65, 0.85,	0.0, 0.0,	0.0, 0.0, 1.0,	// 30 low-left right 
		+4.1,	+0.90,  +2.0,	0.85, 0.85, 1.00,	0.0, 1.0,	0.0, 0.0, 1.0,	// 31 top-left right

		/* Back Fin */
		// Vertex				// Color			// Texture	// Normal Z
		//X     Y		Z		//R    G     B		S    T		A	 B	  C		
		-3.9,	+1.00,	+0.0,	0.85, 0.85, 1.00,	0.0, 0.0,	0.0, 0.0, 1.0,	// 32 top-left back slope
		+3.9,	+1.00,	+0.0,	0.85, 0.85, 1.00,	0.0, 1.0,	0.0, 0.0, 1.0,	// 33 top-right back slope
		+3.7,	+1.00,	+0.0,	0.65, 0.65, 0.85,	1.0, 1.0,	0.0, 0.0, 1.0,	// 34 top-right back
		-3.9,	+1.00,	+0.0,	0.65, 0.65, 0.85,	1.0, 0.0,	0.0, 0.0, 1.0,	// 35 top-left back
		+3.7,	+0.60,	+0.0,	0.85, 0.85, 1.00,	0.0, 1.0,	0.0, 0.0, 1.0,	// 36 bottom-right back
		-3.9,	+0.60,	+0.0,	0.85, 0.85, 1.00,	0.0, 0.0,	0.0, 0.0, 1.0,	// 37 bottom-left back
	};

	GLubyte coverI[] = {
		/*	Top	Cover Plate */
		// Strip 1														
		0,		4,		5,			5,		1,		0,			// left
		3,		6,		7,			7,		2,		3,			// right
		7,		6,		4,			4,		5,		7,			// front

		// Strip 2	
		8,		12,		13,			13,		9,		8,			// left
		11,		14,		15,			15,		10,		11,			// right
		15,		14,		12,			12,		13,		15,			// front

		// Strip 3	
		16,		20,		21,			21,		17,		16,			// left
		19,		22,		23,			23,		18,		19,			// right
		23,		22,		20,			20,		21,		23,			// front

		// Strip 4	
		24,		28,		29,			29,		25,		24,			// left
		27,		30,		31,			31,		26,		27,			// right
		31,		30,		28,			28,		29,		31,			// front

		// Top	
		25,		26,		29,			29,		31,		26,			// top

		// Back Fin	
		25,		32,		33,			33,		26,		25,			// front
		34,		35,		36,			36,		35,		37,			// back
	};

	GLfloat combV[] = {
		/* Comb */
		// Height	= 0.6cm
		// Width	= 10cm
		// Depth	= 2.5cm
		// color : 0.82, 0.42, 0.12, chocolate brown

		// Vertex				// Color			// Texture	// Normal Z
		//X     Y		Z		//R    G     B		S    T		A	 B	  C
		-5.0,	 0.0,	0.0,	0.20, 0.20, 0.20,	0.0, 0.0,	0.0, 0.0, 1.0,	// 0 low-left back
		-5.0,	+0.3,	0.0,	0.20, 0.20, 0.20,	0.1, 0.0,	0.0, 0.0, 1.0,	// 1 top-left back
		+5.0,	+0.3,	0.0,	0.20, 0.20, 0.20,	0.1, 1.0,	0.0, 0.0, 1.0,	// 2 top-right back
		+5.0,	 0.0,	0.0,	0.20, 0.20, 0.20,	0.0, 1.0,	0.0, 0.0, 1.0,	// 3 low-right back

		// Left Block
		// Vertex				// Color			// Texture	// Normal Z
		//X     Y		Z		//R    G     B		S    T		A	 B	  C
		-5.0,	 0.0,	+2.5,	0.82, 0.42, 0.12,	0.0, 1.0,	0.0, 0.0, 1.0,	// 4 low-right left
		-5.0,	+0.3,	+2.5,	0.82, 0.42, 0.12,	0.1, 1.0,	0.0, 0.0, 1.0,	// 5 top-right left
		-3.35,	 0.0,	+2.5,	0.82, 0.42, 0.12,	0.0, 0.0,	0.0, 0.0, 1.0,	// 6 low-right front
		-3.35,	+0.3,	+2.5,	0.82, 0.42, 0.12,	0.1, 0.0,	0.0, 0.0, 1.0,	// 7 top-right front
		-3.35,	 0.0,	0.0,	0.82, 0.42, 0.12,	0.0, 1.0,	0.0, 0.0, 1.0,	// 8 low-right front
		-3.35,	+0.3,	0.0,	0.82, 0.42, 0.12,	0.1, 1.0,	0.0, 0.0, 1.0,	// 9 top-right front

		/* Divider 1 */
		// Vertex				// Color			// Texture	// Normal Z
		//X     Y		Z		//R    G     B		S    T		A	 B	  C
		-2.95,	 0.0,  +0.0,	0.82, 0.42, 0.12,	0.0, 0.0,	0.0, 0.0, 1.0,	// 10 low-left back
		-2.95,	+0.3,  +0.0,	0.82, 0.42, 0.12,	0.1, 0.0,	0.0, 0.0, 1.0,	// 11 top-left back
		-2.65,	+0.3,  +0.0,	0.82, 0.42, 0.12,	0.1, 1.0,	0.0, 0.0, 1.0,	// 12 top-right back
		-2.65,	+0.0,  +0.0,	0.82, 0.42, 0.12,	0.0, 1.0,	0.0, 0.0, 1.0,	// 13 low-right back

		-2.95,	 0.0,	+2.5,	0.82, 0.42, 0.12,	0.0, 1.0,	0.0, 0.0, 1.0,	// 14 low-right front
		-2.95,	+0.3,	+2.5,	0.82, 0.42, 0.12,	0.1, 1.0,	0.0, 0.0, 1.0,	// 15 top-right front
		-2.65,	 0.0,	+2.5,	0.82, 0.42, 0.12,	0.0, 0.0,	0.0, 0.0, 1.0,	// 16 low-right front
		-2.65,	+0.3,	+2.5,	0.82, 0.42, 0.12,	0.1, 0.0,	0.0, 0.0, 1.0,	// 17 top-right front
		-2.65,	 0.0,	+0.0,	0.82, 0.42, 0.12,	0.0, 1.0,	0.0, 0.0, 1.0,	// 18 low-right front
		-2.65,	+0.3,	+0.0,	0.82, 0.42, 0.12,	0.1, 1.0,	0.0, 0.0, 1.0,	// 19 top-right front

		/* Divider 2 */
		// Vertex				// Color			// Texture	// Normal Z
		//X     Y		Z		//R    G     B		S    T		A	 B	  C
		-2.25,	 0.0,  +0.0,	0.82, 0.42, 0.12,	0.0, 0.0,	0.0, 0.0, 1.0,	// 20 low-left back
		-2.25,	+0.3,  +0.0,	0.82, 0.42, 0.12,	0.1, 0.0,	0.0, 0.0, 1.0,	// 21 top-left back
		-1.95,	+0.3,  +0.0,	0.82, 0.42, 0.12,	0.1, 1.0,	0.0, 0.0, 1.0,	// 22 top-right back
		-1.95,	+0.0,  +0.0,	0.82, 0.42, 0.12,	0.0, 1.0,	0.0, 0.0, 1.0,	// 23 low-right back

		-2.25,	 0.0,	+2.5,	0.82, 0.42, 0.12,	0.0, 1.0,	0.0, 0.0, 1.0,	// 24 low-right front
		-2.25,	+0.3,	+2.5,	0.82, 0.42, 0.12,	0.1, 1.0,	0.0, 0.0, 1.0,	// 25 top-right front
		-1.95,	 0.0,	+2.5,	0.82, 0.42, 0.12,	0.0, 0.0,	0.0, 0.0, 1.0,	// 26 low-right front
		-1.95,	+0.3,	+2.5,	0.82, 0.42, 0.12,	0.1, 0.0,	0.0, 0.0, 1.0,	// 27 top-right front
		-1.95,	 0.0,	+0.0,	0.82, 0.42, 0.12,	0.0, 1.0,	0.0, 0.0, 1.0,	// 28 low-right front
		-1.95,	+0.3,	+0.0,	0.82, 0.42, 0.12,	0.1, 1.0,	0.0, 0.0, 1.0,	// 29 top-right front

		/* Divider 3 */
		// Vertex				// Color			// Texture	// Normal Z
		//X     Y		Z		//R    G     B		S    T		A	 B	  C	
		-1.55,	 0.0,  +0.0,	0.82, 0.42, 0.12,	0.0, 0.0,	0.0, 0.0, 1.0,	// 30 low-left back
		-1.55,	+0.3,  +0.0,	0.82, 0.42, 0.12,	0.1, 0.0,	0.0, 0.0, 1.0,	// 31 top-left back
		-1.25,	+0.3,  +0.0,	0.82, 0.42, 0.12,	0.1, 1.0,	0.0, 0.0, 1.0,	// 32 top-right back
		-1.25,	+0.0,  +0.0,	0.82, 0.42, 0.12,	0.0, 1.0,	0.0, 0.0, 1.0,	// 33 low-right back

		-1.55,	 0.0,	+2.5,	0.82, 0.42, 0.12,	0.0, 1.0,	0.0, 0.0, 1.0,	// 34 low-right front
		-1.55,	+0.3,	+2.5,	0.82, 0.42, 0.12,	0.1, 1.0,	0.0, 0.0, 1.0,	// 35 top-right front
		-1.25,	 0.0,	+2.5,	0.82, 0.42, 0.12,	0.0, 0.0,	0.0, 0.0, 1.0,	// 36 low-right front
		-1.25,	+0.3,	+2.5,	0.82, 0.42, 0.12,	0.1, 0.0,	0.0, 0.0, 1.0,	// 37 top-right front
		-1.25,	 0.0,	+0.0,	0.82, 0.42, 0.12,	0.0, 1.0,	0.0, 0.0, 1.0,	// 38 low-right front
		-1.25,	+0.3,	+0.0,	0.82, 0.42, 0.12,	0.1, 1.0,	0.0, 0.0, 1.0,	// 39 top-right front

		/* Divider 4 */
		// Vertex				// Color			// Texture	// Normal Z
		//X     Y		Z		//R    G     B		S    T		A	 B	  C
		-0.85,	 0.0,  +0.0,	0.82, 0.42, 0.12,	0.0, 0.0,	0.0, 0.0, 1.0,	// 40 low-left back
		-0.85,	+0.3,  +0.0,	0.82, 0.42, 0.12,	0.1, 0.0,	0.0, 0.0, 1.0,	// 41 top-left back
		-0.55,	+0.3,  +0.0,	0.82, 0.42, 0.12,	0.1, 1.0,	0.0, 0.0, 1.0,	// 42 top-right back
		-0.55,	+0.0,  +0.0,	0.82, 0.42, 0.12,	0.0, 1.0,	0.0, 0.0, 1.0,	// 43 low-right back

		-0.85,	 0.0,	+2.5,	0.82, 0.42, 0.12,	0.0, 1.0,	0.0, 0.0, 1.0,	// 44 low-right front
		-0.85,	+0.3,	+2.5,	0.82, 0.42, 0.12,	0.1, 1.0,	0.0, 0.0, 1.0,	// 45 top-right front
		-0.55,	 0.0,	+2.5,	0.82, 0.42, 0.12,	0.0, 0.0,	0.0, 0.0, 1.0,	// 46 low-right front
		-0.55,	+0.3,	+2.5,	0.82, 0.42, 0.12,	0.1, 0.0,	0.0, 0.0, 1.0,	// 47 top-right front
		-0.55,	 0.0,	+0.0,	0.82, 0.42, 0.12,	0.0, 1.0,	0.0, 0.0, 1.0,	// 48 low-right front
		-0.55,	+0.3,	+0.0,	0.82, 0.42, 0.12,	0.1, 1.0,	0.0, 0.0, 1.0,	// 49 top-right front

		/* Divider 5 */
		// Vertex				// Color			// Texture	// Normal Z
		//X     Y		Z		//R    G     B		S    T		A	 B	  C
		-0.15,	 0.0,  +0.0,	0.82, 0.42, 0.12,	0.0, 0.0,	0.0, 0.0, 1.0,	// 50 low-left back
		-0.15,	+0.3,  +0.0,	0.82, 0.42, 0.12,	0.1, 0.0,	0.0, 0.0, 1.0,	// 51 top-left back
		+0.15,	+0.3,  +0.0,	0.82, 0.42, 0.12,	0.1, 1.0,	0.0, 0.0, 1.0,	// 52 top-right back
		+0.15,	+0.0,  +0.0,	0.82, 0.42, 0.12,	0.0, 1.0,	0.0, 0.0, 1.0,	// 53 low-right back

		-0.15,	 0.0,	+2.5,	0.82, 0.42, 0.12,	0.0, 1.0,	0.0, 0.0, 1.0,	// 54 low-right front
		-0.15,	+0.3,	+2.5,	0.82, 0.42, 0.12,	0.1, 1.0,	0.0, 0.0, 1.0,	// 55 top-right front
		+0.15,	 0.0,	+2.5,	0.82, 0.42, 0.12,	0.0, 0.0,	0.0, 0.0, 1.0,	// 56 low-right front
		+0.15,	+0.3,	+2.5,	0.82, 0.42, 0.12,	0.1, 0.0,	0.0, 0.0, 1.0,	// 57 top-right front
		+0.15,	 0.0,	+0.0,	0.82, 0.42, 0.12,	0.0, 1.0,	0.0, 0.0, 1.0,	// 58 low-right front
		+0.05,	+0.3,	+0.0,	0.82, 0.42, 0.12,	0.1, 1.0,	0.0, 0.0, 1.0,	// 59 top-right front

		/* Divider 6 */
		// Vertex				// Color			// Texture	// Normal Z
		//X     Y		Z		//R    G     B		S    T		A	 B	  C
		+0.85,	 0.0,  +0.0,	0.82, 0.42, 0.12,	0.0, 0.0,	0.0, 0.0, 1.0,	// 60 low-left back
		+0.85,	+0.3,  +0.0,	0.82, 0.42, 0.12,	0.1, 0.0,	0.0, 0.0, 1.0,	// 61 top-left back
		+0.55,	+0.3,  +0.0,	0.82, 0.42, 0.12,	0.1, 1.0,	0.0, 0.0, 1.0,	// 62 top-right back
		+0.55,	+0.0,  +0.0,	0.82, 0.42, 0.12,	0.0, 1.0,	0.0, 0.0, 1.0,	// 63 low-right back

		+0.85,	 0.0,	+2.5,	0.82, 0.42, 0.12,	0.0, 1.0,	0.0, 0.0, 1.0,	// 64 low-right front
		+0.85,	+0.3,	+2.5,	0.82, 0.42, 0.12,	0.1, 1.0,	0.0, 0.0, 1.0,	// 65 top-right front
		+0.55,	 0.0,	+2.5,	0.82, 0.42, 0.12,	0.0, 0.0,	0.0, 0.0, 1.0,	// 66 low-right front
		+0.55,	+0.3,	+2.5,	0.82, 0.42, 0.12,	0.1, 0.0,	0.0, 0.0, 1.0,	// 67 top-right front
		+0.55,	 0.0,	+0.0,	0.82, 0.42, 0.12,	0.0, 1.0,	0.0, 0.0, 1.0,	// 68 low-right front
		+0.55,	+0.3,	+0.0,	0.82, 0.42, 0.12,	0.1, 1.0,	0.0, 0.0, 1.0,	// 69 top-right front

		/* Divider 7 */
		// Vertex				// Color			// Texture	// Normal Z
		//X     Y		Z		//R    G     B		S    T		A	 B	  C	
		+1.55,	 0.0,  +0.0,	0.82, 0.42, 0.12,	0.0, 0.0,	0.0, 0.0, 1.0,	// 70 low-left back
		+1.55,	+0.3,  +0.0,	0.82, 0.42, 0.12,	0.1, 0.0,	0.0, 0.0, 1.0,	// 71 top-left back
		+1.25,	+0.3,  +0.0,	0.82, 0.42, 0.12,	0.1, 1.0,	0.0, 0.0, 1.0,	// 72 top-right back
		+1.25,	+0.0,  +0.0,	0.82, 0.42, 0.12,	0.0, 1.0,	0.0, 0.0, 1.0,	// 73 low-right back

		+1.55,	 0.0,	+2.5,	0.82, 0.42, 0.12,	0.0, 1.0,	0.0, 0.0, 1.0,	// 74 low-right front
		+1.55,	+0.3,	+2.5,	0.82, 0.42, 0.12,	0.1, 1.0,	0.0, 0.0, 1.0,	// 75 top-right front
		+1.25,	 0.0,	+2.5,	0.82, 0.42, 0.12,	0.0, 0.0,	0.0, 0.0, 1.0,	// 76 low-right front
		+1.25,	+0.3,	+2.5,	0.82, 0.42, 0.12,	0.1, 0.0,	0.0, 0.0, 1.0,	// 77 top-right front
		+1.25,	 0.0,	+0.0,	0.82, 0.42, 0.12,	0.0, 1.0,	0.0, 0.0, 1.0,	// 78 low-right front
		+1.25,	+0.3,	+0.0,	0.82, 0.42, 0.12,	0.1, 1.0,	0.0, 0.0, 1.0,	// 79 top-right front

		/* Divider 8 */
		// Vertex				// Color			// Texture	// Normal Z
		//X     Y		Z		//R    G     B		S    T		A	 B	  C
		+2.25,	 0.0,  +0.0,	0.82, 0.42, 0.12,	0.0, 0.0,	0.0, 0.0, 1.0,	// 80 low-left back
		+2.25,	+0.3,  +0.0,	0.82, 0.42, 0.12,	0.1, 0.0,	0.0, 0.0, 1.0,	// 81 top-left back
		+1.95,	+0.3,  +0.0,	0.82, 0.42, 0.12,	0.1, 1.0,	0.0, 0.0, 1.0,	// 82 top-right back
		+1.95,	+0.0,  +0.0,	0.82, 0.42, 0.12,	0.0, 1.0,	0.0, 0.0, 1.0,	// 83 low-right back

		+2.25,	 0.0,	+2.5,	0.82, 0.42, 0.12,	0.0, 1.0,	0.0, 0.0, 1.0,	// 84 low-right front
		+2.25,	+0.3,	+2.5,	0.82, 0.42, 0.12,	0.1, 1.0,	0.0, 0.0, 1.0,	// 85 top-right front
		+1.95,	 0.0,	+2.5,	0.82, 0.42, 0.12,	0.0, 0.0,	0.0, 0.0, 1.0,	// 86 low-right front
		+1.95,	+0.3,	+2.5,	0.82, 0.42, 0.12,	0.1, 0.0,	0.0, 0.0, 1.0,	// 87 top-right front
		+1.95,	 0.0,	+0.0,	0.82, 0.42, 0.12,	0.0, 1.0,	0.0, 0.0, 1.0,	// 88 low-right front
		+1.95,	+0.3,	+0.0,	0.82, 0.42, 0.12,	0.1, 1.0,	0.0, 0.0, 1.0,	// 89 top-right front

		/* Divider 9 */
		// Vertex				// Color			// Texture	// Normal Z
		//X     Y		Z		//R    G     B		S    T		A	 B	  C
		+2.95,	 0.0,  +0.0,	0.82, 0.42, 0.12,	0.0, 0.0,	0.0, 0.0, 1.0,	// 90 low-left back
		+2.95,	+0.3,  +0.0,	0.82, 0.42, 0.12,	0.1, 0.0,	0.0, 0.0, 1.0,	// 91 top-left back
		+2.65,	+0.3,  +0.0,	0.82, 0.42, 0.12,	0.1, 1.0,	0.0, 0.0, 1.0,	// 92 top-right back
		+2.65,	+0.0,  +0.0,	0.82, 0.42, 0.12,	0.0, 1.0,	0.0, 0.0, 1.0,	// 93 low-right back

		+2.95,	 0.0,	+2.5,	0.82, 0.42, 0.12,	0.0, 1.0,	0.0, 0.0, 1.0,	// 94 low-right front
		+2.95,	+0.3,	+2.5,	0.82, 0.42, 0.12,	0.1, 1.0,	0.0, 0.0, 1.0,	// 95 top-right front
		+2.65,	 0.0,	+2.5,	0.82, 0.42, 0.12,	0.0, 0.0,	0.0, 0.0, 1.0,	// 96 low-right front
		+2.65,	+0.3,	+2.5,	0.82, 0.42, 0.12,	0.1, 0.0,	0.0, 0.0, 1.0,	// 97 top-right front
		+2.65,	 0.0,	+0.0,	0.82, 0.42, 0.12,	0.0, 1.0,	0.0, 0.0, 1.0,	// 98 low-right front
		+2.65,	+0.3,	+0.0,	0.82, 0.42, 0.12,	0.1, 1.0,	0.0, 0.0, 1.0,	// 99 top-right front

		/* Right Block */
		// Vertex				// Color			// Texture	// Normal Z
		//X     Y		Z		//R    G     B		S    T		A	 B	  C
		+5.0,	 0.0,	+2.5,	0.82, 0.42, 0.12,	0.0, 0.0,	0.0, 0.0, 1.0,	// 100 low-right left
		+5.0,	+0.3,	+2.5,	0.82, 0.42, 0.12,	0.1, 0.0,	0.0, 0.0, 1.0,	// 101 top-right left
		+3.35,	 0.0,	+2.5,	0.82, 0.42, 0.12,	0.0, 0.0,	0.0, 0.0, 1.0,	// 102 low-right front
		+3.35,	+0.3,	+2.5,	0.82, 0.42, 0.12,	0.1, 0.0,	0.0, 0.0, 1.0,	// 103 top-right front
		+3.35,	 0.0,	0.0,	0.82, 0.42, 0.12,	0.0, 1.0,	0.0, 0.0, 1.0,	// 104 low-right front
		+3.35,	+0.3,	0.0,	0.82, 0.42, 0.12,	0.1, 1.0,	0.0, 0.0, 1.0,	// 105 top-right front

		// Fixed front wall
		+5.0,	+0.3,	+2.5,	0.82, 0.42, 0.12,	0.1, 1.0,	0.0, 0.0, 1.0,	// 106 top-right front
		+3.35,	0.0,	+2.5,	0.82, 0.42, 0.12,	0.0, 0.0,	0.0, 0.0, 1.0,	// 107 low-left front
		+3.35,	+0.3,	+2.5,	0.82, 0.42, 0.12,	0.1, 0.0,	0.0, 0.0, 1.0,	// 108 top-left front
		+5.0,	0.0,	+2.5,	0.82, 0.42, 0.12,	0.0, 1.0,	0.0, 0.0, 1.0,	// 109 low-right left
	};

	GLubyte combI[] = {
		/* Comb */
		0,		1,		2,			2,		3,		0,			// back wall

		// Left Block
		0,		4,		5,			5,		1,		0,			// left
		4,		5,		6,			6,		7,		5,			// front
		6,		7,		9,			9,		8,		6,			// right

		// Divider 1
		10,		14,		15,			15,		11,		10,			// left
		14,		15,		16,			16,		17,		15,			// front
		16,		17,		19,			19,		18,		16,			// right

		// Divider 2
		20,		24,		25,			25,		21,		20,			// left
		24,		25,		26,			26,		27,		25,			// front
		26,		27,		29,			29,		28,		26,			// right

		// Divider 3
		30,		34,		35,			35,		31,		30,			// left
		34,		35,		36,			36,		37,		35,			// front
		36,		37,		39,			39,		38,		36,			// right

		// Divider 4
		40,		44,		45,			45,		41,		40,			// left
		44,		45,		46,			46,		47,		45,			// front
		46,		47,		49,			49,		48,		46,			// right

		// Divider 5
		50,		54,		55,			55,		51,		50,			// left
		54,		55,		56,			56,		57,		55,			// front
		56,		57,		59,			59,		58,		56,			// right

		// Divider 6
		60,		64,		65,			65,		61,		60,			// left
		64,		65,		66,			66,		67,		65,			// front
		66,		67,		69,			69,		68,		66,			// right

		// Divider 7
		70,		74,		75,			75,		71,		70,			// left
		74,		75,		76,			76,		77,		75,			// front
		76,		77,		79,			79,		78,		76,			// right

		// Divider 8
		80,		84,		85,			85,		81,		80,			// left
		84,		85,		86,			86,		87,		85,			// front
		86,		87,		89,			89,		88,		86,			// right

		// Divider 9
		90,		94,		95,			95,		91,		90,			// left
		94,		95,		96,			96,		97,		95,			// front
		96,		97,		99,			99,		98,		96,			// right

		// Right Block
		2,		3,		101,		101,	100,	3,			// left
		107,	108,	106,		106,	109,	107,		// front
		102,	103,	105,		105,	104,	102,		// right
	};

	GLfloat lampV[] = {
		// Vertex
		//X     Y		Z
		-0.5,	-0.5,	0.0,		// index 0
		-0.5,	0.5,	0.0,		// index 1
		0.5,	-0.5,	0.0,		// index 2		
		0.5,	0.5,	0.0,		// index 3	
	};

	GLubyte lampI[] = {
		0, 1, 2,
		1, 2, 3
	};

	/* Lamp Transforms */
	glm::vec3 lampPlanePositions[] = {
		glm::vec3(0.0f,  0.0f,  0.5f),
		glm::vec3(0.5f,  0.0f,  0.0f),
		glm::vec3(0.0f,  0.0f,  -0.5f),
		glm::vec3(-0.5f, 0.0f,  0.0f),
		glm::vec3(0.0f, 0.5f,  0.0f),
		glm::vec3(0.0f, -0.5f,  0.0f)
	};

	glm::float32 lampPlaneRotations[] = {
		0.0f, 90.0f, 180.0f, -90.0f, -90.f, 90.f
	};

	// Enable Depth Buffer
	glEnable(GL_DEPTH_TEST);

	/* Setup VBO, EBO, and VAO for objects */
	GLuint reedVBO, reedEBO, reedVAO;
	GLuint combVBO, combEBO, combVAO;
	GLuint coverVBO, coverEBO, coverVAO;
	GLuint lampVBO, lampEBO, lampVAO;

	// Reed Buffers
	glGenBuffers(1, &reedVBO);
	glGenBuffers(1, &reedEBO);

	// Comb Buffers
	glGenBuffers(1, &combVBO);
	glGenBuffers(1, &combEBO);

	// Cover Buffers
	glGenBuffers(1, &coverVBO);
	glGenBuffers(1, &coverEBO);

	// Light Buffers
	glGenBuffers(1, &lampVBO);
	glGenBuffers(1, &lampEBO);

	// Vertex Arrays
	glGenVertexArrays(1, &reedVAO);
	glGenVertexArrays(1, &combVAO);
	glGenVertexArrays(1, &coverVAO);
	glGenVertexArrays(1, &lampVAO);

	/* Reed VAO */
	glBindVertexArray(reedVAO); // Bind Reed VAO

	glBindBuffer(GL_ARRAY_BUFFER, reedVBO); // Select VBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, reedEBO); // Select EBO

	glBufferData(GL_ARRAY_BUFFER, sizeof(reedV), reedV, GL_STATIC_DRAW); // Load vertex attributes
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(reedI), reedI, GL_STATIC_DRAW); // Load indices

	// Specify attribute location and layout to GPU
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(8 * sizeof(GLfloat)));
	glEnableVertexAttribArray(3);

	glBindVertexArray(0); // Unbind Reed VAO

	/* Comb VAO */
	glBindVertexArray(combVAO); // Bind Comb VAO

	glBindBuffer(GL_ARRAY_BUFFER, combVBO); // Select VBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, combEBO); // Select EBO

	glBufferData(GL_ARRAY_BUFFER, sizeof(combV), combV, GL_STATIC_DRAW); // Load vertex attributes
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(combI), combI, GL_STATIC_DRAW); // Load indices

	// Specify attribute location and layout to GPU
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(8 * sizeof(GLfloat)));
	glEnableVertexAttribArray(3);

	glBindVertexArray(0); // Unbind Comb VAO

	/* Cover VAO */
	glBindVertexArray(coverVAO); // Bind Cover VAO

	glBindBuffer(GL_ARRAY_BUFFER, coverVBO); // Select VBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, coverEBO); // Select EBO

	glBufferData(GL_ARRAY_BUFFER, sizeof(coverV), coverV, GL_STATIC_DRAW); // Load vertex attributes
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(coverI), coverI, GL_STATIC_DRAW); // Load indices

	// Specify attribute location and layout to GPU
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(8 * sizeof(GLfloat)));
	glEnableVertexAttribArray(3);

	glBindVertexArray(0); // Unbind Cover VAO

	/* Lamp VAO */
	glBindVertexArray(lampVAO); // Bind Lamp VAO

	glBindBuffer(GL_ARRAY_BUFFER, lampVBO); // Select VBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lampEBO); // Select EBO

	glBufferData(GL_ARRAY_BUFFER, sizeof(lampV), lampV, GL_STATIC_DRAW); // Load vertex attributes
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(lampI), lampI, GL_STATIC_DRAW); // Load indices 

	// Specify attribute location and layout to GPU
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0); // unbind Lamp VAO

	/* Load Textures */
	int reedTexW, reedTexH, combTexW, combTexH, coverTexW, coverTexH;
	unsigned char* reedImage = SOIL_load_image("brass1024.jpg", &reedTexW, &reedTexH, 0, SOIL_LOAD_RGB);	// reed
	unsigned char* combImage = SOIL_load_image("burl2.jpg", &combTexW, &combTexH, 0, SOIL_LOAD_RGB);		// comb
	unsigned char* coverImage = SOIL_load_image("silver.jpg", &coverTexW, &coverTexH, 0, SOIL_LOAD_RGB);	// cover

	// Reed Texture
	GLuint reedTexture;
	glGenTextures(1, &reedTexture);
	glBindTexture(GL_TEXTURE_2D, reedTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, reedTexW, reedTexH, 0, GL_RGB, GL_UNSIGNED_BYTE, reedImage);
	glGenerateMipmap(GL_TEXTURE_2D);
	SOIL_free_image_data(reedImage);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Comb Texture
	GLuint combTexture;
	glGenTextures(1, &combTexture);
	glBindTexture(GL_TEXTURE_2D, combTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, combTexW, combTexH, 0, GL_RGB, GL_UNSIGNED_BYTE, combImage);
	glGenerateMipmap(GL_TEXTURE_2D);
	SOIL_free_image_data(combImage);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Cover Texture
	GLuint coverTexture;
	glGenTextures(1, &coverTexture);
	glBindTexture(GL_TEXTURE_2D, coverTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, coverTexW, coverTexH, 0, GL_RGB, GL_UNSIGNED_BYTE, coverImage);
	glGenerateMipmap(GL_TEXTURE_2D);
	SOIL_free_image_data(coverImage);
	glBindTexture(GL_TEXTURE_2D, 0);

	/* Shader source code */
	// Vertex shader source code
	string vertexShaderSource =
		"#version 330 core\n"
		"layout(location = 0) in vec3 vPosition;"
		"layout(location = 1) in vec3 aColor;"
		"layout(location = 2) in vec2 texCoord;"
		"layout(location = 3) in vec3 normal;"
		"out vec3 oColor;"
		"out vec2 oTexCoord;"
		"out vec3 oNormal;"
		"out vec3 FragPos;"
		"uniform mat4 model;"
		"uniform mat4 view;"
		"uniform mat4 projection;"
		"void main()\n"
		"{\n"
		"gl_Position = projection * view * model * vec4(vPosition.x, vPosition.y, vPosition.z, 1.0f);"
		"oColor = aColor;"
		"oTexCoord = texCoord;"
		"oNormal = mat3(transpose(inverse(model))) * normal;" // handles non-uniform scaling
		"FragPos = vec3(model * vec4(vPosition, 1.0f));"
		"}\n";

	// Fragment shader source code
	string fragmentShaderSource =
		"#version 330 core\n"
		"in vec3 oColor;"
		"in vec2 oTexCoord;"
		"in vec3 oNormal;"
		"in vec3 FragPos;"
		"out vec4 fragColor;"
		"uniform sampler2D myTexture;"
		"uniform vec3 objectColor;"
		"uniform vec3 light1Color;"
		"uniform vec3 light1Pos;"
		"uniform vec3 light2Color;"
		"uniform vec3 light2Pos;"
		"uniform vec3 light3Color;"
		"uniform vec3 light3Pos;"
		"uniform vec3 viewPos;"
		"void main()\n"
		"{\n"
		"float ambientStrength = 0.5f;" // Ambient
		"vec3 ambient = ambientStrength * light1Color * light2Color * light3Color;"
		"vec3 norm = normalize(oNormal);" // Diffuse
		"vec3 light1Dir = normalize(light1Pos - FragPos);"
		"vec3 light2Dir = normalize(light2Pos - FragPos);"
		"vec3 light3Dir = normalize(light3Pos - FragPos);"
		"float light1Diff = max(dot(norm, light1Dir), 0.0);"
		"float light2Diff = max(dot(norm, light2Dir), 0.0);"
		"float light3Diff = max(dot(norm, light3Dir), 0.0);"		
		"vec3 light1Diffuse = light1Diff * 1.0 * light1Color;"
		"vec3 light2Diffuse = light2Diff * 0.2 * light2Color;"
		"vec3 light3Diffuse = light3Diff * 0.2 * light3Color;"
		"vec3 fullDiffuse = light1Diffuse + light2Diffuse + light3Diffuse;"
		"float light1SpecStr = 1.5f;" // Specularity
		"float light2SpecStr = 0.5f;"
		"float light3SpecStr = 0.5f;"
		"vec3 viewDir = normalize(viewPos - FragPos);"
		"vec3 light1ReflectDir = reflect(-light1Dir, norm);"
		"vec3 light2ReflectDir = reflect(-light2Dir, norm);"
		"vec3 light3ReflectDir = reflect(-light3Dir, norm);"
		"float light1Spec = pow(max(dot(viewDir, light1ReflectDir), 0.0), 32);"
		"float light2Spec = pow(max(dot(viewDir, light2ReflectDir), 0.0), 32);"
		"float light3Spec = pow(max(dot(viewDir, light3ReflectDir), 0.0), 32);"
		"vec3 light1Specular = light1SpecStr * light1Spec * light1Color;"
		"vec3 light2Specular = light2SpecStr * light2Spec * light2Color;"
		"vec3 light3Specular = light3SpecStr * light3Spec * light3Color;"
		"vec3 fullSpecular = light1Specular + light2Specular + light3Specular;"
		"vec3 result = (ambient + fullDiffuse + fullSpecular) * objectColor;"
		"fragColor = texture(myTexture, oTexCoord) * vec4(result, 1.0f);"
		"}\n";

	// Lamp Vertex shader source code
	string lampVertexShaderSource =
		"#version 330 core\n"
		"layout(location = 0) in vec3 vPosition;"
		"uniform mat4 model;"
		"uniform mat4 view;"
		"uniform mat4 projection;"
		"void main()\n"
		"{\n"
		"gl_Position = projection * view * model * vec4(vPosition.x, vPosition.y, vPosition.z, 1.0f);"
		"}\n";

	// Lamp Fragment shader source code
	string lampFragmentShaderSource =
		"#version 330 core\n"
		"out vec4 fragColor;"
		"void main()\n"
		"{\n"
		"fragColor = vec4(1.0f);"
		"}\n";

	// Creating Shader Programs
	GLuint shaderProgram = CreateShaderProgram(vertexShaderSource, fragmentShaderSource);
	GLuint lampShaderProgram = CreateShaderProgram(lampVertexShaderSource, lampFragmentShaderSource);

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		// Set Delta Time
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Toggle Wireframe mode
		if (wireFrame) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		// Resize window and graphics simultaneously
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);

		/* Render here */
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		/* START PRIMARY SHADER PROGRAM */
		// Use Shader Program exe and select VAO before drawing 
		glUseProgram(shaderProgram); // Call Shader per-frame when updating attributes

		// Declare identity matrix
		glm::mat4 projectionMatrix; // view
		glm::mat4 modelMatrix; // object				

		// Setup views and projections
		if (ortho) {
			GLfloat oWidth = (GLfloat)width * 0.01f; // 10% of width
			GLfloat oHeight = (GLfloat)height * 0.01f; // 10% of height

			viewMatrix = glm::lookAt(cameraPosition, target, -worldUp);			
			projectionMatrix = glm::ortho(-oWidth, oWidth, oHeight, -oHeight, 0.1f, 100.0f);
		} else {
			viewMatrix = glm::lookAt(cameraPosition, target, worldUp);
			projectionMatrix = glm::perspective(fov, (GLfloat)width / (GLfloat)height, 0.1f, 100.0f);
		}

		// Select uniform variable and shader
		GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
		GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
		GLuint projectionLoc = glGetUniformLocation(shaderProgram, "projection");

		// Get light and object color, and light position location
		GLint objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor");
		GLint light1ColorLoc = glGetUniformLocation(shaderProgram, "light1Color");
		GLint light1PosLoc = glGetUniformLocation(shaderProgram, "light1Pos");
		GLint light2ColorLoc = glGetUniformLocation(shaderProgram, "light2Color");
		GLint light2PosLoc = glGetUniformLocation(shaderProgram, "light2Pos");
		GLint light3ColorLoc = glGetUniformLocation(shaderProgram, "light3Color");
		GLint light3PosLoc = glGetUniformLocation(shaderProgram, "light3Pos");
		GLint viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");

		// Assign Light and Object Colors
		glUniform3f(objectColorLoc, 1.0f, 1.0f, 1.0f);
		glUniform3f(light1ColorLoc, 0.0f, 0.0f, 1.0f); // Primary blue light
		glUniform3f(light2ColorLoc, 1.0f, 0.0f, 0.5f); // Secondary purple light
		glUniform3f(light3ColorLoc, 1.0f, 0.0f, 0.5f); // Secondary purple light

		// Set light positions
		glUniform3f(light1PosLoc, lamp1Position.x, lamp1Position.y, lamp1Position.z);
		glUniform3f(light2PosLoc, lamp2Position.x, lamp2Position.y, lamp2Position.z);
		glUniform3f(light3PosLoc, lamp3Position.x, lamp3Position.y, lamp3Position.z);

		// Set view position
		glUniform3f(viewPosLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

		// Pass transform to Shader
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
		glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

		/* DRAW REED */
		glBindVertexArray(reedVAO); // User-defined VAO must be called before draw.	

		glBindTexture(GL_TEXTURE_2D, reedTexture);
		
		// Draw primitive(s)
		for (int i = 0; i < 2; ++i) {		
			
			// Rotate the second model on Z to create a complete object
			if (i == 1) {
				modelMatrix = glm::rotate(modelMatrix, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			}
			
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
			
			draw(sizeof(reedI));
		}

		glBindVertexArray(0); // Unbind reed

		/* DRAW COVER */
		glBindVertexArray(coverVAO); // User-defined VAO must be called before draw.

		glBindTexture(GL_TEXTURE_2D, coverTexture);

		// Draw primitive(s)
		for (int i = 0; i < 2; ++i) {

			// Rotate the second model on Z to create a complete object
			if (i == 1) {
				modelMatrix = glm::rotate(modelMatrix, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			}

			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
			
			draw(sizeof(coverI));
		}

		glBindVertexArray(0); // Unbind cover

		/* DRAW COMB */
		glBindVertexArray(combVAO); // User-defined VAO must be called before draw.		

		glBindTexture(GL_TEXTURE_2D, combTexture);

		// Draw primitive(s)
		for (int i = 0; i < 2; ++i) {

			// Rotate the second model on Z to create a complete object
			if (i == 1) {
				modelMatrix = glm::rotate(modelMatrix, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			}

			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));			

			draw(sizeof(combI));
		}
		
		glBindVertexArray(0); // Unbind comb
		
		glUseProgram(0); // Incase different shader will be used after

		/* LAUNCH LIGHT SHADER PROGRAM */
		glUseProgram(lampShaderProgram);

		// Get matrix's uniform location and set matrix
		GLint lampModelLoc = glGetUniformLocation(lampShaderProgram, "model");
		GLint lampViewLoc = glGetUniformLocation(lampShaderProgram, "view");
		GLint lampProjLoc = glGetUniformLocation(lampShaderProgram, "projection");

		glUniformMatrix4fv(lampViewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
		glUniformMatrix4fv(lampProjLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

		/* DRAW LAMPS */
		if (lightDraw) {
			glBindVertexArray(lampVAO); // User-defined VAO must be called before draw.		

			// Transform planes to form cube
			for (GLuint i = 0; i < 6; i++) {
				glm::mat4 modelMatrix;
				modelMatrix = glm::translate(modelMatrix, lampPlanePositions[i] / glm::vec3(8.0f, 8.0f, 8.0f) + lamp1Position);
				modelMatrix = glm::rotate(modelMatrix, glm::radians(lampPlaneRotations[i]), glm::vec3(0.0f, 1.0f, 0.0f));
				modelMatrix = glm::scale(modelMatrix, glm::vec3(.125f, .125f, .125f));
				if (i >= 4)
					modelMatrix = glm::rotate(modelMatrix, glm::radians(lampPlaneRotations[i]), glm::vec3(1.0f, 0.0f, 0.0f));
				glUniformMatrix4fv(lampModelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));

				// Draw primitive(s)
				draw(sizeof(lampI));
			}

			// Transform planes to form cube
			for (GLuint i = 0; i < 6; i++) {
				glm::mat4 modelMatrix;
				modelMatrix = glm::translate(modelMatrix, lampPlanePositions[i] / glm::vec3(8.0f, 8.0f, 8.0f) + lamp2Position);
				modelMatrix = glm::rotate(modelMatrix, glm::radians(lampPlaneRotations[i]), glm::vec3(0.0f, 1.0f, 0.0f));
				modelMatrix = glm::scale(modelMatrix, glm::vec3(.125f, .125f, .125f));
				if (i >= 4)
					modelMatrix = glm::rotate(modelMatrix, glm::radians(lampPlaneRotations[i]), glm::vec3(1.0f, 0.0f, 0.0f));
				glUniformMatrix4fv(lampModelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));

				// Draw primitive(s)
				draw(sizeof(lampI));
			}

			// Transform planes to form cube
			for (GLuint i = 0; i < 6; i++) {
				glm::mat4 modelMatrix;
				modelMatrix = glm::translate(modelMatrix, lampPlanePositions[i] / glm::vec3(8.0f, 8.0f, 8.0f) + lamp3Position);
				modelMatrix = glm::rotate(modelMatrix, glm::radians(lampPlaneRotations[i]), glm::vec3(0.0f, 1.0f, 0.0f));
				modelMatrix = glm::scale(modelMatrix, glm::vec3(.125f, .125f, .125f));
				if (i >= 4)
					modelMatrix = glm::rotate(modelMatrix, glm::radians(lampPlaneRotations[i]), glm::vec3(1.0f, 0.0f, 0.0f));
				glUniformMatrix4fv(lampModelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));

				// Draw primitive(s)
				draw(sizeof(lampI));
			}

			glBindVertexArray(0); // Unbind lamp
		}

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();

		// Poll Camera Transformation
		TransformCamera();
	}

	/* MAINTENANCE BEFORE SHUTDOWN */
	glDeleteVertexArrays(1, &reedVAO);
	glDeleteBuffers(1, &reedVBO);
	glDeleteBuffers(1, &reedEBO);

	glDeleteVertexArrays(1, &combVAO);
	glDeleteBuffers(1, &combVBO);
	glDeleteBuffers(1, &combEBO);

	glDeleteVertexArrays(1, &coverVAO);
	glDeleteBuffers(1, &coverVBO);
	glDeleteBuffers(1, &coverEBO);

	glDeleteVertexArrays(1, &lampVAO);
	glDeleteBuffers(1, &lampVBO);
	glDeleteBuffers(1, &lampEBO);

	glfwTerminate();
	return 0;
}

/* Define Input callback functions */
// Keyboard input callback
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS) {
		keys[key] = true;

		// Toggle wireframe mode
		if (key == GLFW_KEY_SPACE) {
			wireFrame = !wireFrame;
		}

		// Toggle orthographic projection
		if (key == GLFW_KEY_O) {
			ortho = !ortho;
		}

		// Toggle light draw
		if (key == GLFW_KEY_L) {
			lightDraw = !lightDraw;
		}
	} else if (action == GLFW_RELEASE) {
		keys[key] = false;
	}
}

// Scrollwheel input callback
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	// only scroll if not in ortho mode
	if (!ortho) {
		if (fov >= FOV_MIN && fov <= FOV_MAX) {
			fov -= yoffset * SCROLL_SPEED;
		}

		// clamp FOV
		if (fov < FOV_MIN) {
			fov = FOV_MIN;
		}

		if (fov > FOV_MAX) {
			fov = FOV_MAX;
		}
	}
}

// Mouse position callback
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
	if (firstMouseMove) {
		lastX = xpos;
		lastY = ypos;
		firstMouseMove = false;
	}

	// Calculate cursor offset
	xChange = xpos - lastX;
	yChange = lastY - ypos; // inverted Y axis

	lastX = xpos;
	lastY = ypos;

	// Orbit camera
	if (isOrbiting) {
		if (ortho) {
			rawYaw -= xChange;
		} else {
			rawYaw += xChange;
		}
		rawPitch += yChange;

		degYaw = glm::radians(rawYaw);
		degPitch = glm::clamp(glm::radians(rawPitch), -glm::pi<float>() / 2.0f + 0.1f, glm::pi<float>() / 2.0f - 0.1f);

		// Azimuth Altitude Formula
		cameraPosition.x = target.x + radius * cosf(degPitch) * sinf(degYaw);
		cameraPosition.y = target.y + radius * sinf(degPitch);
		cameraPosition.z = target.z + radius * cosf(degPitch) * cosf(degYaw);
	}
}

// Mouse button callback
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if (action == GLFW_PRESS) {
		mouseButton[button] = true;
	}
	else if (action == GLFW_RELEASE) {
		mouseButton[button] = false;
	}
}

// Define transform camera function
void TransformCamera() {
	// Orbit camera
	if (keys[GLFW_KEY_LEFT_ALT] && mouseButton[GLFW_MOUSE_BUTTON_LEFT]) {
		isOrbiting = true;
	}
	else {
		isOrbiting = false;
	}

	// Reset camera
	if (keys[GLFW_KEY_F]) {
		initCamera();
	}
}

// Define initcamera function
void initCamera() {
	// Reset camera attributes
	cameraPosition = glm::vec3(0.0f, 0.0f, 10.0f);
	target = glm::vec3(0.0f, 0.0f, 0.0f);
	cameraDirection = glm::normalize(cameraPosition - target);
	worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
	cameraRight = glm::normalize(glm::cross(worldUp, cameraDirection));
	cameraUp = glm::normalize(glm::cross(cameraDirection, cameraRight));	
	fov = 45.0f;
	rawPitch = 0.0f;
	rawYaw = 0.0f;
}