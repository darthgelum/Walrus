#ifndef WALRUS_LAYER_H
#define WALRUS_LAYER_H

namespace Walrus {

	class Layer
	{
	public:
		virtual ~Layer() = default;

		virtual void OnAttach() {}
		virtual void OnDetach() {}

		virtual void OnUpdate(float ts) {}
	};

}

#endif // WALRUS_LAYER_H