#pragma once

namespace Low
{
	class Camera
	{
	public:
		Camera() = default;
		Camera(const glm::mat4& view, const glm::mat4& projection) : m_View(view), m_Projection(projection) {}

		inline void SetView(const glm::mat4& view) { m_View = view; }
		inline void SetProjection(const glm::mat4& proj) { m_Projection = proj; }

		inline glm::mat4 View() { return m_View; }
		inline glm::mat4 Projection() { return m_Projection; }
		inline glm::mat4 ViewProjection() { return m_Projection * m_View; }

		inline void SetFromTransform(const glm::vec3& pos, const glm::vec3& eulerRot)
		{
			m_View = glm::inverse(glm::translate(glm::orientate4(eulerRot), pos));
		}

	private:
		glm::mat4 m_View;
		glm::mat4 m_Projection;
	};
}