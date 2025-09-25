#pragma once

extern Walrus::Application* Walrus::CreateApplication(int argc, char** argv);
bool g_ApplicationRunning = true;

namespace Walrus {

	int Main(int argc, char** argv)
	{
		Walrus::Application* app = Walrus::CreateApplication(argc, argv);
		app->Run();
		delete app;
		return 0;
	}

}

int main(int argc, char** argv)
{
	return Walrus::Main(argc, argv);
}
