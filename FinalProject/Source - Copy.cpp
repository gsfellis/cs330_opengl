// Gregory S. Fellis
// CS-330 | SNHU | 5-2 Milestone 2
// 8/7/2018
/* Description:
Creates a perspective OpenGL view and renders
a 3D low-poly harmonica. The user can orbit
around the object by using the controls listed below.

WARNING: Orthographic mode flips camera for some reason.

WARNING: Don't hover over glfwCreateWindow function!!!!!
Long function descriptions cause VS2017 to lock up.
*/
/* Controls:
	F:			Resets view to starting position
	O:			Toggles orthographic viewing
	P:			Toggles panning mode
	Space:		Toggles wireframe mode

	ALT + Left Mouse Button:	Orbits the camera, clamped at +-90degrees
	ALT + Middle Mouse Button:	Pans the camera (if enabled)
	Scroll Wheel:				Zooms the camera in and out (changes FOV)
*/

#include <GLEW/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>

// GLM libraries
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

/* Constants */
const GLfloat FOV_MAX = 46.0f;
const GLfloat FOV_MIN = 44.25f;
const GLfloat SCROLL_SPEED = 0.05f;

/* Global Variables */
int width, height;			// Screen dimensions

bool keys[1024];			// Key press array
bool mouseButton[3];		// Mouse button press array
bool isPanning = false;		// Sets mouse movement to panning
bool panEnabled = false;	// Disables panning ability
bool isOrbiting = false;	// Sets mouse movement to orbiting
bool wireFrame = false;		// Toggles wireframe mode
bool firstMouseMove = true;	// Detect initial mouse movement
bool ortho = false;			// Sets orthographic projection

glm::mat4 viewMatrix;		// Declare View Matrix

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
GLfloat lastX = 0.0f;		// last X mouse position
GLfloat lastY = 0.0f;		// last Y mouse position

// Define camera attributes
glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, 10.0f);
glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 cameraDirection = glm::normalize(cameraPosition - target);
glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 cameraRight = glm::normalize(glm::cross(worldUp, cameraDirection));
glm::vec3 cameraUp = glm::normalize(glm::cross(cameraDirection, cameraRight));
glm::vec3 cameraFront = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f));

/* Input Callback prototypes */
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

/* Camera transformation prototypes */
void TransformCamera();
void initCamera();
glm::vec3 getTarget();

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

	GLfloat vertices[] = {
		/* Comb */
		// Height	= 0.6cm
		// Width	= 10cm
		// Depth	= 2.5cm
		// color : 0.82, 0.42, 0.12, chocolate brown

		// Vertex				// Color
		//X     Y		Z		//R    G     B
		-5.0,	 0.0,	0.0,	0.82, 0.42, 0.12, //  0 low-left back
		-5.0,	+0.3,	0.0,	0.82, 0.42, 0.12, //  1 top-left back
		+5.0,	+0.3,	0.0,	0.82, 0.42, 0.12, //  2 top-right back
		+5.0,	 0.0,	0.0,	0.82, 0.42, 0.12, //  3 low-right back

		-5.0,	 0.0,  +2.5,	0.82, 0.42, 0.12, //  4 low-right left
		-5.0,	+0.3,  +2.5,	0.82, 0.42, 0.12, //  5 top-right left

		+5.0,	 0.0,  +2.5,	0.82, 0.42, 0.12, //  6 low-left right
		+5.0,	+0.3,  +2.5,	0.82, 0.42, 0.12, //  7 top-left right

		/* Top Reed */
		// Height	= 0.1cm
		// Width	= 10cm
		// Depth	= 2.5cm
		// color : 0.80, 0.65, 0.20, bronze

		// Vertex				// Color
		//X     Y		Z		//R    G     B
		-5.0,	+0.3,	0.0,	0.80, 0.65, 0.20, //  8 low-left back
		-5.0,	+0.4,	0.0,	1.00, 0.85, 0.40, //  9 top-left back
		+5.0,	+0.4,	0.0,	1.00, 0.85, 0.40, // 10 top-right back
		+5.0,	+0.3,	0.0,	0.80, 0.65, 0.20, // 11 low-right back

		-5.0,	+0.3,  +2.5,	0.80, 0.65, 0.20, // 12 low-right left
		-5.0,	+0.4,  +2.5,	1.00, 0.85, 0.40, // 13 top-right left

		+5.0,	+0.3,  +2.5,	0.80, 0.65, 0.20, // 14 low-left right
		+5.0,	+0.4,  +2.5,	1.00, 0.85, 0.40, // 15 top-left right

		/* Bottom Reed */
		// Height	= 0.1cm
		// Width	= 10cm
		// Depth	= 2.5cm
		// color : 0.80, 0.65, 0.20, bronze

		// Vertex				// Color
		//X     Y		Z		//R    G     B
		-5.0,	-0.4,	0.0,	1.00, 0.85, 0.40, // 16 low-left back
		-5.0,	-0.3,	0.0,	0.80, 0.65, 0.20, // 17 top-left back
		+5.0,	-0.3,	0.0,	0.80, 0.65, 0.20, // 18 top-right back
		+5.0,	-0.4,	0.0,	1.00, 0.85, 0.40, // 19 low-right back

		-5.0,	-0.4,  +2.5,	1.00, 0.85, 0.40, // 20 low-right left
		-5.0,	-0.3,  +2.5,	0.80, 0.65, 0.20, // 21 top-right left

		+5.0,	-0.4,  +2.5,	1.00, 0.85, 0.40, // 22 low-left right
		+5.0,	-0.3,  +2.5,	0.80, 0.65, 0.20, // 23 top-left right

		/* Top Cover Plate */
		// Height	= 0.5cm
		// Width	= 8.2cm (not counting flaps)
		// Depth	= 2.4cm
		// color : 0.65, 0.65, 0.85, silver

		// Vertex				// Color
		//X     Y		Z		//R    G     B
		-4.1,	+0.4,	0.0,	0.65, 0.65, 0.85, // 24 low-left back
		-4.1,	+0.9,	0.0,	0.85, 0.85, 1.00, // 25 top-left back
		+4.1,	+0.9,	0.0,	0.85, 0.85, 1.00, // 26 top-right back
		+4.1,	+0.4,	0.0,	0.65, 0.65, 0.85, // 27 low-right back

		-4.1,	+0.4,  +2.45,	0.65, 0.65, 0.85, // 28 low-right left
		-4.1,	+0.9,  +2.00,	0.85, 0.85, 1.00, // 29 top-right left

		+4.1,	+0.4,  +2.45,	0.65, 0.65, 0.85, // 30 low-left right
		+4.1,	+0.9,  +2.00,	0.85, 0.85, 1.00, // 31 top-left right

		/* Bottom Cover Plate */
		// Height	= 0.5cm
		// Width	= 8.2cm (not counting flaps)
		// Depth	= 2.4cm
		// color : 0.65, 0.65, 0.85, silver

		// Vertex				// Color
		//X     Y		Z		//R    G     B
		-4.1,	-0.9,	0.0,	0.85, 0.85, 1.00, // 32 low-left back
		-4.1,	-0.4,	0.0,	0.65, 0.65, 0.85, // 33 top-left back
		+4.1,	-0.4,	0.0,	0.65, 0.65, 0.85, // 34 top-right back
		+4.1,	-0.9,	0.0,	0.85, 0.85, 1.00, // 35 low-right back

		-4.1,	-0.9,  +2.00,	0.85, 0.85, 1.00, // 36 low-right left
		-4.1,	-0.4,  +2.45,	0.65, 0.65, 0.85, // 37 top-right left

		+4.1,	-0.9,  +2.00,	0.85, 0.85, 1.00, // 38 low-left right
		+4.1,	-0.4,  +2.45,	0.65, 0.65, 0.85, // 39 top-left right

		/* Top Cover Plate v2 */
		// Height	= 0.5cm
		// Width	= 8.2cm (not counting flaps)
		// Depth	= 2.4cm
		// color : 0.65, 0.65, 0.85, silver
		
		/* Strip 1 */
		// Vertex				// Color
		//X     Y		Z		//R    G     B		
		-4.1,	+0.40,	+0.3,	0.65, 0.65, 0.85, // 40 low-left back 
		-4.1,	+0.525, +0.3,	0.65, 0.65, 0.85, // 41 top-left back 
		+4.1,	+0.525, +0.3,	0.65, 0.65, 0.85, // 42 top-right back 
		+4.1,	+0.40,	+0.3,	0.65, 0.65, 0.85, // 43 low-right back 
		-4.1,	+0.40,  +2.45,	0.65, 0.65, 0.85, // 44 low-right left 
		-4.1,	+0.525,	+2.39,	0.65, 0.65, 0.85, // 45 top-right left 
		+4.1,	+0.40,  +2.45,	0.65, 0.65, 0.85, // 46 low-left right 
		+4.1,	+0.525, +2.39,	0.65, 0.65, 0.85, // 47 top-left right 

		/* Strip 2 */
		// Vertex				// Color
		//X     Y		Z		//R    G     B		
		-4.1,	+0.525, +0.3,	0.65, 0.65, 0.85, // 48 low-left back 
		-4.1,	+0.65,	+0.3,	0.65, 0.65, 0.85, // 49 top-left back 
		+4.1,	+0.65,	+0.3,	0.65, 0.65, 0.85, // 50 top-right back 
		+4.1,	+0.525,	+0.3,	0.65, 0.65, 0.85, // 51 low-right back 
		-4.1,	+0.525, +2.39,	0.65, 0.65, 0.85, // 52 low-right left 
		-4.1,	+0.65,  +2.30,	0.65, 0.65, 0.85, // 53 top-right left 
		+4.1,	+0.525, +2.39,	0.65, 0.65, 0.85, // 54 low-left right 
		+4.1,	+0.65,  +2.30,	0.65, 0.65, 0.85, // 55 top-left right 

		/* Strip 3 */
		// Vertex				// Color
		//X     Y		Z		//R    G     B		
		-4.1,	+0.65,	+0.3,	0.65, 0.65, 0.85, // 56 low-left back 
		-4.1,	+0.775,	+0.3,	0.65, 0.65, 0.85, // 47 top-left back 
		+4.1,	+0.775,	+0.3,	0.65, 0.65, 0.85, // 58 top-right back 
		+4.1,	+0.65,	+0.3,	0.65, 0.65, 0.85, // 59 low-right back 
		-4.1,	+0.65,  +2.30,	0.65, 0.65, 0.85, // 60 low-right left 
		-4.1,	+0.775, +2.175,	0.65, 0.65, 0.85, // 61 top-right left 
		+4.1,	+0.65,  +2.30,	0.65, 0.65, 0.85, // 62 low-left right 
		+4.1,	+0.775, +2.175,	0.65, 0.65, 0.85, // 63 top-left right 

		/* Strip 4 */
		// Vertex				// Color
		//X     Y		Z		//R    G     B		
		-4.1,	+0.775,	+0.3,	0.65, 0.65, 0.85, // 64 low-left back 
		-4.1,	+0.80,	+0.3,	0.65, 0.65, 0.85, // 65 top-left back 
		+4.1,	+0.80,	+0.3,	0.65, 0.65, 0.85, // 66 top-right back 
		+4.1,	+0.775,	+0.3,	0.65, 0.65, 0.85, // 67 low-right back 
		-4.1,	+0.775, +2.175,	0.65, 0.65, 0.85, // 68 low-right left 
		-4.1,	+0.90,  +2.0,	0.85, 0.85, 1.00, // 69 top-right left 
		+4.1,	+0.775, +2.175,	0.65, 0.65, 0.85, // 70 low-left right 
		+4.1,	+0.90,  +2.0,	0.85, 0.85, 1.00, // 71 top-left right

		/* Back Fin */
		-3.9,	+1.00,	+0.0,	0.85, 0.85, 1.00, // 72 top-left back slope
		+3.9,	+1.00,	+0.0,	0.85, 0.85, 1.00, // 73 top-right back slope
		+3.7,	+1.00,	+0.0,	0.65, 0.65, 0.85, // 74 top-right back
		-3.9,	+1.00,	+0.0,	0.65, 0.65, 0.85, // 75 top-left back
		+3.7,	+0.60,	+0.0,	0.85, 0.85, 1.00, // 76 bottom-right back
		-3.9,	+0.60,	+0.0,	0.85, 0.85, 1.00, // 77 bottom-left back

		/* Comb */
		// Height	= 0.6cm
		// Width	= 10cm
		// Depth	= 2.5cm
		// color : 0.82, 0.42, 0.12, chocolate brown

		// Vertex				// Color
		//X     Y		Z		//R    G     B
		-5.0,	 0.0,	0.0,	0.20, 0.20, 0.20, //  78 low-left back
		-5.0,	+0.3,	0.0,	0.20, 0.20, 0.20, //  79 top-left back
		+5.0,	+0.3,	0.0,	0.20, 0.20, 0.20, //  80 top-right back
		+5.0,	 0.0,	0.0,	0.20, 0.20, 0.20, //  81 low-right back

		-5.0,	 0.0,	+2.5,	0.82, 0.42, 0.12, //  82 low-right left
		-5.0,	+0.3,	+2.5,	0.82, 0.42, 0.12, //  83 top-right left
		-3.35,	 0.0,	+2.5,	0.82, 0.42, 0.12, //  84 low-right front
		-3.35,	+0.3,	+2.5,	0.82, 0.42, 0.12, //  85 top-right front
		-3.35,	 0.0,	0.0,	0.82, 0.42, 0.12, //  86 low-right front
		-3.35,	+0.3,	0.0,	0.82, 0.42, 0.12, //  87 top-right front

		/* Divider 1 */
		-2.95,	 0.0,  +0.0,	0.82, 0.42, 0.12, //  88 low-left back
		-2.95,	+0.3,  +0.0,	0.82, 0.42, 0.12, //  89 top-left back
		-2.65,	+0.3,  +0.0,	0.82, 0.42, 0.12, //  90 top-right back
		-2.65,	+0.0,  +0.0,	0.82, 0.42, 0.12, //  91 low-right back

		-2.95,	 0.0,	+2.5,	0.82, 0.42, 0.12, //  92 low-right front
		-2.95,	+0.3,	+2.5,	0.82, 0.42, 0.12, //  93 top-right front
		-2.65,	 0.0,	+2.5,	0.82, 0.42, 0.12, //  94 low-right front
		-2.65,	+0.3,	+2.5,	0.82, 0.42, 0.12, //  95 top-right front
		-2.65,	 0.0,	+0.0,	0.82, 0.42, 0.12, //  96 low-right front
		-2.65,	+0.3,	+0.0,	0.82, 0.42, 0.12, //  97 top-right front

		/* Divider 2 */
		-2.25,	 0.0,  +0.0,	0.82, 0.42, 0.12, //  98 low-left back
		-2.25,	+0.3,  +0.0,	0.82, 0.42, 0.12, //  99 top-left back
		-1.95,	+0.3,  +0.0,	0.82, 0.42, 0.12, //  100 top-right back
		-1.95,	+0.0,  +0.0,	0.82, 0.42, 0.12, //  101 low-right back

		-2.25,	 0.0,	+2.5,	0.82, 0.42, 0.12, //  102 low-right front
		-2.25,	+0.3,	+2.5,	0.82, 0.42, 0.12, //  103 top-right front
		-1.95,	 0.0,	+2.5,	0.82, 0.42, 0.12, //  104 low-right front
		-1.95,	+0.3,	+2.5,	0.82, 0.42, 0.12, //  105 top-right front
		-1.95,	 0.0,	+0.0,	0.82, 0.42, 0.12, //  106 low-right front
		-1.95,	+0.3,	+0.0,	0.82, 0.42, 0.12, //  107 top-right front

		/* Divider 3 */
		-1.55,	 0.0,  +0.0,	0.82, 0.42, 0.12, //  108 low-left back
		-1.55,	+0.3,  +0.0,	0.82, 0.42, 0.12, //  109 top-left back
		-1.25,	+0.3,  +0.0,	0.82, 0.42, 0.12, //  110 top-right back
		-1.25,	+0.0,  +0.0,	0.82, 0.42, 0.12, //  111 low-right back

		-1.55,	 0.0,	+2.5,	0.82, 0.42, 0.12, //  112 low-right front
		-1.55,	+0.3,	+2.5,	0.82, 0.42, 0.12, //  113 top-right front
		-1.25,	 0.0,	+2.5,	0.82, 0.42, 0.12, //  114 low-right front
		-1.25,	+0.3,	+2.5,	0.82, 0.42, 0.12, //  115 top-right front
		-1.25,	 0.0,	+0.0,	0.82, 0.42, 0.12, //  116 low-right front
		-1.25,	+0.3,	+0.0,	0.82, 0.42, 0.12, //  117 top-right front

		/* Divider 4 */
		-0.85,	 0.0,  +0.0,	0.82, 0.42, 0.12, //  118 low-left back
		-0.85,	+0.3,  +0.0,	0.82, 0.42, 0.12, //  119 top-left back
		-0.55,	+0.3,  +0.0,	0.82, 0.42, 0.12, //  120 top-right back
		-0.55,	+0.0,  +0.0,	0.82, 0.42, 0.12, //  121 low-right back

		-0.85,	 0.0,	+2.5,	0.82, 0.42, 0.12, //  122 low-right front
		-0.85,	+0.3,	+2.5,	0.82, 0.42, 0.12, //  123 top-right front
		-0.55,	 0.0,	+2.5,	0.82, 0.42, 0.12, //  124 low-right front
		-0.55,	+0.3,	+2.5,	0.82, 0.42, 0.12, //  125 top-right front
		-0.55,	 0.0,	+0.0,	0.82, 0.42, 0.12, //  126 low-right front
		-0.55,	+0.3,	+0.0,	0.82, 0.42, 0.12, //  127 top-right front

		/* Divider 5 */
		-0.15,	 0.0,  +0.0,	0.82, 0.42, 0.12, //  128 low-left back
		-0.15,	+0.3,  +0.0,	0.82, 0.42, 0.12, //  129 top-left back
		+0.15,	+0.3,  +0.0,	0.82, 0.42, 0.12, //  130 top-right back
		+0.15,	+0.0,  +0.0,	0.82, 0.42, 0.12, //  131 low-right back

		-0.15,	 0.0,	+2.5,	0.82, 0.42, 0.12, //  132 low-right front
		-0.15,	+0.3,	+2.5,	0.82, 0.42, 0.12, //  133 top-right front
		+0.15,	 0.0,	+2.5,	0.82, 0.42, 0.12, //  134 low-right front
		+0.15,	+0.3,	+2.5,	0.82, 0.42, 0.12, //  135 top-right front
		+0.15,	 0.0,	+0.0,	0.82, 0.42, 0.12, //  136 low-right front
		+0.05,	+0.3,	+0.0,	0.82, 0.42, 0.12, //  137 top-right front

		/* Divider 6 */
		+0.85,	 0.0,  +0.0,	0.82, 0.42, 0.12, //  138 low-left back
		+0.85,	+0.3,  +0.0,	0.82, 0.42, 0.12, //  139 top-left back
		+0.55,	+0.3,  +0.0,	0.82, 0.42, 0.12, //  140 top-right back
		+0.55,	+0.0,  +0.0,	0.82, 0.42, 0.12, //  141 low-right back

		+0.85,	 0.0,	+2.5,	0.82, 0.42, 0.12, //  142 low-right front
		+0.85,	+0.3,	+2.5,	0.82, 0.42, 0.12, //  143 top-right front
		+0.55,	 0.0,	+2.5,	0.82, 0.42, 0.12, //  144 low-right front
		+0.55,	+0.3,	+2.5,	0.82, 0.42, 0.12, //  145 top-right front
		+0.55,	 0.0,	+0.0,	0.82, 0.42, 0.12, //  146 low-right front
		+0.55,	+0.3,	+0.0,	0.82, 0.42, 0.12, //  147 top-right front

		/* Divider 7 */
		+1.55,	 0.0,  +0.0,	0.82, 0.42, 0.12, //  148 low-left back
		+1.55,	+0.3,  +0.0,	0.82, 0.42, 0.12, //  149 top-left back
		+1.25,	+0.3,  +0.0,	0.82, 0.42, 0.12, //  150 top-right back
		+1.25,	+0.0,  +0.0,	0.82, 0.42, 0.12, //  151 low-right back

		+1.55,	 0.0,	+2.5,	0.82, 0.42, 0.12, //  152 low-right front
		+1.55,	+0.3,	+2.5,	0.82, 0.42, 0.12, //  153 top-right front
		+1.25,	 0.0,	+2.5,	0.82, 0.42, 0.12, //  154 low-right front
		+1.25,	+0.3,	+2.5,	0.82, 0.42, 0.12, //  155 top-right front
		+1.25,	 0.0,	+0.0,	0.82, 0.42, 0.12, //  156 low-right front
		+1.25,	+0.3,	+0.0,	0.82, 0.42, 0.12, //  157 top-right front

		/* Divider 8 */
		+2.25,	 0.0,  +0.0,	0.82, 0.42, 0.12, //  158 low-left back
		+2.25,	+0.3,  +0.0,	0.82, 0.42, 0.12, //  159 top-left back
		+1.95,	+0.3,  +0.0,	0.82, 0.42, 0.12, //  160 top-right back
		+1.95,	+0.0,  +0.0,	0.82, 0.42, 0.12, //  161 low-right back

		+2.25,	 0.0,	+2.5,	0.82, 0.42, 0.12, //  162 low-right front
		+2.25,	+0.3,	+2.5,	0.82, 0.42, 0.12, //  163 top-right front
		+1.95,	 0.0,	+2.5,	0.82, 0.42, 0.12, //  164 low-right front
		+1.95,	+0.3,	+2.5,	0.82, 0.42, 0.12, //  165 top-right front
		+1.95,	 0.0,	+0.0,	0.82, 0.42, 0.12, //  166 low-right front
		+1.95,	+0.3,	+0.0,	0.82, 0.42, 0.12, //  167 top-right front

		/* Divider 9 */
		+2.95,	 0.0,  +0.0,	0.82, 0.42, 0.12, //  168 low-left back
		+2.95,	+0.3,  +0.0,	0.82, 0.42, 0.12, //  169 top-left back
		+2.65,	+0.3,  +0.0,	0.82, 0.42, 0.12, //  170 top-right back
		+2.65,	+0.0,  +0.0,	0.82, 0.42, 0.12, //  171 low-right back

		+2.95,	 0.0,	+2.5,	0.82, 0.42, 0.12, //  172 low-right front
		+2.95,	+0.3,	+2.5,	0.82, 0.42, 0.12, //  173 top-right front
		+2.65,	 0.0,	+2.5,	0.82, 0.42, 0.12, //  174 low-right front
		+2.65,	+0.3,	+2.5,	0.82, 0.42, 0.12, //  175 top-right front
		+2.65,	 0.0,	+0.0,	0.82, 0.42, 0.12, //  176 low-right front
		+2.65,	+0.3,	+0.0,	0.82, 0.42, 0.12, //  177 top-right front

		/* Right Block */
		+5.0,	 0.0,	+2.5,	0.82, 0.42, 0.12, //  178 low-right left
		+5.0,	+0.3,	+2.5,	0.82, 0.42, 0.12, //  179 top-right left
		+3.35,	 0.0,	+2.5,	0.82, 0.42, 0.12, //  180 low-right front
		+3.35,	+0.3,	+2.5,	0.82, 0.42, 0.12, //  181 top-right front
		+3.35,	 0.0,	0.0,	0.82, 0.42, 0.12, //  182 low-right front
		+3.35,	+0.3,	0.0,	0.82, 0.42, 0.12, //  183 top-right front
	};

	GLubyte indices[] = {
		/* Comb */
		//0,	1,	2,		2,	3,	0,		// back
		//0,	4,	5,		5,	1,	0,		// left
		//3,	6,	7,		7,	2,	3,		// right
		//7,	6,	4,		4,	5,	7,		// front
		//0,	3,	4,		4,	6,	3,		// bottom
		//1,	2,	5,		5,	7,	2,		// top

		/* Top Reed */
		8,	9,	10,		10,	11,	8,		// back
		8,	12,	13,		13,	9,	8,		// left
		11,	14,	15,		15,	10,	11,		// right
		15,	14,	12,		12,	13,	15,		// front
		8,	11,	12,		12,	14,	11,		// bottom
		9,	10,	13,		13,	15,	10,		// top

		/*	Bottom Reed */
		/*
		16,	17,	18,		18,	19,	16,		// back
		16,	20,	21,		21,	17,	16,		// left
		19,	22,	23,		23,	18,	19,		// right
		23,	22,	20,		20,	21,	23,		// front
		16,	19,	20,		20,	22,	19,		// bottom
		17,	18,	21,		21,	23,	18,		// top
		*/
		/*	Top	Cover Plate */
		/*
		24,	25,	26,		26,	27,	24,	// back
		24,	28,	29,		29,	25,	24,		// left
		27,	30,	31,		31,	26,	27,		// right
		31,	30,	28,		28,	29,	31,		// front
		24,	27,	28,		28,	30,	27,	// bottom
		25,	26,	29,		29,	31,	26,		// top
		*/

		/*	Bottom	Cover Plate */
		/*
		//32,	33,	34,		34,	35,	32,	// back
		32,	36,	37,		37,	33,	32,		// left
		35,	38,	39,		39,	34,	35,		// right
		39,	38,	36,		36,	37,	39,		// front
		32,	35,	36,		36,	38,	35,		// bottom
		//33,	34,	37,		37,	39,	34,	// top
		*/

		/*	Top	Cover Plate v2 */		
		/* Strip 1*/
		40,	44,	45,		45,	41,	40,	// left
		43,	46,	47,		47,	42,	43,	// right
		47,	46,	44,		44,	45,	47,	// front

		/* Strip 2*/
		48,	52,	53,		53,	49,	48,
		51,	54,	55,		55,	50,	51,
		55,	54,	52,		52,	53,	55,

		/* Strip 3 */
		56,	60,	61,		61,	57,	56,
		59,	62,	63,		63,	58,	59,
		63,	62,	60,		60,	61,	63,

		/* Strip 4 */
		64,	68,	69,		69,	65,	64,
		67,	70,	71,		71,	66,	67,
		71,	70,	68,		68,	69,	71,

		/* Top */
		65,	66,	69,		69,	71,	66,	// top

		/* Back Fin */
		65, 72, 73,		73, 66, 65,
		74, 75, 76,		76, 75, 77,

		/* Comb */
		78, 79, 80,		80, 81, 78,	// back wall

		// Left Block
		78, 82, 83,		83, 79, 78, // left wall
		82, 83, 84,		84, 85,	83,	// front left wall
		84, 85, 87,		87, 86, 84,	// interior left wall

		// Divider 1
		//88, 89, 90,		90, 91, 88,	// back wall
		88,	92,	93,		93,	89,	88,
		92,	93,	94,		94,	95,	93,
		94,	95,	97,		97,	96,	94,

		// Divider 2
		//98,	99,	100,	100, 101, 98,
		98,	102,103,	103, 99, 98,
		102, 103, 104, 104, 105, 103,
		104, 105, 107, 107, 106, 104,

		// Divider 3
		//108, 109, 110, 110, 111, 108,
		108, 112, 113, 113, 109, 108,
		112, 113, 114, 114, 115, 113,
		114, 115, 117, 117, 116, 114,

		// Divider 4
		//118, 119, 120, 120, 121, 118,
		118, 122, 123, 123, 119, 118,
		122, 123, 124, 124, 125, 123,
		124, 125, 127, 127, 126, 124,

		// Divider 5
		//128, 129, 130, 130, 131, 128,
		128, 132, 133, 133, 129, 128,
		132, 133, 134, 134, 135, 133,
		134, 135, 137, 137, 136, 134,

		// Divider 6
		//138, 139, 140, 140, 141, 138,
		138, 142, 143, 143, 139, 138,
		142, 143, 144, 144, 145, 143,
		144, 145, 147, 147, 146, 144,

		// Divider 7
		//148, 149, 150, 150, 151, 148,
		148, 152, 153, 153, 149, 148,
		152, 153, 154, 154, 155, 153,
		154, 155, 157, 157, 156, 154,

		// Divider 8
		//158, 159, 160, 160, 161, 158,
		158, 162, 163, 163, 159, 158,
		162, 163, 164, 164, 165, 163,
		164, 165, 167, 167, 166, 164,

		// Divider 9
		//168, 169, 170, 170, 171, 168,
		168, 172, 173, 173, 169, 168,
		172, 173, 174, 174, 175, 173,
		174, 175, 177, 177, 176, 174,

		// Right Block
		80, 81, 179, 179, 178, 81,
		180, 181, 179, 179, 178, 180,
		180, 181, 183, 183, 182, 180,

	};

	// Plane positions
	glm::vec3 planePositions[] = {
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.5f, 0.0f, -0.5f),
		glm::vec3(0.0f, 0.0f, -1.0f),
		glm::vec3(-0.5f, 0.0f, -0.5f)
	};	
	
	// Plane rotations
	glm::float32 planeRotations[] = {
		180.0f, 180.0f
	};
	
	// Plane rotations Y
	glm::float32 planeRotationsY[] = {
		0.0f, -90.0f, 180.0f, 90.0f, 0.0f
	};

	// Plane rotations X
	glm::float32 planeRotationsX[] = {
		-30.0f, 30.0f, -30.0f, 30.0f, 0.0f
	};

	// Enable Depth Buffer
	glEnable(GL_DEPTH_TEST);

	GLuint VBO, EBO, VAO;

	glGenBuffers(1, &VBO); // Create VBO
	glGenBuffers(1, &EBO); // Create EBO

	glGenVertexArrays(1, &VAO); // Create VOA
	glBindVertexArray(VAO);

	// VBO and EBO Placed in User-Defined VAO
	glBindBuffer(GL_ARRAY_BUFFER, VBO); // Select VBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO); // Select EBO

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // Load vertex attributes
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); // Load indices 

	// Specify attribute location and layout to GPU
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0); // Unbind VOA or close off (Must call VOA explicitly in loop)

	// Vertex shader source code
	string vertexShaderSource =
		"#version 330 core\n"
		"layout(location = 0) in vec4 vPosition;"
		"layout(location = 1) in vec4 aColor;"
		"out vec4 oColor;"
		"uniform mat4 model;" // adds uniform model variable
		"uniform mat4 view;" // adds uniform view variable
		"uniform mat4 projection;" // adds uniform projection variable
		"void main()\n"
		"{\n"
		"gl_Position = projection * view * model * vPosition;" // must be this order
		"oColor = aColor;"
		"}\n";

	// Fragment shader source code
	string fragmentShaderSource =
		"#version 330 core\n"
		"in vec4 oColor;"
		"out vec4 fragColor;"
		"void main()\n"
		"{\n"
		"fragColor = oColor;"
		"}\n";

	// Creating Shader Program
	GLuint shaderProgram = CreateShaderProgram(vertexShaderSource, fragmentShaderSource);

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

		// Use Shader Program exe and select VAO before drawing 
		glUseProgram(shaderProgram); // Call Shader per-frame when updating attributes

		// Declare identity matrix
		glm::mat4 projectionMatrix; // view
		glm::mat4 modelMatrix; // object
				

		// Setup views and projections
		if (ortho) {
			// TODO: ortho inverts camera for some reason
			GLfloat oWidth = (GLfloat)width * 0.01f; // 10% of width
			GLfloat oHeight = (GLfloat)height * 0.01f; // 10% of height

			viewMatrix = glm::lookAt(cameraPosition, getTarget(), worldUp);			
			projectionMatrix = glm::ortho(-oWidth, oWidth, oHeight, -oHeight, 0.1f, 100.0f);
			
		} else {
			//modelMatrix = glm::rotate(modelMatrix, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			viewMatrix = glm::lookAt(cameraPosition, getTarget(), worldUp);
			projectionMatrix = glm::perspective(fov, (GLfloat)width / (GLfloat)height, 0.1f, 100.0f);			
		}

		// Select uniform variable and shader
		GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
		GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
		GLuint projectionLoc = glGetUniformLocation(shaderProgram, "projection");

		// Pass transform to Shader
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
		glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

		glBindVertexArray(VAO); // User-defined VAO must be called before draw.				
		
		// Draw primitive(s)
		for (int i = 0; i < 2; ++i) {		
			
			// Rotate the second model on Z to create a complete object
			if (i == 1) {
				modelMatrix = glm::rotate(modelMatrix, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			}
			
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
			draw(sizeof(indices));
		}		

		// Unbind Shader exe and VOA after drawing per frame
		glBindVertexArray(0); //Incase different VAO wii be used after
		glUseProgram(0); // Incase different shader will be used after

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();

		// Poll Camera Transformation
		TransformCamera();
	}

	//Clear GPU resources
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);

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

		// Toggle panning mode
		if (key == GLFW_KEY_P) {
			panEnabled = !panEnabled;
		}
	}
	else if (action == GLFW_RELEASE) {
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
	if ((isPanning && panEnabled) || (isOrbiting)) {
		if (firstMouseMove) {
			lastX = xpos;
			lastY = ypos;
			firstMouseMove = false;
		}

		// Calculate cursor offset
		xChange = xpos - lastX;
		if (ortho) {
			yChange = ypos - lastY;
		}
		else {
			yChange = lastY - ypos; // inverted Y axis
		}

		lastX = xpos;
		lastY = ypos;
	}

	// Pan camera
	if (isPanning && panEnabled) {
		GLfloat cameraSpeed = xChange * deltaTime;
		cameraPosition += cameraSpeed * cameraRight;

		cameraSpeed = yChange * deltaTime;
		cameraPosition += cameraSpeed * cameraUp;
	}

	// Orbit camera
	if (isOrbiting) {
		rawYaw += xChange;
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

// Define getTarget function
glm::vec3 getTarget() {
	if (isPanning && panEnabled) {
		target = cameraPosition + cameraFront;
	}

	return target;
}

// Define transform camera function
void TransformCamera() {
	// Pan camera
	if (keys[GLFW_KEY_LEFT_ALT] && mouseButton[GLFW_MOUSE_BUTTON_MIDDLE]) {
		isPanning = true;
	}
	else {
		isPanning = false;
	}

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
	cameraFront = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f));
	firstMouseMove = true;
	rawYaw = 0.0f;
	rawPitch = 0.0f;
	fov = 45.0f;
}