///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
	const char* g_UVscaleName = "UVscale";
}

SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;
}

SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
	DestroyGLTextures();
}

bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0, height = 0, colorChannels = 0;
	GLuint textureID = 0;
	std::cout << "Attempting to load texture: " << filename << std::endl;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* image = stbi_load(filename, &width, &height, &colorChannels, 0);
	if (image)
	{
		std::cout << "Successfully loaded image: " << filename << ", width: " << width << ", height: " << height << ", channels: " << colorChannels << std::endl;
		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else {
			std::cout << "Unsupported channel count: " << colorChannels << std::endl;
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
	std::cout << "Could not load image: " << filename << std::endl;
	return false;
}

void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glDeleteTextures(1, &m_textureIDs[i].ID);
	}
}

int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	while (index < m_loadedTextures)
	{
		if (m_textureIDs[index].tag == tag)
		{
			textureID = m_textureIDs[index].ID;
			break;
		}
		index++;
	}
	return textureID;
}

int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	while (index < m_loadedTextures)
	{
		if (m_textureIDs[index].tag == tag)
		{
			textureSlot = index;
			break;
		}
		index++;
	}
	return textureSlot;
}

void SceneManager::SetTransformations(glm::vec3 scaleXYZ, float XrotationDegrees, float YrotationDegrees, float ZrotationDegrees, glm::vec3 positionXYZ)
{
	glm::mat4 modelView = glm::translate(positionXYZ) *
		glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0, 0, 1)) *
		glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0, 1, 0)) *
		glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1, 0, 0)) *
		glm::scale(scaleXYZ);

	if (m_pShaderManager != NULL)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

void SceneManager::SetShaderColor(float red, float green, float blue, float alpha)
{
	glm::vec4 color(red, green, blue, alpha);
	if (m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, color);
	}
}

void SceneManager::SetShaderTexture(std::string textureTag)
{
	if (m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);
		int textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

void SceneManager::SetTextureUVScale(float u, float v)
{
	if (m_pShaderManager)
	{
		m_pShaderManager->setVec2Value(g_UVscaleName, glm::vec2(u, v));
	}
}

void SceneManager::LoadSceneTextures()
{
	CreateGLTexture("stainless.jpg", "lamp_base");
	CreateGLTexture("stainless_end.jpg", "lamp_arm");
	CreateGLTexture("circular-brushed-gold-texture.jpg", "lamp_head");
	CreateGLTexture("tilesf2.jpg", "notebook");
	CreateGLTexture("stainless.jpg", "laptop_body");
	CreateGLTexture("screen_display.jpg", "laptop_screen"); // realistic sleek screen
	CreateGLTexture("rusticwood.jpg", "desk");
	BindGLTextures();
}

void SceneManager::PrepareScene()
{
	LoadSceneTextures();
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadTorusMesh();
}

void SceneManager::RenderScene()
{
	glm::vec3 scaleXYZ;
	float rotX = 0.0f, rotY = 0.0f, rotZ = 0.0f;
	glm::vec3 posXYZ;

	// Desk
	scaleXYZ = glm::vec3(60.0f, 1.0f, 30.0f);
	posXYZ = glm::vec3(0.0f, -1.2f, 2.0f);
	SetTransformations(scaleXYZ, 0, 0, 0, posXYZ);
	SetShaderTexture("desk");
	SetTextureUVScale(6.0f, 6.0f);
	m_basicMeshes->DrawPlaneMesh();

	// Lamp base
	scaleXYZ = glm::vec3(0.8f, 0.1f, 0.8f);
	posXYZ = glm::vec3(-4.5f, 0.05f, 1.0f);
	SetTransformations(scaleXYZ, 0, 0, 0, posXYZ);
	SetShaderTexture("lamp_base");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// Lamp arm
	scaleXYZ = glm::vec3(0.1f, 2.4f, 0.1f);
	rotZ = 10.0f;
	posXYZ = glm::vec3(-4.5f, 0.1f, 1.0f);
	SetTransformations(scaleXYZ, 0, 0, rotZ, posXYZ);
	SetShaderTexture("lamp_arm");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// Lamp head
	scaleXYZ = glm::vec3(0.3f, 0.3f, 0.3f);
	rotX = 20.0f;
	posXYZ = glm::vec3(-4.5f, 2.4f, 1.1f);
	SetTransformations(scaleXYZ, rotX, 0, 0, posXYZ);
	SetShaderTexture("lamp_head");
	SetTextureUVScale(4.0f, 4.0f);
	m_basicMeshes->DrawSphereMesh();

	// Notebook
	scaleXYZ = glm::vec3(1.5f, 0.15f, 1.0f);
	posXYZ = glm::vec3(1.5f, 0.03f, 2.8f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, posXYZ);
	SetShaderTexture("notebook");
	SetTextureUVScale(2.0f, 2.0f);
	m_basicMeshes->DrawBoxMesh();

	// Laptop body
	scaleXYZ = glm::vec3(3.0f, 0.1f, 2.0f);
	posXYZ = glm::vec3(-1.0f, 0.1f, 2.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, posXYZ);
	SetShaderTexture("laptop_body");
	SetTextureUVScale(1.5f, 1.5f);
	m_basicMeshes->DrawBoxMesh();

	// Laptop screen
	scaleXYZ = glm::vec3(3.0f, 2.5f, 0.1f);
	rotX = -29.0f;
	posXYZ = glm::vec3(-1.0f, 1.15f, 0.5f);
	SetTransformations(scaleXYZ, rotX, 0.0f, 0.0f, posXYZ);
	SetShaderTexture("laptop_screen");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Coffee Mug
	scaleXYZ = glm::vec3(0.3f, 0.5f, 0.3f);
	posXYZ = glm::vec3(-2.5f, 0.3f, 4.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, posXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// Mug handle
	scaleXYZ = glm::vec3(0.1f, 0.1f, 0.1f);
	rotX = 90.0f;
	posXYZ = glm::vec3(-2.25f, 0.3f, 4.0f);
	SetTransformations(scaleXYZ, rotX, 0.0f, 0.0f, posXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	m_basicMeshes->DrawTorusMesh();
}
