#include "dxvk_descriptor.h"

namespace dxvk {
  
  DxvkDescriptorAlloc::DxvkDescriptorAlloc(
    const Rc<vk::DeviceFn>& vkd)
  : m_vkd(vkd) {
    TRACE(this);
  }
  
  
  DxvkDescriptorAlloc::~DxvkDescriptorAlloc() {
    TRACE(this);
    
    for (auto p : m_pools) {
      m_vkd->vkDestroyDescriptorPool(
        m_vkd->device(), p, nullptr);
    }
  }
  
  
  VkDescriptorSet DxvkDescriptorAlloc::alloc(VkDescriptorSetLayout layout) {
    TRACE(this, layout);
    
    VkDescriptorSet set = VK_NULL_HANDLE;
    
    if (m_poolId < m_pools.size())
      set = this->allocFrom(m_pools.at(m_poolId), layout);
    
    if (set == VK_NULL_HANDLE) {
      VkDescriptorPool pool = this->createDescriptorPool();
      set = this->allocFrom(pool, layout);
      
      if (set == VK_NULL_HANDLE)
        throw DxvkError("DxvkDescriptorAlloc::alloc: Failed to allocate descriptor set");
      
      m_pools.push_back(pool);
      m_poolId += 1;
    }
      
    return set;
  }
  
  
  void DxvkDescriptorAlloc::reset() {
    for (auto p : m_pools) {
      m_vkd->vkResetDescriptorPool(
        m_vkd->device(), p, 0);
    }
    
    m_poolId = 0;
  }
  
  
  VkDescriptorPool DxvkDescriptorAlloc::createDescriptorPool() {
    TRACE(this);
    
    // TODO tune these values, if necessary
    constexpr uint32_t MaxSets = 64;
    constexpr uint32_t MaxDesc = 256;
    
    std::array<VkDescriptorPoolSize, 7> pools = {{
      { VK_DESCRIPTOR_TYPE_SAMPLER,               MaxDesc },
      { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,         MaxDesc },
      { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,         MaxDesc },
      { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,        MaxDesc },
      { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,        MaxDesc },
      { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,  MaxDesc },
      { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,  MaxDesc } }};
    
    VkDescriptorPoolCreateInfo info;
    info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.pNext         = nullptr;
    info.flags         = 0;
    info.maxSets       = MaxSets;
    info.poolSizeCount = pools.size();
    info.pPoolSizes    = pools.data();
    
    VkDescriptorPool pool = VK_NULL_HANDLE;
    if (m_vkd->vkCreateDescriptorPool(m_vkd->device(),
          &info, nullptr, &pool) != VK_SUCCESS)
      throw DxvkError("DxvkDescriptorAlloc::createDescriptorPool: Failed to create descriptor pool");
    return pool;
  }
  
  
  VkDescriptorSet DxvkDescriptorAlloc::allocFrom(
          VkDescriptorPool      pool,
          VkDescriptorSetLayout layout) const {
    VkDescriptorSetAllocateInfo info;
    info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    info.pNext              = nullptr;
    info.descriptorPool     = pool;
    info.descriptorSetCount = 1;
    info.pSetLayouts        = &layout;
    
    VkDescriptorSet set = VK_NULL_HANDLE;
    if (m_vkd->vkAllocateDescriptorSets(m_vkd->device(), &info, &set) != VK_SUCCESS)
      return VK_NULL_HANDLE;
    return set;
  }
  
}