#pragma once

#ifdef USE_OPENGL
#include <GL/glew.h>
#else
#ifdef USE_VULKAN
#include <vulkan/vulkan.h>
#endif
#endif
#include <GLFW/glfw3.h>

#include <vector>
#include <glm/glm.hpp>

#include "sim_param.hpp"

struct RendererImpl;

class Renderer
{

public:
	Renderer();
	~Renderer();

	void initWindow(GLFWwindow *window);

	/**
	 * Initializes the gl state
	 * @param width viewport width
	 * @param height viewport height
	 * @param params simulation parameters
	 */
	void init(int width, int height, sim_param params);

	/**
	 * Supplies the gl state with initial particle position and velocity
	 * @param pos particle positions
	 * @param vel particle velocities
	 */
	void populateParticles( 
		const std::vector<glm::vec4> pos, 
		const std::vector<glm::vec4> vel);

	/**
	 * Steps the simulation once, with the parameters provided with @see init
	 */
	void stepSim();

	/**
	 * Renders the particles at the current step
	 * @param proj_mat projection matrix @see camera_get_proj
	 * @param view_mat view matrix @see camera_get_view
	 */
	void render(glm::mat4 proj_mat, glm::mat4 view_mat);

private:
	RendererImpl *impl;

};