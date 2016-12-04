#pragma once

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include <vector>
#include <string>
#include <iostream>

#define log(x) std::cout << (x) << std::endl

struct QueueFamilies
{
	int32_t graphics;
	int32_t present;

	bool isValid() { return graphics >= 0 && present >= 0;}
};

class VulkanApp
{
protected:
	VkInstance instance;
	VkDevice device;
	VkPhysicalDevice physicalDevice;
	QueueFamilies families;
	VkCommandPool commandPool;

	VkQueue graphicsQueue;
	VkQueue presentQueue;

	VkSurfaceKHR surface;

	VkPhysicalDeviceProperties deviceProperties;

	struct {
		VkSwapchainKHR swapchain;
		std::vector<VkImage> images;
		std::vector<VkImageView> views;
		std::vector<VkFramebuffer> framebuffers;
		std::vector<VkCommandBuffer> commandBuffers;
		std::vector<VkFence> fences;
		VkExtent2D extent;
		VkFormat format;
		VkSemaphore imageAvailable;
		VkSemaphore renderFinished;
	} swapchain;

	void createInstance(const std::string &appName);
	void createSurface(GLFWwindow *window);
	void createDevice();
	void createSwapchain(int width, int height);
	void createFramebuffers(VkRenderPass renderPass);
	void createCommandPool();

	void createBuffer(
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkBuffer &buffer,
		VkDeviceMemory &memory);

	void createImage(
		VkImageType imageType,
		VkFormat format,
		VkExtent3D extent,
		uint32_t mipLevels,
		uint32_t arrayLayers,
		VkSampleCountFlagBits samples,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkImageLayout layout,
		VkMemoryPropertyFlags properties,
		VkImage &image,
		VkDeviceMemory &memory);

	uint32_t acquireImage();
	void present(uint32_t imageIndex);	

	VkShaderModule createShaderModule(const std::string &filename);
	int32_t findProperties(VkMemoryPropertyFlags flags);
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	void destroy();
};