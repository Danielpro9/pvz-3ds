#include "LawnApp.h"
#include "Resources.h"
#include "Sexy.TodLib/TodStringFile.h"
using namespace Sexy;

#ifdef __3DS__
#include <3ds.h>
#include <citro3d.h>
#include <malloc.h>
#include "CtrLog.h"
extern "C" {
	unsigned int __stacksize__ = 512 * 1024;
#if CTR_HEAP_MB > 0
	// Read by libctru before main(); the linear heap gets the remaining memory.
	// Sized per profile in CMakeLists.txt.
	u32 __ctru_heap_size = CTR_HEAP_MB << 20;
#endif
}
#endif

bool (*gAppCloseRequest)();				//[0x69E6A0]
bool (*gAppHasUsedCheatKeys)();			//[0x69E6A4]
SexyString (*gGetCurrentLevelName)();

//0x44E8F0
//int WINAPI WinMain(_In_ HINSTANCE /* hInstance */, _In_opt_ HINSTANCE /* hPrevInstance */, _In_ LPSTR /* lpCmdLine */, _In_ int /* nCmdShow */)
int main(int argc, char** argv)
{
#ifdef __3DS__
	CtrLogInit();
	osSetSpeedupEnable(true);
#endif

	TodStringListSetColors(gLawnStringFormats, gLawnStringFormatCount);
	gGetCurrentLevelName = LawnGetCurrentLevelName;
	gAppCloseRequest = LawnGetCloseRequest;
	gAppHasUsedCheatKeys = LawnHasUsedCheatKeys;
	gExtractResourcesByName = Sexy::ExtractResourcesByName;
	gLawnApp = new LawnApp();
	gLawnApp->Init();
	gLawnApp->Start();
	gLawnApp->Shutdown();
	if (gLawnApp)
		delete gLawnApp;

#ifdef __3DS__
	// Started in SexyAppBase::MakeWindow (platform/3ds/graphics/Window.cpp).
	C3D_Fini();
	gfxExit();
#endif

	return 0;
};
