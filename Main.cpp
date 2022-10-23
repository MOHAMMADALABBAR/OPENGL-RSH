//------- Ignore this ----------
#include<filesystem>
namespace fs = std::filesystem;
//------------------------------
#include <glad/glad.h>
#include"Mesh.h"


#include <irrKlang.h>
using namespace irrklang;

const unsigned int width = 800;
const unsigned int height = 800;

# include <cassert>
# include <iostream>
# include <fstream>
# include <sstream>
# include "PerlinNoise.hpp"
#include "VAO.h"

using namespace std;

# pragma pack (push, 1)
struct BMPHeader
{
	std::uint16_t bfType;
	std::uint32_t bfSize;
	std::uint16_t bfReserved1;
	std::uint16_t bfReserved2;
	std::uint32_t bfOffBits;
	std::uint32_t biSize;
	std::int32_t  biWidth;
	std::int32_t  biHeight;
	std::uint16_t biPlanes;
	std::uint16_t biBitCount;
	std::uint32_t biCompression;
	std::uint32_t biSizeImage;
	std::int32_t  biXPelsPerMeter;
	std::int32_t  biYPelsPerMeter;
	std::uint32_t biClrUsed;
	std::uint32_t biClrImportant;
};
static_assert(sizeof(BMPHeader) == 54);
# pragma pack (pop)

struct RGB
{
	double r = 0.0;
	double g = 0.0;
	double b = 0.0;
	constexpr RGB() = default;
	explicit constexpr RGB(double _rgb) noexcept
		: r{ _rgb }, g{ _rgb }, b{ _rgb } {}
	constexpr RGB(double _r, double _g, double _b) noexcept
		: r{ _r }, g{ _g }, b{ _b } {}
};

class Image
{
public:

	Image() = default;

	Image(std::size_t width, std::size_t height)
		: m_data(width* height)
		, m_width{ static_cast<std::int32_t>(width) }
		, m_height{ static_cast<std::int32_t>(height) } {}

	void set(std::int32_t x, std::int32_t y, const RGB& color)
	{
		if (not inBounds(y, x))
		{
			return;
		}

		m_data[static_cast<std::size_t>(y) * m_width + x] = color;
	}

	std::int32_t width() const noexcept { return m_width; }

	std::int32_t height() const noexcept { return m_height; }

	bool saveBMP(const std::string& path)
	{
		const std::int32_t  rowSize = m_width * 3 + m_width % 4;
		const std::uint32_t bmpsize = rowSize * m_height;
		const BMPHeader header =
		{
			0x4d42,
			static_cast<std::uint32_t>(bmpsize + sizeof(BMPHeader)),
			0, 0, sizeof(BMPHeader), 40,
			m_width, m_height, 1, 24,
			0, bmpsize, 0, 0, 0, 0
		};

		if (std::ofstream ofs{ path, std::ios_base::binary })
		{
			ofs.write(reinterpret_cast<const char*>(&header), sizeof(header));

			std::vector<std::uint8_t> line(rowSize);

			for (std::int32_t y = m_height - 1; -1 < y; --y)
			{
				size_t pos = 0;

				for (std::int32_t x = 0; x < m_width; ++x)
				{
					const RGB& col = m_data[static_cast<std::size_t>(y) * m_width + x];
					line[pos++] = ToUint8(col.b);
					line[pos++] = ToUint8(col.g);
					line[pos++] = ToUint8(col.r);
				}

				ofs.write(reinterpret_cast<const char*>(line.data()), line.size());
			}

			return true;
		}
		else
		{
			return false;
		}
	}

private:

	std::vector<RGB> m_data;

	std::int32_t m_width = 0, m_height = 0;

	bool inBounds(std::int32_t y, std::int32_t x) const noexcept
	{
		return (0 <= y) && (y < m_height) && (0 <= x) && (x < m_width);
	}

	static constexpr std::uint8_t ToUint8(double x) noexcept
	{
		return (x <= 0.0) ? 0 : (1.0 <= x) ? 255 : static_cast<std::uint8_t>(x * 255.0 + 0.5);
	}
};

void Test()
{
	siv::PerlinNoise perlinA{ std::random_device{} };
	siv::PerlinNoise perlinB;

	perlinB.deserialize(perlinA.serialize());

	assert(perlinA.octave3D(0.1, 0.2, 0.3, 4)
		== perlinB.octave3D(0.1, 0.2, 0.3, 4));

	perlinA.reseed(12345u);
	perlinB.reseed(12345u);

	assert(perlinA.octave3D(0.1, 0.2, 0.3, 4)
		== perlinB.octave3D(0.1, 0.2, 0.3, 4));

	perlinA.reseed(std::mt19937{ 67890u });
	perlinB.reseed(std::mt19937{ 67890u });

	assert(perlinA.octave3D(0.1, 0.2, 0.3, 4)
		== perlinB.octave3D(0.1, 0.2, 0.3, 4));

	for (std::int32_t y = 0; y < 20; ++y)
	{
		for (std::int32_t x = 0; x < 20; ++x)
		{
			const double noise = perlinA.octave2D_01(x * 0.1, y * 0.1, 6);
			std::cout << static_cast<int>(std::floor(noise * 10) - 0.5);
		}
		std::cout << '\n';
	}
}

int noise()
{
	Test();

	Image image{ 512, 512 };

	std::cout << "---------------------------------\n";
	std::cout << "* frequency [0.1 .. 8.0 .. 64.0] \n";
	std::cout << "* octaves   [1 .. 8 .. 16]       \n";
	std::cout << "* seed      [0 .. 2^32-1]        \n";
	std::cout << "---------------------------------\n";

	for (;;)
	{
		double frequency;
		std::cout << "double frequency = ";
		std::cin >> frequency;
		frequency = std::clamp(frequency, 0.1, 64.0);

		std::int32_t octaves;
		std::cout << "int32 octaves    = ";
		std::cin >> octaves;
		octaves = std::clamp(octaves, 1, 16);

		std::uint32_t seed;
		std::cout << "uint32 seed      = ";
		std::cin >> seed;

		const siv::PerlinNoise perlin{ seed };
		const double fx = (frequency / image.width());
		const double fy = (frequency / image.height());

		for (std::int32_t y = 0; y < image.height(); ++y)
		{
			for (std::int32_t x = 0; x < image.width(); ++x)
			{
				const RGB color(perlin.octave2D_01((x * fx), (y * fy), octaves));
				image.set(x, y, color);
			}
		}

		std::stringstream ss;
		ss << 'f' << frequency << 'o' << octaves << '_' << seed << ".bmp";

		if (image.saveBMP(ss.str()))
		{
			std::cout << "...saved \"" << ss.str() << "\"\n";
		}
		else
		{
			std::cout << "...failed\n";
		}

		char c;
		std::cout << "continue? [y/n] >";
		std::cin >> c;
		if (c != 'y') break;
		std::cout << '\n';
	}
	return 0;
}

//------- Ignore this ----------
#include<filesystem>
namespace fs = std::filesystem;
//------------------------------

#include"Model.h"




float skyboxVertices[] =
{
	//   Coordinates
	-1.0f, -1.0f,  1.0f,//        7--------6
	 1.0f, -1.0f,  1.0f,//       /|       /|
	 1.0f, -1.0f, -1.0f,//      4--------5 |
	-1.0f, -1.0f, -1.0f,//      | |      | |
	-1.0f,  1.0f,  1.0f,//      | 3------|-2
	 1.0f,  1.0f,  1.0f,//      |/       |/
	 1.0f,  1.0f, -1.0f,//      0--------1
	-1.0f,  1.0f, -1.0f
};

unsigned int skyboxIndices[] =
{
	// Right
	1, 2, 6,
	6, 5, 1,
	// Left
	0, 4, 7,
	7, 3, 0,
	// Top
	4, 5, 6,
	6, 7, 4,
	// Bottom
	0, 3, 2,
	2, 1, 0,
	// Back
	0, 1, 5,
	5, 4, 0,
	// Front
	3, 7, 6,
	6, 2, 3
};
void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

int main()
{
	// Initialize GLFW
	glfwInit();
	noise();

	// Tell GLFW what version of OpenGL we are using 
	// In this case we are using OpenGL 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	// Tell GLFW we are using the CORE profile
	// So that means we only have the modern functions
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Create a GLFWwindow object of 800 by 800 pixels, naming it "YoutubeOpenGL"
	GLFWwindow* window = glfwCreateWindow(width, height, "Online_Game", NULL, NULL);
	// Error check if the window fails to create
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	// Introduce the window into the current context
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	//Load GLAD so it configures OpenGL
	gladLoadGL();
	// Specify the viewport of OpenGL in the Window
	// In this case the viewport goes from x = 0, y = 0, to x = 800, y = 800






	// Generates Shader objects
	Shader shaderProgram("default.vert", "default.frag");
	Shader skyboxShader("skybox.vert", "skybox.frag");

	// Take care of all the light related things
	glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	glm::vec3 lightPos = glm::vec3(0.5f, 0.5f, 0.5f);

	shaderProgram.Activate();
	glUniform4f(glGetUniformLocation(shaderProgram.ID, "lightColor"), lightColor.x, lightColor.y, lightColor.z, lightColor.w);
	glUniform3f(glGetUniformLocation(shaderProgram.ID, "lightPos"), lightPos.x, lightPos.y, lightPos.z);
	skyboxShader.Activate();
	glUniform1i(glGetUniformLocation(skyboxShader.ID, "skybox"), 0);
	// vertex generation



	// register VAO


;
	// draw mesh



	// Enables the Depth Buffer
	glEnable(GL_DEPTH_TEST);

	// Enables Cull Facing
	glEnable(GL_CULL_FACE);
	// Keeps front faces
	glCullFace(GL_FRONT);
	// Uses counter clock-wise standard
	glFrontFace(GL_CCW);

	// Creates camera object
	Camera camera(width, height, glm::vec3(0.0f, 0.0f, 2.0f));


	/*
	* I'm doing this relative path thing in order to centralize all the resources into one folder and not
	* duplicate them between tutorial folders. You can just copy paste the resources from the 'Resources'
	* folder and then give a relative path from this folder to whatever resource you want to get to.
	* Also note that this requires C++17, so go to Project Properties, C/C++, Language, and select C++17
	*/
	std::string parentDir = (fs::current_path().fs::path::parent_path()).string();
	std::string modelPath = "/Resources/YoutubeOpenGL 19 - Cubemaps & Skyboxes/models/airplane/scene.gltf";

	// Load in models
	Model model((parentDir + modelPath).c_str());



	// Variables to create periodic event for FPS displaying
	double prevTime = 0.0;
	double crntTime = 0.0;
	double timeDiff;
	// Keeps track of the amount of frames in timeDiff
	unsigned int counter = 0;

	// Use this to disable VSync (not advized)
	//glfwSwapInterval(0);

	
	// Create VAO, VBO, and EBO for the skybox
	unsigned int skyboxVAO, skyboxVBO, skyboxEBO;
	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glGenBuffers(1, &skyboxEBO);
	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skyboxEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(skyboxIndices), &skyboxIndices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
    double second = 0.0;
	



	   string facesCubemap[6] =
	{
		parentDir + "/Resources/YoutubeOpenGL 19 - Cubemaps & Skyboxes/skybox/right.jpg",
		parentDir + "/Resources/YoutubeOpenGL 19 - Cubemaps & Skyboxes/skybox/left.jpg",
		parentDir + "/Resources/YoutubeOpenGL 19 - Cubemaps & Skyboxes/skybox/top.jpg",
		parentDir + "/Resources/YoutubeOpenGL 19 - Cubemaps & Skyboxes/skybox/bottom.jpg",
		parentDir + "/Resources/YoutubeOpenGL 19 - Cubemaps & Skyboxes/skybox/front.jpg",
		parentDir + "/Resources/YoutubeOpenGL 19 - Cubemaps & Skyboxes/skybox/back.jpg"
	};
		string facesCubemap1[6] =
	{
		parentDir + "/Resources/YoutubeOpenGL 19 - Cubemaps & Skyboxes/skybox/1.png",
		parentDir + "/Resources/YoutubeOpenGL 19 - Cubemaps & Skyboxes/skybox/1.png",
		parentDir + "/Resources/YoutubeOpenGL 19 - Cubemaps & Skyboxes/skybox/1.png",
		parentDir + "/Resources/YoutubeOpenGL 19 - Cubemaps & Skyboxes/skybox/1.png",
		parentDir + "/Resources/YoutubeOpenGL 19 - Cubemaps & Skyboxes/skybox/1.png",
		parentDir + "/Resources/YoutubeOpenGL 19 - Cubemaps & Skyboxes/skybox/1.png"
	};
	// All the faces of the cubemap (make sure they are in this exact order)
	
	// Creates the cubemap texture object
	unsigned int cubemapTexture;
	glGenTextures(1, &cubemapTexture);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	// These are very important to prevent seams
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	// This might help with seams on some systems
	//glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);



	for (unsigned int x = 0; x < 6; x++)
	{

		int width, height, nrChannels;
		unsigned char* data = stbi_load(facesCubemap[x].c_str(), &width, &height, &nrChannels, 0);

		if (data)
		{
			stbi_set_flip_vertically_on_load(false);
			glTexImage2D
			(
				GL_TEXTURE_CUBE_MAP_POSITIVE_X + x,
				0,
				GL_RGB,
				width,
				height,
				0,
				GL_RGB,
				GL_UNSIGNED_BYTE,
				data
			);
			stbi_image_free(data);
		}
		else
		{

			std::cout << "Failed to load texture: " << facesCubemap1[x] << std::endl;
			stbi_image_free(data);
		}
	}	
	
        	ISoundEngine* engine = createIrrKlangDevice();engine->play2D("moog.mp3", true);if (!engine) {
		return 0;
	}
	// Main while loop
	while (!glfwWindowShouldClose(window))
	{
		// Updates counter and times
		crntTime = glfwGetTime();
		timeDiff = crntTime - prevTime;
		counter++;

		if (timeDiff >= 1.0 / 30.0)
		{
			// Creates new title
			std::string FPS = std::to_string((1.0 / timeDiff) * counter);
			std::string ms = std::to_string((timeDiff / counter) * 1000);
			std::string newTitle = "Online_Game - " + FPS + "FPS / " + ms + "ms";
			glfwSetWindowTitle(window, newTitle.c_str());

			// Resets times and counter
			prevTime = crntTime;
			counter = 0;

			// Use this if you have disabled VSync
			//camera.Inputs(window);
		}
	
		// Specify the color of the background
		glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
		// Clean the back buffer and depth buffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Handles camera inputs (delete this if you have disabled VSync)
		camera.Inputs(window);
		// Updates and exports the camera matrix to the Vertex Shader
		camera.updateMatrix(45.0f, 0.1f, 100.0f);


		// Draw the normal model
		model.Draw(shaderProgram, camera);

		// Since the cubemap will always have a depth of 1.0, we need that equal sign so it doesn't get discarded
		glDepthFunc(GL_LEQUAL);

		skyboxShader.Activate();
		glm::mat4 view = glm::mat4(1.0f);
		glm::mat4 projection = glm::mat4(1.0f);
		// We make the mat4 into a mat3 and then a mat4 again in order to get rid of the last row and column
		// The last row and column affect the translation of the skybox (which we don't want to affect)
		view = glm::mat4(glm::mat3(glm::lookAt(camera.Position, camera.Position + camera.Orientation, camera.Up)));
		projection = glm::perspective(glm::radians(45.0f), (float)width / height, 0.1f, 100.0f);
		glUniformMatrix4fv(glGetUniformLocation(skyboxShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(skyboxShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

		// Draws the cubemap as the last object so we can save a bit of performance by discarding all fragments
		// where an object is present (a depth of 1.0f will always fail against any object's depth value)
		glBindVertexArray(skyboxVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	    



	
	

		// Switch back to the normal depth function
		glDepthFunc(GL_LESS);

		processInput(window);
		// Swap the back buffer with the front buffer
		glfwSwapBuffers(window);
		// Take care of all GLFW events
		glfwPollEvents();
	}
  engine->drop(); // delete engine


	// Delete all the objects we've created
	shaderProgram.Delete();
	skyboxShader.Delete();
	// Delete window before ending the program
	glfwDestroyWindow(window);
	// Terminate GLFW before ending the program

	glfwTerminate();
	return 0;
}