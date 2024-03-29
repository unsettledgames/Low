#include <Vulkan/Material/VulkanMaterial.h>
#include <Vulkan/Descriptor/DescriptorSet.h>
#include <Vulkan/Descriptor/DescriptorSetLayout.h>
#include <Vulkan/VulkanCore.h>
#include <Structures/Buffer.h>

#include <Resources/Texture.h>
#include <Resources/MaterialInstance.h>

namespace Low
{
	VulkanMaterial::VulkanMaterial(Ref<Low::Material> mat)
	{
		auto data = mat->UniformsDescriptions();
		uint32_t bindingIndex = 0;
		std::vector<DescriptorSetBinding> bindings;
		
		// Create a descriptor set and a buffer for each material property
		for (auto& prop : data)
		{
			// Update layout
			DescriptorSetType bindingType;
			if (prop.second.Type != UniformDataType::Texture)
				bindingType = DescriptorSetType::Buffer;
			else
				bindingType = DescriptorSetType::Sampler;

			bindings.emplace_back(bindingType, prop.second.Stage, bindingIndex, 1);
			bindingIndex++;

			// Create buffer for this property
			uint32_t elementSize = prop.second.Size();
			uint32_t bufferSize = elementSize * 1000;

			// [TODO] This 1000 should not be a literal and could probably be obtained from the renderer, which knows EXACTLY how many
			// material instances and frames in flight there are. Btw, creating a buffer every frame? Big no no. The renderer should probably create
			// new VulkanMaterials only if necessary (and delete the old ones when they're not used anymore).
			Ref<Buffer> uniformBuffer = CreateRef<Buffer>(bufferSize, BufferUsage::Uniform);
			m_Buffers[prop.first] = uniformBuffer;
			m_BuffersLastIndex[prop.first] = 0;

			// Map buffer to memory
			vkMapMemory(VulkanCore::Device(), uniformBuffer->Memory(), 0, bufferSize, 0, &m_BuffersMapped[prop.first]);
		}

		// Create layout
		DescriptorSetLayout layout(bindings);
		m_Layouts.push_back(bindings);
		// Create set
		m_DescriptorSet = Low::DescriptorSet(m_Layouts[m_Layouts.size()-1], 1);
	}

	uint32_t VulkanMaterial::PushValue(const std::string& name, const UniformData& value)
	{
		uint32_t currIndex = m_BuffersLastIndex[name];
		memcpy((UniformData*)m_BuffersMapped[name] + currIndex, &value, sizeof(value));
		m_BuffersLastIndex[name]++;

		// [TODO]: pack descriptor writes into contiguous ones using dstArrayElement
		VkWriteDescriptorSet writeSet = {};
		writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeSet.dstSet = m_DescriptorSet;
		writeSet.dstBinding = 0;
		writeSet.dstArrayElement = currIndex;
		writeSet.descriptorCount = 1;
		writeSet.pNext = VK_NULL_HANDLE;

		if (!std::holds_alternative<Ref<Texture>>(value))
		{
			VkDescriptorBufferInfo bufferInfo = {};
			bufferInfo.buffer = *m_Buffers[name];
			bufferInfo.offset = currIndex * sizeof(value);
			bufferInfo.range = sizeof(value);

			writeSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeSet.pBufferInfo = &bufferInfo;
		}
		else
		{
			VkDescriptorImageInfo samplerInfo = {};
			Ref<Texture> texture = std::get<Ref<Texture>>(value);

			samplerInfo.sampler = *texture->Sampler();
			samplerInfo.imageView = texture->ImageView();
			samplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			writeSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeSet.pImageInfo = &samplerInfo;
		}

		m_DescriptorWrites.push_back(writeSet);

		m_BuffersLastIndex[name]++;
		return currIndex;
	}
}