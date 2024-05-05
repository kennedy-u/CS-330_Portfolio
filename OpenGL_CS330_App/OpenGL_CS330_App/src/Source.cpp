////////////////////////////////////
// 
// Final Project: 3D Scene
// Kennedy Uzoho
// CS-330 Comp Graphics and Visualization
// 
////////////////////////////////////


#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <GLAD/glad.h>      // GLAD library
#include <GLFW/glfw3.h>     // GLFW library
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>      // Image loading Utility functions

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//includes
#include <meshes.h>
#include <camera.h>

using namespace std; // Standard namespace 

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
	const char* const WINDOW_TITLE = "Kennedy Uzoho's Final Project"; // Macro for window title

	// Variables for window width and height
	const int WINDOW_WIDTH = 800;
	const int WINDOW_HEIGHT = 600;

	// Stores the GL data relative to a given mesh
	struct GLMesh
	{
		GLuint vao;         // Handle for the vertex array object
		GLuint vbos[2];     // Handles for the vertex buffer objects
		GLuint nIndices;    // Number of indices of the mesh
	};

	// Main GLFW window
	GLFWwindow* gWindow = nullptr;

	// Triangle mesh data
	GLMesh gPlaneMesh;
	GLMesh gPyramid4Mesh;

	// Mesh data
	Meshes meshes;

	// Texture id
	GLuint tabletextureId;
	GLuint mactextureId;
	GLuint macbacktextureId;
	GLuint macapplewhitetexId;
	GLuint mousetextureId;
	GLuint cuptextureId;

	glm::vec2 gUVScale(2.0f, 3.0f);
	GLint gTexWrapMode = GL_REPEAT;
	
	// Shader program
	GLuint gProgramId;
	GLuint gLampProgramId;
	

	// camera
	Camera gCamera(glm::vec3(0.0f, 2.0f, 2.0f));
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;
	float angle = 0.0;
	bool usePerspective = true;
	bool useOrthographic = false;
	float cameraSpeed = 2.5f;

	// timing
	float gDeltaTime = 0.0f; // time between current frame and last frame
	float gLastFrame = 0.0f;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
/* Surface Vertex Shader Source Code*/
const GLchar* surfaceVertexShaderSource = GLSL(440,

layout(location = 0) in vec3 vertexPosition; // VAP position 0 for vertex position data
layout(location = 1) in vec3 vertexNormal; // VAP position 1 for normals
layout(location = 2) in vec2 textureCoordinate;

out vec3 vertexFragmentNormal; // For outgoing normals to fragment shader
out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
out vec2 vertexTextureCoordinate;

//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	gl_Position = projection * view * model * vec4(vertexPosition, 1.0f); // Transforms vertices into clip coordinates

	vertexFragmentPos = vec3(model * vec4(vertexPosition, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)

	vertexFragmentNormal = mat3(transpose(inverse(model))) * vertexNormal; // get normal vectors in world space only and exclude normal translation properties
	vertexTextureCoordinate = textureCoordinate;
}
);

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Surface Fragment Shader Source Code*/
const GLchar* surfaceFragmentShaderSource = GLSL(440,

in vec3 vertexFragmentNormal; // For incoming normals
in vec3 vertexFragmentPos; // For incoming fragment position
in vec2 vertexTextureCoordinate;

out vec4 fragmentColor; // For outgoing cube color to the GPU

// Uniform / Global variables for object color, light color, light position, and camera/view position
uniform vec4 objectColor;
uniform vec3 ambientColor;
uniform vec3 light1Color;
uniform vec3 light1Position;
uniform vec3 light2Color;
uniform vec3 light2Position;
uniform vec3 viewPosition;
uniform sampler2D uTexture; // Useful when working with multiple textures
uniform vec2 uvScale;
uniform bool ubHasTexture;
uniform float ambientStrength = 0.1f; // Set ambient or global lighting strength
uniform float specularIntensity1 = 0.8f;
uniform float highlightSize1 = 15.0f;
uniform float specularIntensity2 = 0.8f;
uniform float highlightSize2 = 15.0f;

void main()
	{
		/*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

		//Calculate Ambient lighting
		vec3 ambient = ambientStrength * ambientColor; // Generate ambient light color

		//**Calculate Diffuse lighting**
		vec3 norm = normalize(vertexFragmentNormal); // Normalize vectors to 1 unit
		vec3 light1Direction = normalize(light1Position - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
		float impact1 = max(dot(norm, light1Direction), 0.0);// Calculate diffuse impact by generating dot product of normal and light
		vec3 diffuse1 = impact1 * light1Color; // Generate diffuse light color
		vec3 light2Direction = normalize(light2Position - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
		float impact2 = max(dot(norm, light2Direction), 0.0);// Calculate diffuse impact by generating dot product of normal and light
		vec3 diffuse2 = impact2 * light2Color; // Generate diffuse light color

		//**Calculate Specular lighting**
		vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
		vec3 reflectDir1 = reflect(-light1Direction, norm);// Calculate reflection vector
		//Calculate specular component
		float specularComponent1 = pow(max(dot(viewDir, reflectDir1), 0.0), highlightSize1);
		vec3 specular1 = specularIntensity1 * specularComponent1 * light1Color;
		vec3 reflectDir2 = reflect(-light2Direction, norm);// Calculate reflection vector
		//Calculate specular component
		float specularComponent2 = pow(max(dot(viewDir, reflectDir2), 0.0), highlightSize2);
		vec3 specular2 = specularIntensity2 * specularComponent2 * light2Color;

		//**Calculate phong result**
		//Texture holds the color to be used for all three components
		vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);
		vec3 phong1;
		vec3 phong2;

		if (ubHasTexture == true)
			{
				phong1 = (ambient + diffuse1 + specular1) * textureColor.xyz;
				phong2 = (ambient + diffuse2 + specular2) * textureColor.xyz;
			}
		else
			{
				phong1 = (ambient + diffuse1 + specular1) * objectColor.xyz;
				phong2 = (ambient + diffuse2 + specular2) * objectColor.xyz;
			}

		fragmentColor = vec4(phong1 + phong2, 1.0); // Send lighting results to GPU
		//fragmentColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	}
);

/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////

/* Light Object Vertex Shader Source Code*/
const GLchar* lampVertexShaderSource = GLSL(440,

	layout(location = 0) in vec3 aPos;  // VAP position 0 for vertex position data

	//Uniform / Global variables for the  transform matrices
	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 projection;

void main()
	{
		gl_Position = projection * view * model * vec4(aPos, 1.0);	// Transforms vertices into clip coordinates
	}
);

/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////

/* Light Object Fragment Shader Source Code*/
const GLchar* lampFragmentShaderSource = GLSL(440,

	out vec4 FragColor;				// For outgoing lamp color (smaller cube) to the GPU

void main()
	{
		FragColor = vec4(1.0);		// Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
	}
);

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
//void UCreateMesh(GLMesh &mesh);
//void UDestroyMesh(GLMesh &mesh);
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);

// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
	for (int j = 0; j < height / 2; ++j)
	{
		int index1 = j * width * channels;
		int index2 = (height - 1 - j) * width * channels;

		for (int i = width * channels; i > 0; --i)
		{
			unsigned char tmp = image[index1];
			image[index1] = image[index2];
			image[index2] = tmp;
			++index1;
			++index2;
		}
	}
}


// main function. Entry point to the OpenGL program
int main(int argc, char* argv[])
{
	if (!UInitialize(argc, argv, &gWindow))
		return EXIT_FAILURE;

	// Create the mesh
	meshes.CreateMeshes();

	// Create the shader program
	if (!UCreateShaderProgram(surfaceVertexShaderSource, surfaceFragmentShaderSource, gProgramId))
		return EXIT_FAILURE;
	// Create the shader program
	if (!UCreateShaderProgram(lampVertexShaderSource, lampFragmentShaderSource, gLampProgramId))
		return EXIT_FAILURE;

	// Load texture
	const char* tableTexFilename = "C:\\Users\\Kenyk\\Desktop\\SNHUCompsci\\CS-330_Portfolio\\OpenGL_CS330_App\\resources\\tabletexture1.jpg";
	const char* macTexFilename = "C:\\Users\\Kenyk\\Desktop\\SNHUCompsci\\CS-330_Portfolio\\OpenGL_CS330_App\\resources\\mactexture.jpg";
	const char* macbackTexFilename = "C:\\Users\\Kenyk\\Desktop\\SNHUCompsci\\CS-330_Portfolio\\OpenGL_CS330_App\\resources\\macbacktexture.jpg";
	const char* macapplewhiteTexFilename = "C:\\Users\\Kenyk\\Desktop\\SNHUCompsci\\CS-330_Portfolio\\OpenGL_CS330_App\\resources\\macapplewhitetex.jpg";
	const char* mouseTexFilename = "C:\\Users\\Kenyk\\Desktop\\SNHUCompsci\\CS-330_Portfolio\\OpenGL_CS330_App\\resources\\mousetexture.jpg";
	const char* cupTexFilename = "C:\\Users\\Kenyk\\Desktop\\SNHUCompsci\\CS-330_Portfolio\\OpenGL_CS330_App\\resources\\cuptexture.jpg";


	if (!UCreateTexture(tableTexFilename, tabletextureId))
	{
		cout << "Failed to load texture " << tableTexFilename << endl;
		return EXIT_FAILURE;
	}

	if (!UCreateTexture(macTexFilename, mactextureId))
	{
		cout << "Failed to load texture " << macTexFilename << endl;
		return EXIT_FAILURE;
	}

	if (!UCreateTexture(macbackTexFilename, macbacktextureId))
	{
		cout << "Failed to load texture " << macbackTexFilename << endl;
		return EXIT_FAILURE;
	}

	if (!UCreateTexture(macapplewhiteTexFilename, macapplewhitetexId))
	{
		cout << "Failed to load texture " << macapplewhiteTexFilename << endl;
		return EXIT_FAILURE;
	}

	if (!UCreateTexture(mouseTexFilename, mousetextureId))
	{
		cout << "Failed to load texture " << mouseTexFilename << endl;
		return EXIT_FAILURE;
	}
	if (!UCreateTexture(cupTexFilename, cuptextureId))
	{
		cout << "Failed to load texture " << cupTexFilename << endl;
		return EXIT_FAILURE;
	}
	
	// tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
	glUseProgram(gProgramId);

	// We set the texture as texture unit 0
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"),  0);
	glUniform2f(glGetUniformLocation(gProgramId, "uvScale"), gUVScale.x, gUVScale.y);

	// Sets the background color of the window to black (it will be implicitely used by glClear)
	glClearColor(0.3f, 0.2f, 0.1f, 1.0);

	/*gCamera.Position = glm::vec3(0.0f, 2.0f, 2.0f);
	gCamera.Front = glm::vec3(0.0, -1.0, -2.0f);
	gCamera.Up = glm::vec3(0.0, 1.0, 0.0);*/

	//
	// render loop
	// -----------
	while (!glfwWindowShouldClose(gWindow))
	{
		// per-frame timing
		// --------------------
		float currentFrame = glfwGetTime();
		gDeltaTime = currentFrame - gLastFrame;
		gLastFrame = currentFrame;

		// input
		// -----
		UProcessInput(gWindow);

		// Render this frame
		URender();

		glfwPollEvents();
	}

	// Release mesh data
	meshes.DestroyMeshes();

	// Release texture
	UDestroyTexture(tabletextureId);
	UDestroyTexture(mactextureId);
	UDestroyTexture(macbacktextureId);
	UDestroyTexture(macapplewhitetexId);
	UDestroyTexture(mousetextureId);
	UDestroyTexture(cuptextureId);

	// Release shader program
	UDestroyShaderProgram(gProgramId);
	UDestroyShaderProgram(gLampProgramId);

	exit(EXIT_SUCCESS); // Terminates the program successfully
}


// Initialize GLFW, GLAD, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
	// GLFW: initialize and configure
	// ------------------------------
	if (!glfwInit())
	{
		std::cout << "Failed to initialize GLFW" << std::endl;
		return false;
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// GLFW: window creation
	// ---------------------
	* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, nullptr, nullptr);
	if (*window == nullptr)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(*window);
	glfwSetFramebufferSizeCallback(*window, UResizeWindow);
	glfwSetCursorPosCallback(*window, UMousePositionCallback);
	glfwSetScrollCallback(*window, UMouseScrollCallback);
	glfwSetMouseButtonCallback(*window, UMouseButtonCallback);

	// tell GLFW to capture our mouse
	glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// GLAD: initialize
	// ----------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cerr << "Failed to initialize GLAD" << std::endl;
		return false;
	}

	// Display GPU OpenGL version
	std::cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << std::endl;

	return true;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		gCamera.ProcessKeyboard(LEFT, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		gCamera.ProcessKeyboard(RIGHT, gDeltaTime);

	float velocity = gCamera.MovementSpeed * gDeltaTime;
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		gCamera.Position -= gCamera.Up * velocity;
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		gCamera.Position += gCamera.Up * velocity;

	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
	{
		usePerspective = true;
		useOrthographic = false;
	}
	if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
	{
		usePerspective = false;
		useOrthographic = true;
	}

}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
	if (gFirstMouse)
	{
		gLastX = xpos;
		gLastY = ypos;
		gFirstMouse = false;
	}

	float xoffset = xpos - gLastX;
	float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top

	gLastX = xpos;
	gLastY = ypos;

	gCamera.ProcessMouseMovement(xoffset, yoffset);
}


// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	gCamera.ProcessMouseScroll(yoffset);
}


// glfw: handle mouse button events
// --------------------------------
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	switch (button)
	{
	case GLFW_MOUSE_BUTTON_LEFT:
	{
		if (action == GLFW_PRESS)
			cout << "Left mouse button pressed" << endl;
		else
			cout << "Left mouse button released" << endl;
	}
	break;

	case GLFW_MOUSE_BUTTON_MIDDLE:
	{
		if (action == GLFW_PRESS)
			cout << "Middle mouse button pressed" << endl;
		else
			cout << "Middle mouse button released" << endl;
	}
	break;

	case GLFW_MOUSE_BUTTON_RIGHT:
	{
		if (action == GLFW_PRESS)
			cout << "Right mouse button pressed" << endl;
		else
			cout << "Right mouse button released" << endl;
	}
	break;

	default:
		cout << "Unhandled mouse button event" << endl;
		break;
	}
}


// Functioned called to render a frame
void URender()
{

	// Enable z-depth
	glEnable(GL_DEPTH_TEST);

	// Clear the frame and z buffers
	glClearColor(0.3f, 0.2f, 0.1f, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Set the shader to be used
	glUseProgram(gProgramId);

	// camera/view transformation
	glm::mat4 view = gCamera.GetViewMatrix();

	// Creates a orthographic projection
	glm::mat4 orthoProjection = glm::ortho(-7.0f, 7.0f, -7.0f, 7.0f, 0.0f, 100.0f);

	// Creates a perspective projection
	glm::mat4 perspectiveProjection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

	// Create a perspective/ortho projection matrix based on the current projection mode
	glm::mat4 projection;
	if (usePerspective)
	{
		projection = perspectiveProjection;
	}
	else
	{
		projection = orthoProjection;
	}

	GLint modelLoc;
	GLint viewLoc;
	GLint projLoc;
	GLint viewPosLoc;
	GLint ambStrLoc;
	GLint ambColLoc;
	GLint light1ColLoc;
	GLint light1PosLoc;
	GLint light2ColLoc;
	GLint light2PosLoc;
	GLint objColLoc;
	GLint specInt1Loc;
	GLint highlghtSz1Loc;
	GLint specInt2Loc;
	GLint highlghtSz2Loc;
	GLint uHasTextureLoc;
	bool ubHasTextureVal;
	glm::mat4 scale;
	glm::mat4 rotation;
	glm::mat4 translation;
	glm::mat4 model;

	// Retrieves and passes transform matrices to the Shader program
	modelLoc = glGetUniformLocation(gProgramId, "model");
	viewLoc = glGetUniformLocation(gProgramId, "view");
	projLoc = glGetUniformLocation(gProgramId, "projection");
	viewPosLoc = glGetUniformLocation(gProgramId, "viewPosition");
	ambStrLoc = glGetUniformLocation(gProgramId, "ambientStrength");
	ambColLoc = glGetUniformLocation(gProgramId, "ambientColor");
	light1ColLoc = glGetUniformLocation(gProgramId, "light1Color");
	light1PosLoc = glGetUniformLocation(gProgramId, "light1Position");
	light2ColLoc = glGetUniformLocation(gProgramId, "light2Color");
	light2PosLoc = glGetUniformLocation(gProgramId, "light2Position");
	objColLoc = glGetUniformLocation(gProgramId, "objectColor");
	specInt1Loc = glGetUniformLocation(gProgramId, "specularIntensity1");
	highlghtSz1Loc = glGetUniformLocation(gProgramId, "highlightSize1");
	specInt2Loc = glGetUniformLocation(gProgramId, "specularIntensity2");
	highlghtSz2Loc = glGetUniformLocation(gProgramId, "highlightSize2");
	uHasTextureLoc = glGetUniformLocation(gProgramId, "ubHasTexture");


	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	//set the camera view location
	glUniform3f(viewPosLoc, gCamera.Position.x, gCamera.Position.y, gCamera.Position.z);
	//set ambient lighting strength
	glUniform1f(ambStrLoc, 0.4f);
	//set ambient color
	glUniform3f(ambColLoc,		0.2f, 0.2f,  0.2f);
	glUniform3f(light1ColLoc,   0.5f, 0.5f,  0.5f);
	glUniform3f(light1PosLoc,   7.0f, 2.8f, -1.0f);
	glUniform3f(light2ColLoc,   0.5f, 0.5f,  0.5f);
	glUniform3f(light2PosLoc,   2.0f, 1.0f, -1.0f);
	//set specular intensity
	glUniform1f(specInt1Loc, .8f);
	glUniform1f(specInt2Loc, .8f);
	//set specular highlight size
	glUniform1f(highlghtSz1Loc, 2.0f);
	glUniform1f(highlghtSz2Loc, 2.0f);

	ubHasTextureVal = true;
	glUniform1i(uHasTextureLoc, ubHasTextureVal);

	GLint UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
	

	//////////////////////////////////////////
	///   3D Scene- Main Objects Render    ///
	/////////////////////////////////////////

	//plane (table)
	//
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gPlaneMesh.vao);

	// Bind the texture to the corresponding texture unit
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tabletextureId);
	// Set the texture uniform in the shader
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(8.0f, 1.0f, 3.0f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(0.0f, 0.8f, 1.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniform4f(objColLoc, 0.5f, 0.5f, 0.5f, 1.0f);
	
	// Draws the triangles
	glDrawElements(GL_TRIANGLES, meshes.gPlaneMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);


	//Laptop Box
	//
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gBoxMesh.vao);

	// Bind the texture to the corresponding texture unit
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mactextureId);
	// Set the texture uniform in the shader
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(4.0f, 0.2f, 2.0f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(4.0f, 0.9f, 1.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));
	glUniform4f(objColLoc, 0.5f, 0.5f, 0.5f, 1.0f);

	// Draws the triangles
	glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);


	// back edge of laptop
	//
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gBoxMesh.vao);

	// Bind the texture to the corresponding texture unit
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, macbacktextureId);
	// Set the texture uniform in the shader
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(2.8f, 0.2f, 0.01f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(4.0f, 0.9f, 2.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));
	glUniform4f(objColLoc, 0.5f, 0.5f, 0.5f, 1.0f);

	// Draws the triangles
	glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);
	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	
	// laptop Logo sphere
	// 
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gSphereMesh.vao);

	// Bind the texture to the corresponding texture unit
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, macapplewhitetexId);
	// Set the texture uniform in the shader
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.3f, 0.006f, 0.2f));
	// 2. Rotate the object
	rotation = glm::rotate(-35.0f, glm::vec3(0.0, 1.0f, 0.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(4.0f, 1.0f, 1.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));
	glUniform4f(objColLoc, 0.5f, 0.5f, 0.5f, 1.0f);

	// Draws the triangles
	glDrawElements(GL_TRIANGLES, meshes.gSphereMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// laptop Logo1 sphere
	// 
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gSphereMesh.vao);

	// Bind the texture to the corresponding texture unit
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, macapplewhitetexId);
	// Set the texture uniform in the shader
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.3f, 0.006f, 0.2f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(0.0, 1.0f, 0.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(4.08f, 1.0f, 1.12f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));
	glUniform4f(objColLoc, 0.5f, 0.5f, 0.5f, 1.0f);

	// Draws the triangles
	glDrawElements(GL_TRIANGLES, meshes.gSphereMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	//laptop logo box
	// 
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gBoxMesh.vao);

	// Bind the texture to the corresponding texture unit
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, macapplewhitetexId);
	// Set the texture uniform in the shader
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.33f, 0.006f, 0.1f));
	// 2. Rotate the object
	rotation = glm::rotate(35.0f, glm::vec3(0.0, 1.0f, 0.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(3.8f, 1.0f, 1.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));
	glUniform4f(objColLoc, 0.5f, 0.5f, 0.5f, 1.0f);

	// Draws the triangles
	glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);


	// Mouse Sphere
	//
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gSphereMesh.vao);

	// Bind the texture to the corresponding texture unit
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mousetextureId);
	// Set the texture uniform in the shader
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.2f, 0.18f, 0.35f));
	// 2. Rotate the object
	rotation = glm::rotate(173.0f, glm::vec3(1.0, 0.0f, 0.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(1.5f, 0.94f, 1.5f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));
	glUniform4f(objColLoc, 0.5f, 0.5f, 0.5f, 1.0f);

	// Draws the triangles
	glDrawElements(GL_TRIANGLES, meshes.gSphereMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);


	// Mouse buttom Box
	//
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gBoxMesh.vao);

	// Bind the texture to the corresponding texture unit
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mousetextureId);
	// Set the texture uniform in the shader
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.3f, 0.22f, 0.35f));
	// 2. Rotate the object
	rotation = glm::rotate(170.0f, glm::vec3(1.0, 0.0f, 0.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(1.5f, 0.86f, 1.33f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));
	glUniform4f(objColLoc, 0.5f, 0.5f, 0.5f, 1.0f);

	// Draws the triangles
	glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);


	//Cup Cylinder Body
	// 
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gTaperedCylinderMesh.vao);

	// Bind the texture to the corresponding texture unit
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, cuptextureId);
	// Set the texture uniform in the shader
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.4f, -0.7f, 0.4f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(7.0f, 1.5f, 1.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));
	glUniform4f(objColLoc, 0.5f, 0.5f, 0.5f, 1.0f);

	// Draws the triangles
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
	glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// Cup Torus handle
	// 
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gTorusMesh.vao);

	// Bind the texture to the corresponding texture unit
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, cuptextureId);
	// Set the texture uniform in the shader
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.15f, 0.3f, 1.0f));
	// 2. Rotate the object
	rotation = glm::rotate(-0.65f, glm::vec3(0.0, 0.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(7.3f, 1.2f, 1.05f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));
	glUniform4f(objColLoc, 0.5f, 0.5f, 0.5f, 1.0f);

	// Draws the triangles
	glDrawArrays(GL_TRIANGLES, 0, meshes.gTorusMesh.nVertices);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// Cup Sphere/depth
	//
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gSphereMesh.vao);

	// Bind the texture to the corresponding texture unit
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mactextureId);
	// Set the texture uniform in the shader
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);
	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.38f,  0.05f, 0.4f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(0.0, 1.0f, 0.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(7.0f, 1.47f, 1.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));
	glUniform4f(objColLoc, 0.5f, 0.5f, 0.5f, 1.0f);

	// Draws the triangles
	glDrawElements(GL_TRIANGLES, meshes.gSphereMesh.nIndices, GL_UNSIGNED_INT, (void*)0);
	glBindVertexArray(0);

	// Light stands
	//
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gCylinderMesh.vao);

	// Bind the texture to the corresponding texture unit
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mousetextureId);
	// Set the texture uniform in the shader
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.2f, 0.8f, 0.2f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(1.0f, 1.0f, -1.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniform4f(objColLoc, 0.5f, 0.5f, 0.5f, 1.0f);

	// Draws the triangles
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
	glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.2f, 0.8f, 0.2f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(7.0f, 1.0f, -1.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniform4f(objColLoc, 0.5f, 0.5f, 0.5f, 1.0f);

	// Draws the triangles
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
	glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// Light Base
	//
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gBoxMesh.vao);

	// Bind the texture to the corresponding texture unit
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mousetextureId);
	// Set the texture uniform in the shader
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(1.0f, 0.14f, 0.8f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(1.0f, 1.0f, -1.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));
	glUniform4f(objColLoc, 0.5f, 0.5f, 0.5f, 1.0f);

	// Draws the triangles
	glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(1.0f, 0.14f, 0.8f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(7.0f, 1.0f, -1.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));
	glUniform4f(objColLoc, 0.5f, 0.5f, 0.5f, 1.0f);

	// Draws the triangles
	glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);
	// Deactivate the Vertex Array Object
	glBindVertexArray(0);


	////////////////////////////////////////////
	///   3D Scene- Lights Objects Render    ///
	////////////////////////////////////////////
	// Light Object
	glUseProgram(gLampProgramId);

	// Retrieves and passes transform matrices to the Shader program
	modelLoc = glGetUniformLocation(gLampProgramId, "model");
	viewLoc = glGetUniformLocation(gLampProgramId, "view");
	projLoc = glGetUniformLocation(gLampProgramId, "projection");

	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));


	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gPyramid4Mesh.vao);

	// 1. Scales the object by 2
	scale = glm::scale(glm::vec3(1.0f, 2.0f, 0.8f));
	// 2. Rotates shape by 15 degrees in the x axis
	rotation = glm::rotate(angle, glm::vec3(0.0, 1.0f, 0.0f));
	angle += 0.002;
	// 3. Place object at the origin
	translation = glm::translate(glm::vec3(1.0f, 2.8f, -1.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	

	glDrawArrays(GL_TRIANGLE_STRIP, 0, meshes.gPyramid4Mesh.nVertices);

	// 1. Scales the object by 2
	scale = glm::scale(glm::vec3(1.0f, 2.0f, 0.8f));
	// 2. Rotates shape by 15 degrees in the x axis
	rotation = glm::rotate(angle, glm::vec3(0.0, 1.0f, 0.0f));
	angle += 0.001;
	// 3. Place object at the origin
	translation = glm::translate(glm::vec3(7.0f, 2.8f, -1.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	

	glDrawArrays(GL_TRIANGLE_STRIP, 0, meshes.gPyramid4Mesh.nVertices);

	glBindVertexArray(0);

	glUseProgram(0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
	glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.
}


// Function to generate and load a texture from an image file
bool UCreateTexture(const char* filename, GLuint& textureId)
{
	int width, height, channels;
	stbi_set_flip_vertically_on_load(true); // Flip the image vertically during loading
	unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
	if (image)
	{
		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);

		// Set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		// Set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (channels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		else if (channels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			cout << "Not implemented to handle an image with " << channels << " channels" << endl;
			return false;
		}

		glGenerateMipmap(GL_TEXTURE_2D);

		// Clean up
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		return true;
	}

	// Error loading the image
	return false;
}

void UDestroyTexture(GLuint textureId)
{
	glGenTextures(1, &textureId);
}


// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint &programId)
{
	// Compilation and linkage error reporting
	int success = 0;
	char infoLog[512];

	// Create a Shader program object.
	programId = glCreateProgram();

	// Create the vertex and fragment shader objects
	GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

	// Retrive the shader source
	glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
	glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

	// Compile the vertex shader, and print compilation errors (if any)
	glCompileShader(vertexShaderId); // compile the vertex shader
	// check for shader compile errors
	glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

		return false;
	}

	glCompileShader(fragmentShaderId); // compile the fragment shader
	// check for shader compile errors
	glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

		return false;
	}

	// Attached compiled shaders to the shader program
	glAttachShader(programId, vertexShaderId);
	glAttachShader(programId, fragmentShaderId);

	glLinkProgram(programId);   // links the shader program
	// check for linking errors
	glGetProgramiv(programId, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

		return false;
	}

	glUseProgram(programId);    // Uses the shader program

	return true;
}


void UDestroyShaderProgram(GLuint programId)
{
	glDeleteProgram(programId);
}
