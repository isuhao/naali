// For conditions of distribution and use, see copyright notice in LICENSE

#include "DebugOperatorNew.h"

#include "Application.h"
#include "Framework.h"
#include "LoggingFunctions.h"
#include "CoreDefines.h"

#include <QDir>

#include "Win.h"

#include "MemoryLeakCheck.h"

#ifdef ANDROID
#include <android/log.h>
#include <jni.h>

#include "StaticPluginRegistry.h"

/// \todo Eliminate the need to list static plugins explicitly here
REGISTER_STATIC_PLUGIN(OgreRenderingModule)
REGISTER_STATIC_PLUGIN(PhysicsModule)
REGISTER_STATIC_PLUGIN(EnvironmentModule)
REGISTER_STATIC_PLUGIN(TundraLogicModule)
REGISTER_STATIC_PLUGIN(AssetModule)
REGISTER_STATIC_PLUGIN(JavascriptModule)
REGISTER_STATIC_PLUGIN(AvatarModule)
REGISTER_STATIC_PLUGIN(DebugStatsModule)

#endif

int run(int argc, char **argv);

#if !defined(_MSC_VER)
// Unix entry point
int main(int argc, char **argv)
{
    return run(argc, argv);
}
#endif

#if defined(_MSC_VER)
// Windows application entry point.
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    std::string cmdLine(lpCmdLine);

    // Parse the Windows command line.
    std::vector<std::string> arguments;
    unsigned i;
    unsigned cmdStart = 0;
    unsigned cmdEnd = 0;
    bool cmd = false;
    bool quote = false;

    // Inject executable name as Framework will expect it to be there.
    // Otherwise the first param will be ignored (it assumes its the executable name).
    // In WinMain() its not included in the 'lpCmdLine' param.
    arguments.push_back("Tundra.exe");

    for(i = 0; i < cmdLine.length(); ++i)
    {
        if (cmdLine[i] == '\"')
            quote = !quote;
        if ((cmdLine[i] == ' ') && (!quote))
        {
            if (cmd)
            {
                cmd = false;
                cmdEnd = i;
                arguments.push_back(cmdLine.substr(cmdStart, cmdEnd-cmdStart));
            }
        }
        else
        {
            if (!cmd)
            {
               cmd = true;
               cmdStart = i;
            }
        }
    }
    if (cmd)
        arguments.push_back(cmdLine.substr(cmdStart, i-cmdStart));

    std::vector<const char*> argv;
    for(size_t i = 0; i < arguments.size(); ++i)
        argv.push_back(arguments[i].c_str());
    
    if (argv.size())
        return run(argv.size(), (char**)&argv[0]);
    else
        return run(0, 0);
}
#endif

#ifdef ANDROID
extern "C" {

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* /*reserved*/)
{
    __android_log_print(ANDROID_LOG_INFO,"Tundra", "Tundra JNI_OnLoad");
    JNIEnv* env;
    if (vm->GetEnv((void**)&env, JNI_VERSION_1_4) != JNI_OK)
    {
        __android_log_print(ANDROID_LOG_FATAL,"Tundra","GetEnv failed");
        return -1;
    }

    Framework::SetJavaVMInstance(vm);
    Framework::SetJniEnvInstance(env);

    return JNI_VERSION_1_4;
}

}
#endif
