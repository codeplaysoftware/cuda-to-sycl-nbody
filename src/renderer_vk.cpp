#include "renderer.hpp"

struct RendererImpl
{

};

Renderer::Renderer()
{
	impl = new RendererImpl();
}

Renderer::~Renderer()
{
	delete impl;
}

void Renderer::initWindow(GLFWwindow *window)
{
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
}

void Renderer::init(int width, int height, sim_param params)
{

}

void Renderer::populateParticles( 
	const std::vector<glm::vec4> pos, 
	const std::vector<glm::vec4> vel)
{

}

void Renderer::stepSim()
{

}

void Renderer::render(glm::mat4 proj_mat, glm::mat4 view_mat)
{

}