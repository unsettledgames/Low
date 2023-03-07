#include <Core/Layer.h>

namespace Low
{
	class ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();

		virtual void Init() override;
		virtual void Update() override;
	};
}