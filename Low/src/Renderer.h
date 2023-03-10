#pragma once

#include <string>

namespace Low
{
	class Renderer
	{
	public:
		static void Init(const char** extensions, uint32_t nExtensions);
		static void Destroy();
	};
}