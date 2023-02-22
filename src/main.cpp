#include <g3log/g3log.hpp>
#include <g3log/logworker.hpp>
#include "StandardOutSink.h"
#include "cgr_error.h"
#include "hello.h"

int main(int argc, char* argv[])
{
    // Init g3log
    //
    auto logWorker = g3::LogWorker::createLogWorker();
    const std::string logfilePath = ".";

    auto logHandle = logWorker->addDefaultLogger("Hello-Diligent", logfilePath);
    g3::initializeLogging(logWorker.get());

    // add our stdout sink to logger
    auto stdOutHandle = logWorker->addSink(std::make_unique<cgr::StandardOutSink>(g3::kDebugValue),
                                           &cgr::StandardOutSink::ReceiveLogMessage);

    LOG(INFO) << "Hello-Diligent Started";

    try
    {
        // Run the Diligent App
        HelloDiligent app;
        return app.Run();
    }
    catch (const cgrebel::Error& e)
    {
        // log using g3 manually to show the exception source location information
        LogCapture(e.file.c_str(), e.line, e.function.c_str(), WARNING).stream() << e.what();
        return EXIT_FAILURE;
    }
    catch (const std::exception& e)
    {
        LOG(WARNING) << e.what();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
