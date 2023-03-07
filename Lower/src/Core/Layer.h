#pragma once
#include <string>

namespace Low
{
	class Layer
	{
	public:
		Layer(const std::string& name) : m_Name(name) {};
		
		virtual void Init() = 0;
		virtual void Update() = 0;

	protected:
		std::string m_Name;
	};
}