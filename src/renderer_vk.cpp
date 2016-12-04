#include "renderer_vk.hpp"

#include <vector>
#include <stdexcept>
#include <string>
#include <cstring>
#include <iostream>
#include <fstream>
#include <set>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "vulkan_app.hpp"
#include "gen.hpp"

VkDeviceSize align(VkDeviceSize offset, VkDeviceSize minAlign)
{
	VkDeviceSize remainder = offset%minAlign;
	if (remainder != 0) offset += minAlign-remainder;
	return offset;
}

// Vulkan clip space correction
glm::mat4 clip(
		1.0f,  0.0f,  0.0f,  0.0f,
		0.0f, -1.0f,  0.0f,  0.0f,
		0.0f,  0.0f,  0.5f,  0.0f,
		0.0f,  0.0f,  0.5f,  1.0f);

struct ComputeUBO
{
	float dt;
	float G;
	float damping;
};
struct HdrUBO
{
	glm::vec2 flareSize;
};

void RendererVk::initWindow()
{
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
}

void RendererVk::init(GLFWwindow *window, int width, int height, SimParam params)
{
	this->params = params;
	texSize = 16;

	createInstance("NBody");
	createSurface(window);
	createDevice();
	createSwapchain(width, height);

	createRenderPass();

	createFramebuffers(renderPass);

	createCommandPool();

	createResources();
	createDescriptorLayouts();
	createDescriptorPool();
	allocateDescriptorSets();
	updateDescriptors();

	createGraphicsPipeline();
	createComputePipelines();

	createComputeCmdBuffer();
	recordCompute();

	// Semaphores
	VkSemaphoreCreateInfo semaphoreInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
	vkCreateSemaphore(device, &semaphoreInfo, nullptr, &computeFinished);
	vkCreateSemaphore(device, &semaphoreInfo, nullptr, &drawingFinished);

	// Signal drawingFinished
	VkCommandBuffer once = beginOnce();
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &drawingFinished;
	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	endOnce(once);

}

void RendererVk::createRenderPass()
{
	// Attachments
	std::vector<VkAttachmentDescription> attachments;

	// attach 0 : hdr rendering
	uint32_t attachmentCount = 1;

	VkFormat formats[] = {
		swapchain.format};
	VkAttachmentLoadOp loadOps[] = {
		VK_ATTACHMENT_LOAD_OP_CLEAR
	};
	VkAttachmentStoreOp storeOps[] = {
		VK_ATTACHMENT_STORE_OP_STORE
	};

	for (uint32_t i=0;i<attachmentCount;++i)
	{
		VkAttachmentDescription desc = {};
		desc.format = formats[i];
		desc.samples = VK_SAMPLE_COUNT_1_BIT;
		desc.loadOp = loadOps[i];
		desc.storeOp = storeOps[i];
		desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		desc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		attachments.push_back(desc);
	}	

	// Subpasses
	std::vector<VkSubpassDescription> subpasses;
	VkSubpassDescription subpass;

	// subpass attachments
	VkAttachmentReference hdr_attach = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

	// HDR rendering subpass
	subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &hdr_attach;
	subpasses.push_back(subpass);

	// Subpass dependencies
	std::vector<VkSubpassDependency> dependencies;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies.push_back(dependency);

	VkRenderPassCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.attachmentCount = attachments.size();
	createInfo.pAttachments = attachments.data();
	createInfo.subpassCount = subpasses.size();
	createInfo.pSubpasses = subpasses.data();
	createInfo.dependencyCount = dependencies.size();
	createInfo.pDependencies = dependencies.data();

	if (vkCreateRenderPass(device, &createInfo, nullptr, &renderPass)
		!= VK_SUCCESS)
	{
		throw std::runtime_error("Can't create renderpass");
	}
}

void RendererVk::createResources()
{
	// Figure out offsets for sub buffer allocations
	VkDeviceSize minAlignUBO  = deviceProperties.limits.minUniformBufferOffsetAlignment;
	VkDeviceSize minAlignSSBO = deviceProperties.limits.minStorageBufferOffsetAlignment;

	hdrUBOOffset = 0;
	hdrUBOSize = sizeof(HdrUBO);
	computeUBOOffset = align(hdrUBOOffset+hdrUBOSize, minAlignUBO);
	computeUBOSize = sizeof(ComputeUBO);
	computePositionSSBOOffset = align(computeUBOOffset+computeUBOSize, minAlignSSBO);
	computePositionSSBOSize = sizeof(glm::vec4)*params.numParticles;
	computeVelocitySSBOOffset = align(computePositionSSBOOffset+computePositionSSBOSize, minAlignSSBO);
	computeVelocitySSBOSize = sizeof(glm::vec4)*params.numParticles;

	// Deduce buffer size from last member + its size
	bufferSize = computeVelocitySSBOOffset+computePositionSSBOSize;

	// Create buffer
	createBuffers();

	// Fill uniform buffer data
	ComputeUBO computeParam;

	computeParam.dt = params.dt;
	computeParam.G = params.G;
	computeParam.damping = params.damping;

	HdrUBO hdrParam;
	hdrParam.flareSize = glm::vec2(
		texSize/float(2*swapchain.extent.width),
		texSize/float(2*swapchain.extent.height));

	void *data;
	vkMapMemory(device, hostMemory, computeUBOOffset, computeUBOSize, 0, &data);
	memcpy(data, &computeParam, computeUBOSize);
	vkUnmapMemory(device, hostMemory);

	vkMapMemory(device, hostMemory, hdrUBOOffset, hdrUBOSize, 0, &data);
	memcpy(data, &hdrParam, hdrUBOSize);
	vkUnmapMemory(device, hostMemory);

	// Flare texture
	createFlare();
}

void RendererVk::createBuffers()
{
	// local buffer
	createBuffer(bufferSize, 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT   | 
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT  |
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | 
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		localBuffer,
		localMemory);

	// Staging buffer
	createBuffer(bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		hostBuffer,
		hostMemory);
}

void RendererVk::createFlare()
{
	VkExtent3D extent = {
		(uint32_t)texSize,
		(uint32_t)texSize,
		1};

	createImage(
		VK_IMAGE_TYPE_2D,
		VK_FORMAT_R32_SFLOAT,
		extent,
		1,
		1,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_LINEAR,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		VK_IMAGE_LAYOUT_PREINITIALIZED,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		flareImageStaging,
		flareMemoryStaging);

	createImage(
		VK_IMAGE_TYPE_2D,
		VK_FORMAT_R32_SFLOAT,
		extent,
		1,
		1,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		flareImage,
		flareMemory);

	// Data copy to staging image
	std::vector<float> pixelData = genFlareTex(texSize);
	void *data;
	vkMapMemory(device, flareMemoryStaging, 0, pixelData.size()*sizeof(float), 0, &data);
	memcpy(data, pixelData.data(), pixelData.size()*sizeof(float));
	vkUnmapMemory(device, flareMemoryStaging);

	// Transitions
	transitionImageLayout(flareImageStaging, 
		VK_IMAGE_LAYOUT_PREINITIALIZED, 
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		0,
		VK_ACCESS_TRANSFER_READ_BIT,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT);

	transitionImageLayout(flareImage,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		0,
		0,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT);

	// Staging image copy
	copyImage(flareImageStaging, flareImage, extent);

	// Final transition
	transitionImageLayout(flareImage,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

	// Image view
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = flareImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = VK_FORMAT_R32_SFLOAT;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;
	if (vkCreateImageView(device, &viewInfo, nullptr, &flareView)
		!= VK_SUCCESS)
	{
		throw std::runtime_error("Can't create flare image view");
	}

	// Sampler
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;
	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	if (vkCreateSampler(device, &samplerInfo, nullptr, &flareSampler)
		!= VK_SUCCESS)
	{
		throw std::runtime_error("Can't create flare sampler");
	}
}

VkCommandBuffer RendererVk::beginOnce()
{
	//Allocate 1 command buffer
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer once;

	vkAllocateCommandBuffers(device, &allocInfo, &once);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(once, &beginInfo);

	return once;
}

void RendererVk::endOnce(VkCommandBuffer once)
{
	vkEndCommandBuffer(once);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &once;

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);
	vkFreeCommandBuffers(device, commandPool, 1, &once);
}

void RendererVk::copyBuffer()
{
	VkCommandBuffer once = beginOnce();

	// Staging buffer copy
	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = bufferSize;
	vkCmdCopyBuffer(once, hostBuffer, localBuffer, 1, &copyRegion);

	endOnce(once);
}

void RendererVk::transitionImageLayout(
	VkImage image,
	VkImageLayout oldLayout,
	VkImageLayout newLayout,
	VkAccessFlags srcAccessMask,
	VkAccessFlags dstAccessMask,
	VkPipelineStageFlagBits srcStageMask,
	VkPipelineStageFlagBits dstStageMask
	)
{
	VkCommandBuffer once = beginOnce();

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = srcAccessMask;
	barrier.dstAccessMask = dstAccessMask;

	vkCmdPipelineBarrier(
		once, 
		srcStageMask,
		dstStageMask,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	endOnce(once);
}

void RendererVk::copyImage(VkImage srcImage, VkImage dstImage, VkExtent3D extent)
{
	VkCommandBuffer once = beginOnce();

	VkImageSubresourceLayers subResource = {};
	subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subResource.baseArrayLayer = 0;
	subResource.mipLevel = 0;
	subResource.layerCount = 1;

	VkImageCopy region = {};
	region.srcSubresource = subResource;
	region.dstSubresource = subResource;
	region.srcOffset = {0, 0, 0};
	region.dstOffset = {0, 0, 0};
	region.extent = extent;

	vkCmdCopyImage(
		once,
		srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &region);

	endOnce(once);
}

void RendererVk::createDescriptorLayouts()
{
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	VkDescriptorSetLayoutBinding binding;
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

	// HDR Descriptor Set layout
	bindings.clear();
	// Parameter ubo
	binding = {};
	binding.binding = 0;
	binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	binding.descriptorCount = 1;
	binding.stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT;
	bindings.push_back(binding);
	
	// Flare texture
	binding = {};
	binding.binding = 1;
	binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	binding.descriptorCount = 1;
	binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings.push_back(binding);

	layoutInfo.bindingCount = bindings.size();
	layoutInfo.pBindings = bindings.data();
	
	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &hdrSetLayout)
		!= VK_SUCCESS)
	{
		throw std::runtime_error("Can't create HDR descriptor set layout");
	}

	// Compute Descriptor Set Layout
	bindings.clear();
	// Parameters UBO
	binding = {};
	binding.binding = 0;
	binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	binding.descriptorCount = 1;
	binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	bindings.push_back(binding);
	// Particle position SSBO
	binding = {};
	binding.binding = 1;
	binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	binding.descriptorCount = 1;
	binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	bindings.push_back(binding);
	// Particle velocity SSBO
	binding = {};
	binding.binding = 2;
	binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	binding.descriptorCount = 1;
	binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	bindings.push_back(binding);
	
	layoutInfo.bindingCount = bindings.size();
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &computeSetLayout)
		!= VK_SUCCESS)
	{
		throw std::runtime_error("Can't create compute descriptor set layout");
	}
}

void RendererVk::createDescriptorPool()
{
	// Pool sizes
	std::vector<VkDescriptorPoolSize> poolSizes;
	poolSizes.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2});
	poolSizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2});
	poolSizes.push_back({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1});

	VkDescriptorPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.maxSets = 2;
	createInfo.poolSizeCount = poolSizes.size();
	createInfo.pPoolSizes = poolSizes.data();

	if (vkCreateDescriptorPool(device, &createInfo, nullptr, &descriptorPool)
		!= VK_SUCCESS)
	{
		throw std::runtime_error("Can't create descriptor pool");
	}
}

void RendererVk::allocateDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> layouts = {hdrSetLayout, computeSetLayout};
	descriptorSets.resize(layouts.size());

	VkDescriptorSetAllocateInfo allocateInfo = {};
	allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocateInfo.descriptorPool = descriptorPool;
	allocateInfo.descriptorSetCount = descriptorSets.size();
	allocateInfo.pSetLayouts = layouts.data();

	if (vkAllocateDescriptorSets(device, &allocateInfo, descriptorSets.data())
		!= VK_SUCCESS)
	{
		throw std::runtime_error("Can't allocate descriptor sets");
	}
}

void RendererVk::updateDescriptors()
{
	std::vector<VkWriteDescriptorSet> writes;
	VkWriteDescriptorSet write;

	// HDR parameter UBO
	VkDescriptorBufferInfo hdrParamUBOInfo = {
		localBuffer, 
		hdrUBOOffset, 
		hdrUBOSize};

	write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = descriptorSets.at(0);
	write.dstBinding = 0;
	write.dstArrayElement = 0;
	write.descriptorCount = 1;
	write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write.pBufferInfo = &hdrParamUBOInfo;
	writes.push_back(write);

	// HDR Texture
	VkDescriptorImageInfo imageInfo = {flareSampler, flareView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
	write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = descriptorSets.at(0);
	write.dstBinding = 1;
	write.dstArrayElement = 0;
	write.descriptorCount = 1;
	write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write.pImageInfo = &imageInfo;
	writes.push_back(write);

	// Compute Parameters UBO
	VkDescriptorBufferInfo computeParamInfo = {
		localBuffer,
		computeUBOOffset,
		computeUBOSize};
	write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = descriptorSets.at(1);
	write.dstBinding = 0;
	write.dstArrayElement = 0;
	write.descriptorCount = 1;
	write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write.pBufferInfo = &computeParamInfo;
	writes.push_back(write);

	// Compute Position SSBO
	VkDescriptorBufferInfo computePositionInfo = {
		localBuffer,
		computePositionSSBOOffset,
		computePositionSSBOSize};
	write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = descriptorSets.at(1);
	write.dstBinding = 1;
	write.dstArrayElement = 0;
	write.descriptorCount = 1;
	write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	write.pBufferInfo = &computePositionInfo;
	writes.push_back(write);

	// Compute Velocity SSBO
	VkDescriptorBufferInfo computeVelocityInfo = {
		localBuffer,
		computeVelocitySSBOOffset,
		computeVelocitySSBOSize};
	write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = descriptorSets.at(1);
	write.dstBinding = 2;
	write.dstArrayElement = 0;
	write.descriptorCount = 1;
	write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	write.pBufferInfo = &computeVelocityInfo;
	writes.push_back(write);

	vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
}

void RendererVk::createGraphicsPipeline()
{
	// Shader stages
	shaders.push_back(createShaderModule("shaders/vk/main_vert.spv"));
	shaders.push_back(createShaderModule("shaders/vk/main_geom.spv"));
	shaders.push_back(createShaderModule("shaders/vk/main_frag.spv"));

	VkPipelineShaderStageCreateInfo vertShader = {};
	vertShader.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShader.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShader.module = shaders.at(0);
	vertShader.pName = "main";

	VkPipelineShaderStageCreateInfo geomShader = {};
	geomShader.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	geomShader.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
	geomShader.module = shaders.at(1);
	geomShader.pName = "main";

	VkPipelineShaderStageCreateInfo fragShader = {};
	fragShader.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShader.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShader.module = shaders.at(2);
	fragShader.pName = "main";

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {vertShader, geomShader, fragShader};

	// Vertex input state
	std::vector<VkVertexInputBindingDescription> bindings = {
		{0, sizeof(glm::vec4), VK_VERTEX_INPUT_RATE_VERTEX},
		{1, sizeof(glm::vec4), VK_VERTEX_INPUT_RATE_VERTEX},
	};
	std::vector<VkVertexInputAttributeDescription> attributes = {
		{0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0}, // position vec4
		{1, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 0}  // velocity vec4
	};

	VkPipelineVertexInputStateCreateInfo vis = {};
	vis.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vis.vertexBindingDescriptionCount = bindings.size();
	vis.pVertexBindingDescriptions = bindings.data();
	vis.vertexAttributeDescriptionCount = attributes.size();
	vis.pVertexAttributeDescriptions = attributes.data();

	// Input assembly state
	VkPipelineInputAssemblyStateCreateInfo ias = {};
	ias.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	ias.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;

	// Viewport state
	VkViewport viewport = {0, 0, 
		(float)swapchain.extent.width, (float)swapchain.extent.height, 0, 1};
	VkRect2D scissor = {{0,0}, {swapchain.extent.width, swapchain.extent.height}};

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	// Rasterization state
	VkPipelineRasterizationStateCreateInfo rasterState = {};
	rasterState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterState.polygonMode = VK_POLYGON_MODE_FILL;
	rasterState.cullMode = VK_CULL_MODE_FRONT_BIT;
	rasterState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterState.lineWidth = 1.0f;

	// Multisample state
	VkPipelineMultisampleStateCreateInfo multisampleState = {};
	multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleState.sampleShadingEnable = VK_FALSE;

	// Color blend state
	VkPipelineColorBlendAttachmentState attachment = {};
	attachment.blendEnable = VK_TRUE;
	attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	attachment.colorBlendOp = VK_BLEND_OP_ADD;
	attachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT ;

	VkPipelineColorBlendStateCreateInfo blendState = {};
	blendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blendState.attachmentCount = 1;
	blendState.pAttachments = &attachment;

	// Pipeline layout
	// Push Constant ranges
	std::vector<VkPushConstantRange> ranges = {
		{VK_SHADER_STAGE_VERTEX_BIT,     0,  64},
		{VK_SHADER_STAGE_VERTEX_BIT,    64, 128}
	};

	VkPipelineLayoutCreateInfo layoutCreateInfo = {};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutCreateInfo.setLayoutCount = 1;
	layoutCreateInfo.pSetLayouts = &hdrSetLayout;
	layoutCreateInfo.pushConstantRangeCount = ranges.size();
	layoutCreateInfo.pPushConstantRanges = ranges.data();

	if (vkCreatePipelineLayout(device, &layoutCreateInfo, nullptr, &hdrLayout)
		!= VK_SUCCESS)
	{
		throw std::runtime_error("Can't create pipeline layout");
	}

	VkGraphicsPipelineCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	createInfo.stageCount = shaderStages.size();
	createInfo.pStages = shaderStages.data();
	createInfo.pVertexInputState = &vis;
	createInfo.pInputAssemblyState = &ias;
	createInfo.pViewportState = &viewportState;
	createInfo.pRasterizationState = &rasterState;
	createInfo.pMultisampleState = &multisampleState;
	createInfo.pColorBlendState = &blendState;
	createInfo.layout = hdrLayout;
	createInfo.renderPass = renderPass;
	createInfo.subpass = 0;

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &graphicsPipeline)
		!= VK_SUCCESS)
	{
		throw std::runtime_error("Can't create graphics Pipeline");
	}
}

void RendererVk::createComputePipelines()
{
	std::array<const char*, 2> shaderFilenames = {
		"shaders/vk/interaction_comp.spv",
		"shaders/vk/integration_comp.spv" };

	VkPipelineLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = 1;
	layoutInfo.pSetLayouts = &computeSetLayout;

	if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &computeLayout)
		!= VK_SUCCESS)
	{
		throw std::runtime_error("Can't create compute pipeline layout");
	}

	for (int i=0;i<computePipelines.size();++i)
	{
		shaders.push_back(createShaderModule(shaderFilenames.at(i)));

		VkPipelineShaderStageCreateInfo stageInfo = {};
		stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		stageInfo.module = shaders.back();
		stageInfo.pName = "main";

		VkComputePipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.stage = stageInfo;
		pipelineInfo.layout = computeLayout;

		if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipelines[i])
			!= VK_SUCCESS)
		{
			throw std::runtime_error("Can't create compute pipeline");
		}
	}
}

void RendererVk::createComputeCmdBuffer()
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(device, &allocInfo, &computeCmdBuffer)
		!= VK_SUCCESS)
	{
		throw std::runtime_error("Can't allocate compute cmd buffer");
	}
}

void RendererVk::recordCompute()
{
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	// barrier between interaction/integration and vice versa
	VkBufferMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.buffer = localBuffer;
	barrier.offset = 0;
	barrier.size = sizeof(glm::vec4)*params.numParticles*2;

	vkBeginCommandBuffer(computeCmdBuffer, &beginInfo);

	// Common descriptor set
	vkCmdBindDescriptorSets(
		computeCmdBuffer, 
		VK_PIPELINE_BIND_POINT_COMPUTE, 
		computeLayout,
		0,
		1, &descriptorSets.at(1),
		0, nullptr);

	for (size_t i=0;i<params.maxIterationsPerFrame;++i)
	{
		// Integration/Interaction barrier
		vkCmdPipelineBarrier(computeCmdBuffer,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			0,
			0, nullptr,
			1, &barrier,
			0, nullptr);

		// Interaction
		vkCmdBindPipeline(computeCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelines[0]);
		vkCmdDispatch(computeCmdBuffer, params.numParticles/256, 1, 1);

		// Interaction/Integration barrier
		vkCmdPipelineBarrier(computeCmdBuffer,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			0,
			0, nullptr,
			1, &barrier,
			0, nullptr);

		// Integration
		vkCmdBindPipeline(computeCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelines[1]);
		vkCmdDispatch(computeCmdBuffer, params.numParticles/256, 1, 1);
	}

	vkEndCommandBuffer(computeCmdBuffer);
}

void RendererVk::submitCompute()
{
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	std::vector<VkSemaphore> waitSemaphores = {drawingFinished};
	std::vector<VkPipelineStageFlags> waitStages = {
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT};

	std::vector<VkSemaphore> signalSemaphores = {computeFinished};

	submitInfo.waitSemaphoreCount = waitSemaphores.size();
	submitInfo.pWaitSemaphores = waitSemaphores.data();
	submitInfo.pWaitDstStageMask = waitStages.data();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &computeCmdBuffer;
	submitInfo.signalSemaphoreCount = signalSemaphores.size();
	submitInfo.pSignalSemaphores = signalSemaphores.data();

	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
	{
		throw std::runtime_error("Can't submit command buffer");
	}
}

void RendererVk::recordDrawing(uint32_t imageIndex)
{
	// Wait on command completion
	vkWaitForFences(device, 1, &swapchain.fences[imageIndex], VK_TRUE, std::numeric_limits<uint64_t>::max());
	vkResetFences(device, 1, &swapchain.fences[imageIndex]);

	auto &cb = swapchain.commandBuffers[imageIndex];

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	vkBeginCommandBuffer(cb, &beginInfo);

	// barrier with compute
	VkBufferMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.buffer = localBuffer;
	barrier.offset = 0;
	barrier.size = sizeof(glm::vec4)*params.numParticles*2;

	vkCmdPipelineBarrier(cb,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
		0,
		0, nullptr,
		1, &barrier,
		0, nullptr);

	// Render pass begin
	VkClearValue clear = {0.0,0.0,0.0,0.0};

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = swapchain.framebuffers[imageIndex];
	renderPassInfo.renderArea.offset = {0,0};
	renderPassInfo.renderArea.extent = swapchain.extent;
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clear;
	vkCmdBeginRenderPass(cb, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	// Pipeline bind
	vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

	// Vertex buffer bind
	VkDeviceSize offsets[] = {0, sizeof(glm::vec4)*params.numParticles};
	VkBuffer buffers[] = {localBuffer, localBuffer};
	vkCmdBindVertexBuffers(cb, 0, 2, buffers, offsets);

	// Push constants
	vkCmdPushConstants(cb, hdrLayout, 
		VK_SHADER_STAGE_VERTEX_BIT  ,   0, 64, glm::value_ptr(viewMat));

	vkCmdPushConstants(cb, hdrLayout, 
		VK_SHADER_STAGE_VERTEX_BIT  ,  64, 64, glm::value_ptr(projMat));

	// Descriptor sets
	vkCmdBindDescriptorSets(
		cb, 
		VK_PIPELINE_BIND_POINT_GRAPHICS, 
		hdrLayout, 
		0, 
		1, &descriptorSets.at(0),
		0, nullptr);

	// Draw
	vkCmdDraw(cb, params.numParticles, 1, 0, 0);

	vkCmdEndRenderPass(cb);

	if (vkEndCommandBuffer(cb) != VK_SUCCESS)
	{
		throw std::runtime_error("Can't record command buffer");
	}
}

void RendererVk::submitDrawing(uint32_t imageIndex)
{
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	std::vector<VkSemaphore> waitSemaphores = {swapchain.imageAvailable, computeFinished};
	std::vector<VkPipelineStageFlags> waitStages = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_VERTEX_INPUT_BIT};
	std::vector<VkSemaphore> signalSemaphores = {swapchain.renderFinished, drawingFinished};

	submitInfo.waitSemaphoreCount = waitSemaphores.size();
	submitInfo.pWaitSemaphores = waitSemaphores.data();
	submitInfo.pWaitDstStageMask = waitStages.data();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &swapchain.commandBuffers[imageIndex];
	submitInfo.signalSemaphoreCount = signalSemaphores.size();
	submitInfo.pSignalSemaphores = signalSemaphores.data();

	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, swapchain.fences[imageIndex]) != VK_SUCCESS)
	{
		throw std::runtime_error("Can't submit command buffer");
	}
}

void RendererVk::destroy()
{
	vkDeviceWaitIdle(device);

	vkDestroySemaphore(device, computeFinished, nullptr);
	vkDestroySemaphore(device, drawingFinished, nullptr);

	vkDestroyImageView(device, flareView, nullptr);
	vkDestroySampler(device, flareSampler, nullptr);

	// Free buffer
	vkFreeMemory(device, hostMemory, nullptr);
	vkFreeMemory(device, localMemory, nullptr);
	vkDestroyBuffer(device, hostBuffer, nullptr);
	vkDestroyBuffer(device, localBuffer, nullptr);

	// Free flare image
	vkFreeMemory(device, flareMemory, nullptr);
	vkFreeMemory(device, flareMemoryStaging, nullptr);
	vkDestroyImage(device, flareImage, nullptr);
	vkDestroyImage(device, flareImageStaging, nullptr);
	
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);

	vkFreeCommandBuffers(device, commandPool, 1, &computeCmdBuffer);

	for (auto shader : shaders)
		vkDestroyShaderModule(device, shader, nullptr);

	vkDestroyPipeline(device, graphicsPipeline, nullptr);
	for (auto pipeline : computePipelines)
		vkDestroyPipeline(device, pipeline, nullptr);

	vkDestroyPipelineLayout(device, hdrLayout, nullptr);
	vkDestroyPipelineLayout(device, computeLayout, nullptr);
	vkDestroyDescriptorSetLayout(device, hdrSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(device, computeSetLayout, nullptr);

	vkDestroyRenderPass(device, renderPass, nullptr);

	VulkanApp::destroy();
}

void RendererVk::populateParticles( 
	const std::vector<glm::vec4> pos, 
	const std::vector<glm::vec4> vel)
{
	if (pos.size() != vel.size() || pos.size() != params.numParticles)
		throw std::runtime_error("Particle buffers have invalid sizes");

	// Fill staging buffers
	void *data;
	vkMapMemory(device, hostMemory, computePositionSSBOOffset, computePositionSSBOSize, 0, (void**)&data);
	memcpy(data, pos.data(), computePositionSSBOSize);
	vkUnmapMemory(device, hostMemory);

	vkMapMemory(device, hostMemory, computeVelocitySSBOOffset, computeVelocitySSBOSize, 0, (void**)&data);
	memcpy(data, vel.data(), computeVelocitySSBOSize);
	vkUnmapMemory(device, hostMemory);

	// Copy
	copyBuffer();
}

void RendererVk::stepSim()
{
	submitCompute();
}

void RendererVk::render(glm::mat4 proj_mat, glm::mat4 view_mat)
{
	uint32_t imageIndex = acquireImage();

	projMat = clip*proj_mat;
	viewMat = view_mat;

	recordDrawing(imageIndex);
	submitDrawing(imageIndex);

	present(imageIndex);
}