#include <exception>
#include <cstdio>
#include <cstdlib>

#include "helpers/Logger.h"
#include "helpers/Utils.h"

#include "ui/Window.h"

namespace {
// Mirror every log record to the console, unbuffered. The file sink stays
// buffered; the callback fires as messages are polled.
void ConsoleLogCallback(int64_t /*ns*/, fmtlog::LogLevel /*level*/,
                        fmt::string_view /*location*/, size_t /*basePos*/,
                        fmt::string_view /*threadName*/, fmt::string_view msg,
                        size_t /*bodyPos*/, size_t /*logFilePos*/) {
  std::fwrite(msg.data(), sizeof(char), msg.size(), stdout);
  if (msg.size() == 0 || msg[msg.size() - 1] != '\n')
    std::fputc('\n', stdout);
  std::fflush(stdout);
}
}

int main(int /*argc*/, char* /*argv*/[])
{
	// Fresh log.txt next to the working directory on every start.
	fmtlog::setLogFile("log.txt", true);
	fmtlog::setLogCB(&ConsoleLogCallback, fmtlog::DBG);
	fmtlog::startPollingThread();
	// Errors flush immediately so a crash never loses its own log tail.
	fmtlog::flushOn(fmtlog::ERR);

#ifndef NDEBUG
	fmtlog::setLogLevel(fmtlog::LogLevel::DBG);
#endif

	try
	{
		Timer::mark("process start");
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
