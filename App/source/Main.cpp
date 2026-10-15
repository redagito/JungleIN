#include <exception>
#include <cstdlib>

#include "helpers/Logger.h"

#include "ui/Window.h"

int main(int /*argc*/, char* /*argv*/[])
{
	// Fresh log.txt next to the working directory on every start.
	fmtlog::setLogFile("log.txt", true);
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
