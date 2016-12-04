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

class Renderer
{

public:
	virtual void initWindow()=0;

	/**
	 * Initializes the gl state
	 * @param width viewport width
	 * @param height viewport height
	 * @param params simulation parameters
	 */
	virtual void init(GLFWwindow *window, int width, int height, SimParam params)=0;

	virtual void destroy()=0;

	/**
	 * Supplies the gl state with initial particle position and velocity
	 * @param pos particle positions
	 * @param vel particle velocities
	 */
	virtual void populateParticles( 
		const std::vector<glm::vec4> pos, 
		const std::vector<glm::vec4> vel)=0;

	/**
	 * Steps the simulation once, with the parameters provided with @see init
	 */
	virtual void stepSim()=0;

	/**
	 * Renders the particles at the current step
	 * @param proj_mat projection matrix @see camera_get_proj
	 * @param view_mat view matrix @see camera_get_view
	 */
	virtual void render(glm::mat4 projMat, glm::mat4 viewMat)=0;

};