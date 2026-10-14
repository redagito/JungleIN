#include <exception>
#include <cstdlib>

#include <fmtlog/fmtlog.h>

#include "ui/Window.h"

int main(int argc, char* argv[]) 
{
	fmtlog::startPollingThread();

#ifndef NDEBUG
	fmtlog::setLogLevel(fmtlog::LogLevel::DBG);
#endif

	try
	{
		Window window;
		window.initializeGL();

		window.run();
		return EXIT_SUCCESS;
	}
	catch (const std::exception& e)
	{
		loge("{}", e.what());
		return EXIT_FAILURE;
	}
	catch (...)
	{
		loge("Unknown non-standard exception");
		return EXIT_FAILURE;
	}
}
