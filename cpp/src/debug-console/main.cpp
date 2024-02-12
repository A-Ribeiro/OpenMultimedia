#include <InteractiveToolkit/InteractiveToolkit.h>

bool to_exit = false;

void signal_handler(int signal) {
    printf("   ****************** signal_handler **********************\n");
    to_exit = true;

    Platform::Thread::getMainThread()->interrupt();
}

int main(int argc, char* argv[]){
    Platform::Signal::Set(signal_handler);
    ITKCommon::Path::setWorkingPath(ITKCommon::Path::getExecutablePath(argv[0]));
    Platform::Thread::staticInitialization();

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

    Platform::Tool::DebugConsoleIPC console(Platform::IPC::QueueIPC_READ);
    console.runReadLoop();

    printf("main end.\n");

    Platform::Signal::Reset();
    return 0;
}
