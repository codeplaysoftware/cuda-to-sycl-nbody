#include "vulkan_app.hpp"
#include <set>
#include <vector>
#include <string>
#include <fstream>
#include <limits>
#include <cstring>

/**
 * Returns whether vulkan supports the given layers
 */
bool areLayersSupported(const std::vector<const char*> &layers);

/**
 * Returns a score for a physical device - the highest score runs the application
 * A score of -1 is unsuitable, and can't run the application.
 */
int getPhysicalDeviceScore(
	VkInstance instance, 
	const std::vector<const char*> &extensions,
	VkSurfaceKHR surface,
	VkPhysicalDevice physicalDevice);

/**
 * Returns the physical device most suitable for this application
 */
VkPhysicalDevice findSuitablePhysicalDevice(
	VkInstance instance, 
	const std::vector<const char*> &extensions,
	VkSurfaceKHR surface);

/**
 * Returns valid queue family indices
 */
QueueFamilies findSuitableQueueFamilies(VkInstance instance, VkPhysicalDevice physicalDevice);

void VulkanApp::createInstance(const std::string &appName)
{
	// Create Instance
	VkApplicationInfo applicationInfo = {};
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	applicationInfo.pApplicationName = appName.c_str();

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &applicationInfo;

	std::vector<const char*> extensions = {};

	uint32_t count;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&count);

	for (uint32_t i=0;i<count;++i)
		extensions.push_back(glfwExtensions[i]);

	createInfo.enabledExtensionCount = extensions.size();
	createInfo.ppEnabledExtensionNames = extensions.data();

#ifdef USE_VALIDATION_LAYERS
	// Required layers
	std::vector<const char*> requiredLayers = {
		"VK_LAYER_LUNARG_standard_validation"
		//"VK_LAYER_LUNARG_api_dump"
	};

	if (!areLayersSupported(requiredLayers))
	{
		throw std::runtime_error("Validation layers not supported");
	}
	createInfo.enabledLayerCount   = requiredLayers.size();
	createInfo.ppEnabledLayerNames = requiredLayers.data();
#endif

	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
	{
		throw std::runtime_error("Can't create vulkan instance");
	}
}

void VulkanApp::createSurface(GLFWwindow *window)
{
	if (glfwCreateWindowSurface(
		instance,
		window, nullptr, 
		&surface)
		!= VK_SUCCESS)
	{
		throw std::runtime_error("Can't create Surface");
	}
}

void VulkanApp::createDevice()
{
	std::vector<const char*> extensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	// Find suitable device
	physicalDevice = findSuitablePhysicalDevice(instance, extensions, surface);

	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(physicalDevice, &properties);

	log(std::string(properties.deviceName) + " chosen"); 

	// Grab the queue families
	uint32_t queueFamilyPropertyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(
		physicalDevice, &queueFamilyPropertyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(
		physicalDevice, &queueFamilyPropertyCount, queueFamilyProperties.data());

	families = findSuitableQueueFamilies(instance, physicalDevice);

	std::set<int32_t> queueFamilies = {families.graphics, families.present};
	const float queuePriorities[] = {1.0};
	std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfo;
	for (uint32_t family : queueFamilies)
	{
		VkDeviceQueueCreateInfo queueci = {};
		queueci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueci.queueFamilyIndex = family;
		queueci.queueCount = 1;
		queueci.pQueuePriorities = queuePriorities;
		deviceQueueCreateInfo.push_back(queueci);
	}

	// Features
	VkPhysicalDeviceFeatures features = {};
	features.geometryShader = VK_TRUE;

	// Create logical device with one graphics and one presentation queue
	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = deviceQueueCreateInfo.size();
	createInfo.pQueueCreateInfos = deviceQueueCreateInfo.data();

	// Device specific extensions
	createInfo.enabledExtensionCount = extensions.size();
	createInfo.ppEnabledExtensionNames = extensions.data();

	createInfo.pEnabledFeatures = &features;

	if (vkCreateDevice(
		  physicalDevice,
		  &createInfo, nullptr, &device) != VK_SUCCESS)
	{
		throw std::runtime_error("Can't create logical device");
	}

	deviceProperties = properties;

	// Get queues
	vkGetDeviceQueue(device, families.graphics, 0, &graphicsQueue);
	vkGetDeviceQueue(device, families.present,  0, &presentQueue);
}

VkSurfaceFormatKHR chooseSurfaceFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
VkPresentModeKHR choosePresentMode(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
VkExtent2D chooseExtent(VkSurfaceCapabilitiesKHR capabilities, int width, int height);

void VulkanApp::createSwapchain(int width, int height)
{
	VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(physicalDevice, surface);
	VkPresentModeKHR presentMode = choosePresentMode(physicalDevice, surface);

	VkSurfaceCapabilitiesKHR capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);
	swapchain.extent = chooseExtent(capabilities, width, height);

	uint32_t imageCount = capabilities.minImageCount + 1;
	if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
		imageCount = capabilities.maxImageCount;

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = swapchain.extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32_t queueFamilyIndices[] = {
		(uint32_t)families.graphics, 
		(uint32_t)families.present};
	if (families.graphics != families.present)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform = capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain.swapchain)
		!= VK_SUCCESS)
	{
		throw std::runtime_error("Can't create swapchain");
	}

	swapchain.format = surfaceFormat.format;

	// Swapchain images
	uint32_t count;
	vkGetSwapchainImagesKHR(device, swapchain.swapchain, &count, nullptr);
	swapchain.images.resize(count);
	vkGetSwapchainImagesKHR(device, swapchain.swapchain, &count, swapchain.images.data());

	// Image views
	swapchain.views.resize(swapchain.images.size());
	for (uint32_t i=0;i<swapchain.images.size();++i)
	{
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = swapchain.images.at(i);
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = surfaceFormat.format;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(device, &createInfo, nullptr, &swapchain.views.at(i))
			!= VK_SUCCESS)
		{
			throw std::runtime_error("Can't create swapchain image views");
		}
	}

	// Semaphores
	VkSemaphoreCreateInfo sci = {};
	sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if (vkCreateSemaphore(device, &sci, nullptr, &swapchain.imageAvailable)
		!= VK_SUCCESS ||
			vkCreateSemaphore(device, &sci, nullptr, &swapchain.renderFinished)
		!= VK_SUCCESS)
	{
		throw std::runtime_error("Can't create semaphores");
	}

	// Fences
	swapchain.fences.resize(swapchain.views.size());
	for (size_t i=0; i<swapchain.fences.size(); ++i)
	{
		VkFenceCreateInfo fci = {};
		fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		if (vkCreateFence(device, &fci, nullptr, &swapchain.fences[i])
			!= VK_SUCCESS)
		{
			throw std::runtime_error("Can't create fences");
		}
	}
}

void VulkanApp::createFramebuffers(VkRenderPass renderPass)
{
	swapchain.framebuffers.resize(swapchain.views.size());
	// One framebuffer per swapchain image
	for (size_t i=0; i<swapchain.views.size(); ++i)
	{
		VkFramebufferCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.renderPass = renderPass;
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = &swapchain.views[i];
		createInfo.width  = swapchain.extent.width;
		createInfo.height = swapchain.extent.height;
		createInfo.layers = 1;

		if (vkCreateFramebuffer(device, &createInfo, nullptr, &swapchain.framebuffers[i])
			!= VK_SUCCESS)
		{
			throw std::runtime_error("Can't create framebuffer");
		}
	}
}

void VulkanApp::createCommandPool()
{
	// Command Pool creation
	VkCommandPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.queueFamilyIndex = families.graphics;
	createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vkCreateCommandPool(device, &createInfo, nullptr, &commandPool)
	  != VK_SUCCESS)
	{
		throw std::runtime_error("Can't create command pool");
	}

	// Command buffers allocation (for swapchain images)
	VkCommandBufferAllocateInfo allocateInfo = {};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.commandPool = commandPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = swapchain.images.size();

	swapchain.commandBuffers.resize(swapchain.views.size());
	if (vkAllocateCommandBuffers(device, &allocateInfo, swapchain.commandBuffers.data())
		!= VK_SUCCESS)
	{
		throw std::runtime_error("Can't allocate command buffers");
	}
}

uint32_t VulkanApp::acquireImage()
{
	uint32_t imageIndex;
	vkAcquireNextImageKHR(device, swapchain.swapchain, std::numeric_limits<uint64_t>::max(),
		swapchain.imageAvailable, VK_NULL_HANDLE, &imageIndex);
	return imageIndex;
}

void VulkanApp::present(uint32_t imageIndex)
{
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &swapchain.renderFinished;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain.swapchain;
	presentInfo.pImageIndices = &imageIndex;

	vkQueuePresentKHR(presentQueue, &presentInfo);
}

void VulkanApp::destroy()
{
	vkDeviceWaitIdle(device);
	vkDestroySemaphore(device, swapchain.imageAvailable, nullptr);
	vkDestroySemaphore(device, swapchain.renderFinished, nullptr);
	for (auto fence : swapchain.fences)
		vkDestroyFence(device, fence, nullptr);

	vkFreeCommandBuffers(device, commandPool, swapchain.commandBuffers.size(), swapchain.commandBuffers.data());

	vkDestroyCommandPool(device, commandPool, nullptr);
	for (auto fbo : swapchain.framebuffers)
		vkDestroyFramebuffer(device, fbo, nullptr);
	for (auto views : swapchain.views)
		vkDestroyImageView(device, views, nullptr);
	vkDestroySwapchainKHR(device, swapchain.swapchain, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyDevice(device, nullptr);
  vkDestroyInstance(instance, nullptr);
}

void VulkanApp::createBuffer(
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags properties,
	VkBuffer &buffer,
	VkDeviceMemory &memory)
{		
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer)
		!= VK_SUCCESS)
	{
		throw std::runtime_error("Can't create buffer");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
		properties);

	if (vkAllocateMemory(device, &allocInfo, nullptr, &memory)
		!= VK_SUCCESS)
	{
		throw std::runtime_error("Can't allocate buffer memory");
	}
	vkBindBufferMemory(device, buffer, memory, 0);
}

void VulkanApp::createImage(
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
	VkDeviceMemory &memory)
{
	VkImageCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	createInfo.imageType = imageType;
	createInfo.format = format;
	createInfo.extent = extent;
	createInfo.mipLevels = mipLevels;
	createInfo.arrayLayers = arrayLayers;
	createInfo.samples = samples;
	createInfo.tiling = tiling;
	createInfo.usage = usage;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.initialLayout = layout;

	if (vkCreateImage(device, &createInfo, nullptr, &image)
		!= VK_SUCCESS)
	{
		throw std::runtime_error("Can't create image");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
		properties);

	if (vkAllocateMemory(device, &allocInfo, nullptr, &memory)
		!= VK_SUCCESS)
	{
		throw std::runtime_error("Can't allocate image memory");
	}

	vkBindImageMemory(device, image, memory, 0);
}

bool areLayersSupported(const std::vector<const char*> &layers)
{
	// Query available layers
	uint32_t propertyCount;
	vkEnumerateInstanceLayerProperties(&propertyCount, nullptr);
	std::vector<VkLayerProperties> availableLayers(propertyCount);
	vkEnumerateInstanceLayerProperties(&propertyCount,availableLayers.data());

	// Check if required layers are available
	for (auto rl : layers)
	{
		bool supported = false;
		for (auto al : availableLayers)
		{
			if (strcmp(rl, al.layerName) == 0) supported = true;
		}
		if (!supported) return false;
	}
	return true;
}

bool areDeviceExtensionsSupported(VkPhysicalDevice physicalDevice, const std::vector<const char*> &extensions)
{
	uint32_t propertyCount;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &propertyCount, nullptr);
	std::vector<VkExtensionProperties> properties(propertyCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &propertyCount, properties.data());

	for (auto ext : extensions)
	{
		bool extensionSupported = false;
		for (auto prop : properties)
		{
			if (strcmp(prop.extensionName, ext) == 0)
			{
				extensionSupported = true;
				break;
			}
			if (!extensionSupported) return false;
		}
	}
	return true;
}

QueueFamilies findSuitableQueueFamilies(VkInstance instance, VkPhysicalDevice physicalDevice)
{
	// Queue family properties
	uint32_t queueFamilyPropertyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(
		physicalDevice, &queueFamilyPropertyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(
		physicalDevice, &queueFamilyPropertyCount, queueFamilyProperties.data());

	QueueFamilies families = {-1, -1};

	// Get index of first queue family
	for (uint32_t i=0;i<queueFamilyProperties.size();++i)
	{
		if (families.graphics == -1 && queueFamilyProperties.at(i).queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			families.graphics = i;
		}
		if (families.present == -1 && glfwGetPhysicalDevicePresentationSupport(instance, physicalDevice, i))
		{
			families.present = i;
		}
	}
	return families;
}

int getPhysicalDeviceScore(
	VkInstance instance, 
	const std::vector<const char*> &extensions,
	VkSurfaceKHR surface,
	VkPhysicalDevice physicalDevice)
{
	// General properties
	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(physicalDevice, &properties);

	int score = 0;

	// affinity
	if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		score += 1000;
	else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
		score += 100;

	// extensions
	if (!areDeviceExtensionsSupported(physicalDevice, extensions)) return -1;

	// queue families
	QueueFamilies families = findSuitableQueueFamilies(instance, physicalDevice);
	if (!families.isValid()) return -1;

	// surface support
	VkBool32 surfaceSupport;
	vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, families.present, surface, &surfaceSupport);
	if (!surfaceSupport) return -1;

	// features support
	VkPhysicalDeviceFeatures features;
	vkGetPhysicalDeviceFeatures(physicalDevice, &features);
	if (!features.geometryShader) return -1;

	// swapchain support
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
	if (formatCount == 0 || presentModeCount == 0) return -1;

	return score;
}

VkPhysicalDevice findSuitablePhysicalDevice(
	VkInstance instance, 
	const std::vector<const char*> &extensions,
	VkSurfaceKHR surface)
{
	// List all physical devices
	uint32_t physicalDeviceCount;
	vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
	std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
	vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());

	VkPhysicalDevice highestScoringPhysicalDevice;
	int highestScore = -1;

	for (auto physicalDevice : physicalDevices)
	{
		int score = getPhysicalDeviceScore(instance, extensions, surface, physicalDevice);
		if (score > highestScore)
		{
			highestScore = score;
			highestScoringPhysicalDevice = physicalDevice;
		}
	}
	if (highestScore == -1)
	{
		throw std::runtime_error("No suitable physical device");
	}
	return highestScoringPhysicalDevice;
}


VkShaderModule VulkanApp::createShaderModule(const std::string &filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) throw std::runtime_error("Can't open" + filename);
	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	VkShaderModule module;

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = buffer.size();
	createInfo.pCode = (uint32_t*)buffer.data();

	if (vkCreateShaderModule(device, &createInfo, nullptr, &module)
		!= VK_SUCCESS)
	{
		throw std::runtime_error("Can't create shader module");
	}
	return module;
}

int32_t VulkanApp::findProperties(
	VkMemoryPropertyFlags flags)
{
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
	for (int32_t i=0;i<memoryProperties.memoryTypeCount; ++i)
	{
		if ((memoryProperties.memoryTypes[i].propertyFlags & flags) == flags)
		{
			return i;
		}
	}		
	return -1;
}

uint32_t VulkanApp::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}
	throw std::runtime_error("Can't find suitable memory type");
}

VkSurfaceFormatKHR chooseSurfaceFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
	std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, surfaceFormats.data());

	if (surfaceFormats.size() == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED)
		return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

	for (auto format : surfaceFormats)
	{
		if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
			format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return format;
		}
	}
	return surfaceFormats[0];
}

VkPresentModeKHR choosePresentMode(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
	std::vector<VkPresentModeKHR> presentModes(presentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());

	for (auto presentMode : presentModes)
	{
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			return presentMode;
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseExtent(VkSurfaceCapabilitiesKHR capabilities, int width, int height)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return capabilities.currentExtent;
	}
	else
	{
		VkExtent2D actualExtent;
		actualExtent.width = std::max(capabilities.minImageExtent.width, 
			std::min(capabilities.maxImageExtent.width , (uint32_t)width ));
		actualExtent.width = std::max(capabilities.minImageExtent.height, 
			std::min(capabilities.maxImageExtent.height, (uint32_t)height));
		return actualExtent;
	}
}