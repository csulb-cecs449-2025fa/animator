#include <glad/glad.h>
#include <iostream>
#include <memory>

#include <SFML/Window/Event.hpp>
#include <SFML/Window/Window.hpp>
#include <SFML/Graphics.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "StbImage.h"
#include "ShaderProgram.h"
#include "Animator.h"
#include "RotationAnimation.h"

struct Mesh {
	uint32_t vao;
	uint32_t faces;
	uint32_t texture;
};

struct Vertex3D {
	float x;
	float y;
	float z;
	float u;
	float v;
};

ShaderProgram textureShader() {
	ShaderProgram shader{};
	try {
		shader.load("shaders/texture_perspective.vert", "shaders/texturing.frag");
	}
	catch (std::runtime_error& e) {
		std::cout << "ERROR: " << e.what() << std::endl;
		exit(1);
	}
	return shader;
}

Mesh constructMesh(const std::vector<Vertex3D>& vertices, const std::vector<uint32_t>& faces) {
	Mesh m{};
	m.faces = static_cast<uint32_t>(faces.size());

	// Generate a vertex array object on the GPU.
	glGenVertexArrays(1, &m.vao);
	// "Bind" the newly-generated vao, which makes future functions operate on that specific object.
	glBindVertexArray(m.vao);

	// Generate a vertex buffer object on the GPU.
	uint32_t vbo;
	glGenBuffers(1, &vbo);

	// "Bind" the newly-generated vbo, which makes future functions operate on that specific object.
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	// This vbo is now associated with m_vao.
	// Copy the contents of the vertices list to the buffer that lives on the GPU.
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex3D), &vertices[0], GL_STATIC_DRAW);
	// Inform OpenGL how to interpret the buffer: each vertex is 3 contiguous floats (4 bytes each)
	glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex3D), 0);
	glEnableVertexAttribArray(0);
	// TODO: configure a second vertex attribute, of 2 floats, offset by the (x, y, z) of the vertex.
	// See the Texture lecture slides.




	// Generate a second buffer, to store the indices of each triangle in the mesh.
	uint32_t ebo;
	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, faces.size() * sizeof(uint32_t), &faces[0], GL_STATIC_DRAW);

	// Unbind the vertex array, so no one else can accidentally mess with it.
	glBindVertexArray(0);

	return m;
}

const size_t VERTICES_PER_FACE = 3;

// Reads the vertices and faces of an Assimp mesh, and uses them to initialize mesh structures
// compatible with the rest of our application.
void fromAssimpMesh(const aiMesh* mesh, std::vector<Vertex3D>& vertices,
	std::vector<uint32_t>& faces) {
	for (size_t i{ 0 }; i < mesh->mNumVertices; ++i) {
		// Each "vertex" from Assimp has to be transformed into a Vertex3D in our application.
		vertices.push_back(
			Vertex3D{ mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z }
		);
		// TODO: construct the vertex above with texture coordinates (u, v) in addition to (x, y, z).
		// To get (u, v) for the current Assimp vertex, access the mTextureCoords field of the
		// "mesh" local variable. mTextureCoords[0] is the first texture of the mesh (in the future,
		// meshes will have more than 1 texture). mTextureCoords[0][i] is the (u, v) coordinate
		// for the index-i vertex in the first texture. The .x and .y fields of this coordinate
		// are the u and v coordinates for the vertex, to be used when constructing our Vertex3D object.



	}

	faces.reserve(mesh->mNumFaces * VERTICES_PER_FACE);
	for (size_t i{ 0 }; i < mesh->mNumFaces; ++i) {
		// We assume the faces are triangular, so we push three face indexes at a time into our faces list.
		faces.push_back(mesh->mFaces[i].mIndices[0]);
		faces.push_back(mesh->mFaces[i].mIndices[1]);
		faces.push_back(mesh->mFaces[i].mIndices[2]);
	}
}

// Loads an asset file supported by Assimp, extracts the first mesh in the file, and fills in the 
// given vertices and faces lists with its data.
Mesh assimpLoad(const std::string& path, bool flipUvs = false) {
	int flags{ static_cast<aiPostProcessSteps>(aiProcessPreset_TargetRealtime_MaxQuality) };
	if (flipUvs) {
		flags |= aiProcess_FlipUVs;
	}

	Assimp::Importer importer{};
	const aiScene* scene{ importer.ReadFile(path, flags) };

	// If the import failed, report it
	if (nullptr == scene) {
		std::cout << "ASSIMP ERROR" << importer.GetErrorString() << std::endl;
		exit(1);
	}
	else {
		std::vector<Vertex3D> vertices{};
		std::vector<uint32_t> faces{};
		fromAssimpMesh(scene->mMeshes[0], vertices, faces);
		return constructMesh(vertices, faces);
	}
}

void drawMesh(const Mesh& m) {
	glBindTexture(GL_TEXTURE_2D, m.texture);
	glBindVertexArray(m.vao);
	// Draw the vertex array, using is "element buffer" to identify the faces, and whatever ShaderProgram
	// has been activated prior to this.
	glDrawElements(GL_TRIANGLES, m.faces, GL_UNSIGNED_INT, nullptr);
	// Deactivate the mesh's vertex array.
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

uint32_t generateTexture(const StbImage& texture) {
	uint32_t textureId;
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGBA,
		texture.getWidth(),
		texture.getHeight(),
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		texture.getData()
	);
	glGenerateMipmap(GL_TEXTURE_2D);
	return textureId;
}

// A scene of a textured triangle.
Mesh triangle() {
	std::vector<Vertex3D> triangleVertices{
		{ -0.5, -0.5, 0, 0, 1 },
		{ -0.5, 0.5, 0,  0, 0 },
		{  0.5, 0.5, 0,  1, 0 }
	};
	std::vector<uint32_t> triangleFaces{
		2, 1, 0
	};
	Mesh m{ constructMesh(triangleVertices, triangleFaces) };

	StbImage wall{};
	wall.loadFromFile("models/wall.jpg");
	m.texture = generateTexture(wall);
	return m;
}

Mesh bunny() {
	// Load the bunny, but vertically flip its UV coordinates, because the creator uses 
	// (0, 0) as the lower LEFT corner of texture space.
	Mesh obj{ assimpLoad("models/bunny_textured.obj", true) };
	StbImage texture{};
	texture.loadFromFile("models/bunny_textured.jpg");
	obj.texture = generateTexture(texture);
	return obj;
}

glm::mat4 buildModelMatrix(const glm::vec3& position, const glm::vec3& orientation, const glm::vec3& scale) {
	auto m{ glm::translate(glm::mat4(1), position) };
	m = glm::scale(m, scale);
	m = glm::rotate(m, orientation[2], glm::vec3{ 0, 0, 1 });
	m = glm::rotate(m, orientation[0], glm::vec3{ 1, 0, 0 });
	m = glm::rotate(m, orientation[1], glm::vec3{ 0, 1, 0 });
	return m;
}

int main() {
	sf::ContextSettings settings;
	settings.depthBits = 24; // Request a 24 bits depth buffer
	settings.stencilBits = 8;  // Request a 8 bits stencil buffer
	settings.majorVersion = 3; // You might have to change these on Mac.
	settings.minorVersion = 3;

	sf::Window window{
		sf::VideoMode::getFullscreenModes().at(0), "Modern OpenGL",
		sf::Style::Resize | sf::Style::Close,
		sf::State::Windowed, settings
	};

	gladLoadGL();
	glEnable(GL_DEPTH_TEST);

	// Inintialize scene objects.
	Mesh obj{ bunny() };
	//Mesh obj = triangle();
	glm::vec3 objectPosition{ 0, 0, -3 };
	glm::vec3 objectOrientation{ 0, 0, 0 };
	glm::vec3 objectScale{ 3, 3, 3 };

	// Activate the shader program.
	ShaderProgram program{ textureShader() };
	program.activate();

	// Initialize an animator.
	Animator bunnyAnimator{};
	bunnyAnimator.addAnimation(std::make_unique<RotationAnimation>(objectOrientation, 10.0f, glm::vec3{ 0, 6.28, 0 }));
	bunnyAnimator.start();

	// Ready, set, go!
	bool running{ true };
	sf::Clock c;

	auto last = c.getElapsedTime();
	while (running) {
		// Check for events.
		while (const std::optional event{ window.pollEvent() }) {
			if (event->is<sf::Event::Closed>()) {
				window.close();
			}
		}
		auto now{ c.getElapsedTime() };
		auto diff{ now - last };
		last = now;

#ifdef LOG_FPS
		// FPS calculation.
		std::cout << 1 / diff.asSeconds() << " FPS " << std::endl;
#endif

		// Apply animations.
		bunnyAnimator.tick(diff.asSeconds());

		// Set up the model, view and projection matrices.
		glm::mat4 model{
			buildModelMatrix(objectPosition, objectOrientation, objectScale)
		};
		glm::mat4 camera{
			glm::lookAt(glm::vec3{0, 0, 0}, glm::vec3{0, 0, -1}, glm::vec3{0, 1, 0}) 
		};
		glm::mat4 perspective{
			glm::perspective(glm::radians(45.0), static_cast<double>(window.getSize().x) / window.getSize().y, 0.1, 100.0)
		};
		program.setUniform("model", model);
		program.setUniform("view", camera);
		program.setUniform("projection", perspective);

		// Draw!
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		drawMesh(obj);
		window.display();
	}

	return 0;
}


