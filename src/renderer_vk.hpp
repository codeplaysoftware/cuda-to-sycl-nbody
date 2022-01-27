// Copyright (C) 2016 - 2018 Sarah Le Luron

#pragma once

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <array>

#include "renderer.hpp"
#include "vulkan_app.hpp"

class RendererVk : public Renderer, VulkanApp {
  public:
   void initWindow();
   void init(GLFWwindow *window, int width, int height, SimParam params);
   void destroy();
   void populateParticles(const std::vector<glm::vec4> pos,
                          const std::vector<glm::vec4> vel);
   void stepSim();
   void render(glm::mat4 proj_mat, glm::mat4 view_mat);

  private:
   void createRenderPass();
   void createResources();
   void createVertexBuffers();
   void createDescriptorLayouts();
   void createDescriptorPool();
   void allocateDescriptorSets();
   void updateDescriptors();
   void createGraphicsPipeline();
   void createComputePipelines();

   // Resource creation
   void createBuffers();
   void createFlare();

   // copy staging buffer to actual buffer
   void copyBuffer();
   // transition images
   void transitionImageLayout(VkImage image, VkImageLayout oldLayout,
                              VkImageLayout newLayout,
                              VkAccessFlags srcAccessMask,
                              VkAccessFlags dstAccessMask,
                              VkPipelineStageFlagBits srcStageMask,
                              VkPipelineStageFlagBits dstStageMask);

   void copyImage(VkImage srcImage, VkImage dstImage, VkExtent3D extent);

   // Helpers for sending commands once
   VkCommandBuffer beginOnce();
   void endOnce(VkCommandBuffer once);

   // Compute command buffer
   void createComputeCmdBuffer();
   void recordCompute();
   void submitCompute();

   // Drawing command buffer record & submit
   void recordDrawing(uint32_t imageIndex);
   void submitDrawing(uint32_t imageIndex);

   VkDescriptorSetLayout hdrSetLayout;
   VkDescriptorSetLayout computeSetLayout;

   std::vector<VkShaderModule> shaders;
   VkDescriptorPool descriptorPool;
   std::vector<VkDescriptorSet> descriptorSets;

   VkDeviceSize bufferSize;
   VkDeviceMemory hostMemory;
   VkBuffer hostBuffer;
   VkDeviceMemory localMemory;
   VkBuffer localBuffer;

   // Sub-buffer offsets
   VkDeviceSize computePositionSSBOOffset;
   VkDeviceSize computePositionSSBOSize;
   VkDeviceSize computeVelocitySSBOOffset;
   VkDeviceSize computeVelocitySSBOSize;
   VkDeviceSize computeUBOOffset;
   VkDeviceSize computeUBOSize;
   VkDeviceSize hdrUBOOffset;
   VkDeviceSize hdrUBOSize;

   // Flare texture
   VkImage flareImage;
   VkDeviceMemory flareMemory;
   VkImageView flareView;
   VkSampler flareSampler;
   VkImage flareImageStaging;
   VkDeviceMemory flareMemoryStaging;

   // Graphics renderpass & pipeline
   VkRenderPass renderPass;
   VkPipeline graphicsPipeline;
   VkPipelineLayout hdrLayout;

   // Compute pipelines
   std::array<VkPipeline, 2>
       computePipelines;  // 0 for interaction, 1 for integration
   VkPipelineLayout computeLayout;

   // Command buffer for compute operations
   VkCommandBuffer computeCmdBuffer;

   // Semaphores between compute operations & drawing operations
   VkSemaphore computeFinished;
   VkSemaphore drawingFinished;

   GLFWwindow *window;

   SimParam params;
   glm::mat4 projMat;
   glm::mat4 viewMat;
   int texSize;
};