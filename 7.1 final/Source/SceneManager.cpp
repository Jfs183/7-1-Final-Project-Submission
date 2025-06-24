///////////////////////////////////////////////////////////////////////////////
// SceneManager.cpp
// ============
// Manage the loading and rendering of 3D scenes.
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//  Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>
#include <GLFW/glfw3.h>  //  MOUSE/KEYBOARD/GLFW FUNCTIONS
#include <iostream>      //  For debug output

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	stbi_set_flip_vertically_on_load(true);
	unsigned char* image = stbi_load(filename, &width, &height, &colorChannels, 0);

	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		glGenerateMipmap(GL_TEXTURE_2D);
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0);

		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;
	return false;
}
/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots. There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glDeleteTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}
/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.empty())
		return false;

	int index = 0;
	bool bFound = false;
	while (index < m_objectMaterials.size() && !bFound)
	{
		if (m_objectMaterials[index].tag == tag)
		{
			bFound = true;
			material = m_objectMaterials[index];
		}
		else
		{
			index++;
		}
	}
	return bFound;
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	glm::mat4 modelView;
	glm::mat4 scale = glm::scale(scaleXYZ);
	glm::mat4 rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	glm::mat4 rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (m_pShaderManager != NULL)
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	glm::vec4 currentColor(redColorValue, greenColorValue, blueColorValue, alphaValue);

	if (m_pShaderManager != NULL)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(std::string textureTag)
{
	if (m_pShaderManager != NULL)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);
		int textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (m_pShaderManager != NULL)
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(std::string materialTag)
{
	if (!m_objectMaterials.empty())
	{
		OBJECT_MATERIAL material;
		if (FindMaterial(materialTag, material))
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}
#include <iostream> // For debug output

/***********************************************************
 *  scroll_callback()
 *
 *  Callback function for handling scroll input to zoom camera.
 ***********************************************************/
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	static glm::vec3& cameraPos = *reinterpret_cast<glm::vec3*>(glfwGetWindowUserPointer(window));
	static glm::vec3& cameraFront = *reinterpret_cast<glm::vec3*>((char*)glfwGetWindowUserPointer(window) + sizeof(glm::vec3));
	cameraPos += cameraFront * static_cast<float>(yoffset);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  Provides balanced lighting for the scene, improving visibility
 *  of light-colored objects like paper, mug, and pen while
 *  preserving a natural look through ambient, diffuse, and specular
 *  components from various sources.
 ***********************************************************/
void SceneManager::SetupSceneLights(const glm::vec3& cameraPos, const glm::vec3& cameraFront)
{
	// Directional Light — soft overhead lighting
	m_pShaderManager->setVec3Value("dirLight.direction", glm::vec3(-0.2f, -1.0f, -0.1f));
	m_pShaderManager->setVec3Value("dirLight.ambient", glm::vec3(0.4f));  // brighter ambient
	m_pShaderManager->setVec3Value("dirLight.diffuse", glm::vec3(0.7f));
	m_pShaderManager->setVec3Value("dirLight.specular", glm::vec3(0.7f));

	// Front Fill Light — simulate camera-facing lighting
	m_pShaderManager->setVec3Value("pointLight.position", glm::vec3(0.0f, 4.0f, 6.0f));  // move forward slightly
	m_pShaderManager->setVec3Value("pointLight.ambient", glm::vec3(0.25f));
	m_pShaderManager->setVec3Value("pointLight.diffuse", glm::vec3(0.75f));
	m_pShaderManager->setVec3Value("pointLight.specular", glm::vec3(1.0f));
	m_pShaderManager->setFloatValue("pointLight.constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLight.linear", 0.09f);
	m_pShaderManager->setFloatValue("pointLight.quadratic", 0.032f);

	// Rim Light — subtle warm glow
	m_pShaderManager->setVec3Value("pointLight2.position", glm::vec3(-4.0f, 3.0f, -2.0f));
	m_pShaderManager->setVec3Value("pointLight2.ambient", glm::vec3(0.08f, 0.04f, 0.02f));
	m_pShaderManager->setVec3Value("pointLight2.diffuse", glm::vec3(0.3f, 0.15f, 0.08f));
	m_pShaderManager->setVec3Value("pointLight2.specular", glm::vec3(0.4f, 0.2f, 0.1f));
	m_pShaderManager->setFloatValue("pointLight2.constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLight2.linear", 0.14f);
	m_pShaderManager->setFloatValue("pointLight2.quadratic", 0.07f);

	// Spotlight (Camera torch effect)
	m_pShaderManager->setVec3Value("spotLight.position", cameraPos);
	m_pShaderManager->setVec3Value("spotLight.direction", cameraFront);
	m_pShaderManager->setFloatValue("spotLight.cutOff", glm::cos(glm::radians(10.0f)));
	m_pShaderManager->setFloatValue("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));
	m_pShaderManager->setVec3Value("spotLight.ambient", glm::vec3(0.15f));
	m_pShaderManager->setVec3Value("spotLight.diffuse", glm::vec3(0.8f));
	m_pShaderManager->setVec3Value("spotLight.specular", glm::vec3(1.0f));
	m_pShaderManager->setFloatValue("spotLight.constant", 1.0f);
	m_pShaderManager->setFloatValue("spotLight.linear", 0.09f);
	m_pShaderManager->setFloatValue("spotLight.quadratic", 0.032f);
}





/***********************************************************
 *  PrepareScene()
 *
 *  Loads and sets up meshes, textures, materials, lights.
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// Load basic mesh shapes
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTorusMesh();

	// Load wood texture
	if (!CreateGLTexture("Debug/wood.jpg", "wood"))
	{
		std::cout << "[ERROR] Could not load wood.jpg\n";
	}

	// Scale texture UVs for repetition
	SetTextureUVScale(8.0f, 4.0f);
	BindGLTextures();

	// Define material for wood surface (floor/table)
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.tag = "woodMaterial";
	woodMaterial.ambientColor = glm::vec3(0.15f, 0.08f, 0.03f);   // deeper tone
	woodMaterial.ambientStrength = 0.25f;
	woodMaterial.diffuseColor = glm::vec3(0.5f, 0.3f, 0.1f);      // richer wood
	woodMaterial.specularColor = glm::vec3(0.5f);                  // stronger reflection
	woodMaterial.shininess = 48.0f;                            // semi-gloss
	m_objectMaterials.push_back(woodMaterial);

	// Define material for white ceramic mug
	OBJECT_MATERIAL whiteMaterial;
	whiteMaterial.tag = "whiteMaterial";
	whiteMaterial.ambientColor = glm::vec3(0.4f);                  // warmer ambient
	whiteMaterial.ambientStrength = 0.5f;
	whiteMaterial.diffuseColor = glm::vec3(1.0f);
	whiteMaterial.specularColor = glm::vec3(1.2f);                  // polished ceramic
	whiteMaterial.shininess = 96.0f;                            // glossy
	m_objectMaterials.push_back(whiteMaterial);
}


/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***********************************************************
 *  RenderScene()
 *
 *  Main per-frame rendering logic: camera input, matrices,
 *  transformations, materials, textures, and drawing objects.
 ***********************************************************/
void SceneManager::RenderScene()
{
	static glm::vec3 cameraPos = glm::vec3(0.0f, 2.0f, 8.0f);
	static glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	static glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
	static float yaw = -90.0f;
	static float pitch = 0.0f;
	static float lastX = 400.0f;
	static float lastY = 300.0f;
	static float speed = 5.0f;
	static bool firstMouse = true;
	static bool perspectiveMode = true;
	static float lastFrame = 0.0f;

	GLFWwindow* window = glfwGetCurrentContext();
	if (!window) return;

	glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glfwSetWindowUserPointer(window, &cameraPos);
	glfwSetScrollCallback(window, scroll_callback);

	m_pShaderManager->setVec3Value("viewPos", cameraPos);
	m_pShaderManager->setIntValue("bUseLighting", true);

	SetupSceneLights(cameraPos, cameraFront);

	// Handle frame timing
	float currentFrame = static_cast<float>(glfwGetTime());
	float deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;
	float adjustedSpeed = speed * deltaTime;

	// Mouse input
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	if (firstMouse)
	{
		lastX = static_cast<float>(xpos);
		lastY = static_cast<float>(ypos);
		firstMouse = false;
	}

	float xoffset = static_cast<float>(xpos) - lastX;
	float yoffset = lastY - static_cast<float>(ypos);
	lastX = static_cast<float>(xpos);
	lastY = static_cast<float>(ypos);

	const float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;
	yaw += xoffset;
	pitch += yoffset;
	pitch = glm::clamp(pitch, -89.0f, 89.0f);

	glm::vec3 front;
	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraFront = glm::normalize(front);

	// Keyboard input
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		cameraPos += adjustedSpeed * cameraFront;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		cameraPos -= adjustedSpeed * cameraFront;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * adjustedSpeed;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * adjustedSpeed;
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		cameraPos += adjustedSpeed * cameraUp;
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		cameraPos -= adjustedSpeed * cameraUp;

	// Projection toggle
	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
		perspectiveMode = true;
	if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
		perspectiveMode = false;

	glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
	glm::mat4 projection = perspectiveMode ?
		glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f) :
		glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 100.0f);

	m_pShaderManager->setMat4Value("view", view);
	m_pShaderManager->setMat4Value("projection", projection);

	glm::vec3 scaleXYZ, positionXYZ;

	// Floor (Wood Table)
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetTextureUVScale(4.0f, 2.0f);
	SetShaderTexture("wood");
	SetShaderMaterial("woodMaterial");
	m_basicMeshes->DrawPlaneMesh();

	m_pShaderManager->setIntValue("bUseTexture", false);

	// Mug Body – smaller and properly lowered
	scaleXYZ = glm::vec3(0.75f, 1.125f, 0.75f);       // 75% of original
	positionXYZ = glm::vec3(8.0f, 0.5625f, 0.0f);      // Y = half of height
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderMaterial("whiteMaterial");
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// Mug Rim
	scaleXYZ = glm::vec3(0.375f, 0.375f, 0.0375f);
	positionXYZ = glm::vec3(8.0f, 1.125f, 0.0f);       // top of mug
	SetTransformations(scaleXYZ, 90.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	m_basicMeshes->DrawTorusMesh();

	// Corrected Mug Handle Position
	scaleXYZ = glm::vec3(0.3f, 0.3f, 0.075f);
	positionXYZ = glm::vec3(8.75f, 0.85f, 0.0f); // closer to mug and raised slightly
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 90.0f, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	m_basicMeshes->DrawTorusMesh();

	// Notebook (Dark Blue)
	scaleXYZ = glm::vec3(3.0f, 0.2f, 2.0f);
	positionXYZ = glm::vec3(-3.0f, 0.2f, 1.0f);
	SetTransformations(scaleXYZ, 0.0f, 15.0f, 0.0f, positionXYZ);
	SetShaderColor(0.1f, 0.1f, 0.4f, 1.0f);
	m_basicMeshes->DrawPlaneMesh();

	// Pen (Bright Red, fixed position and clearly visible)
	scaleXYZ = glm::vec3(0.1f, 2.0f, 0.1f);  // Thin cylinder for pen body
	positionXYZ = glm::vec3(-2.8f, 0.5f, 1.7f);  // On top of notebook
	SetTransformations(scaleXYZ, 90.0f, 15.0f, 0.0f, positionXYZ);
	SetShaderColor(1.0f, 0.0f, 0.0f, 1.0f);  // Bright red color
	m_basicMeshes->DrawCylinderMesh();

	// Laptop Base – slightly raised and flatter
	scaleXYZ = glm::vec3(3.0f, 0.05f, 2.0f);
	positionXYZ = glm::vec3(3.0f, 0.075f, -2.0f); // slight lift above table
	SetTransformations(scaleXYZ, 0.0f, -10.0f, 0.0f, positionXYZ);
	SetShaderColor(0.75f, 0.75f, 0.75f, 1.0f);
	m_basicMeshes->DrawPlaneMesh();

	// Laptop Screen – slightly back, better aligned to base
	scaleXYZ = glm::vec3(3.0f, 2.0f, 1.0f);
	positionXYZ = glm::vec3(3.0f, 1.15f, -2.95f); // lowered and moved forward
	SetTransformations(scaleXYZ, -100.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.2f, 0.2f, 0.2f, 1.0f);
	m_basicMeshes->DrawPlaneMesh();

}


