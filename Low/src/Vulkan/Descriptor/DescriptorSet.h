#pragma once

/*
	Let' s f r i c k i n g go, descriptor sets (?)

	- BINDLESS is the way to go, at least for descriptors that change per object
		- Have a descriptor for stuff that is global to the rendering (stays the same during the whole runtime)
		- Have a descriptor for stuff that changes once per frame (camera matrices etc)
		- Use bindless descriptors for stuff that changes once per object

		- For each batch (same material, different values):
			- Create a buffer for each uniform
			- Put all the values inside it
			- Provide an index for the shader

			ISSUES
			- Uniform blocks? Maybe I could make the Material class to be a bit more expressive, so that it's also possible to push blocks instead
				of just single uniforms. That would probably solve the issue.
			- Update values? The easy answer is to just overwrite the previous buffer and recompute the offsets.
			- PushConstants? In them I should put the transform matrix and the indices for every resource instead of the resources themselves. 
				I'd also like to give the user some space for their own push constants too (maybe after all uniform buffers are specified, 
				the user can get the remaining space and decide what do with it.

	- Pass a material. Create descriptors based on them.
	- For each property, create a descriptor set. Their container is what I'd probably call a "vulkan material"

	- A runtime material contains all the descriptor sets needed to work. 
	- When processing the material (the Renderer does it, the user doesn't know about this), we start building our descriptor sets accordingly.
		Pretty easy in the general use case.
	
	- Methods of sending uniforms:
		- Push constants: just have the user send their push constant data, send it before rendering the object and you're done
		- Memory mapped: map the memory in the material instance to a uniform buffer object
		- Other: use a huge uniform buffer, keep track of the offset in the buffer. Buffers should probably be handled by something else

	Or just go bindless:
	- https://henriquegois.dev/posts/bindless-resources-in-vulkan/
	- https://vincent-p.github.io/posts/vulkan_bindless_descriptors/
	
*/
namespace Low
{
	//enum class UniformData;
	struct UniformDescription;

	class DescriptorSet
	{
	public:
		DescriptorSet() {}
		operator VkDescriptorSet() const { return m_Handle; }

	private:
		VkDescriptorSet m_Handle;
	};
}