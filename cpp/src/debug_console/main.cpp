#include <aRibeiroCore/aRibeiroCore.h>
#include <aRibeiroPlatform/aRibeiroPlatform.h>
using namespace aRibeiro;

bool to_exit = false;

void signal_handler(int signal) {
    printf("   ****************** signal_handler **********************\n");
    to_exit = true;

    PlatformThread::getMainThread()->interrupt();
}

int main(int argc, char* argv[]){
    PlatformSignal::Set(signal_handler);
    PlatformPath::setWorkingPath(PlatformPath::getExecutablePath(argv[0]));
    // initialize self referencing of the main thread.
    PlatformThread::getMainThread();

    printf("DEBUG CONSOLE\n"
        "\n"
        "For this is how God loved the world :\n"
        "he gave his only Son, so that everyone\n"
        "who believes in him may not perish\n"
        "but may have eternal life.\n"
        "\n"
        "John 3:16\n\n");

    //std::string path = PlatformPath::getDocumentsPath("OpenGLStarter", "RTMPBox");
    //std::string file_path = path + PlatformPath::SEPARATOR + std::string("config.cfg");

    DebugConsoleIPC console(PlatformQueueIPC_READ);
    console.runReadLoop();

    printf("main end.\n");

    PlatformSignal::Reset();
    return 0;
}
