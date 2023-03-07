#include <Core/Layer.h>

namespace Low
{
	class RenderingLayer : public Layer
	{
	public:
		RenderingLayer();

		virtual void Init() override;
		virtual void Update() override;
	};
}