#include <3ds.h>
#include <exception>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "CtrLog.h"
#include "misc/SexyLog.h"

// The 3DS log sink (see SexyLog.h): every engine log line and printf goes to
// log.txt on the SD card and is echoed to the top-screen console.
//
// The path is fixed rather than <appdata>/userdata/log.txt so the log records
// from the first line of main(), before SexyAppBase::Init has chosen the app
// data folder. The previous run is kept as log_old.txt, so relaunching after a
// crash does not overwrite the log that explains it.
#define CTR_LOG_DIR      "sdmc:/3ds/PlantsvsZombies"
#define CTR_LOG_PATH     CTR_LOG_DIR "/log.txt"
#define CTR_LOG_OLD_PATH CTR_LOG_DIR "/log_old.txt"

// Unbuffered, so the FILE lock serializes whole lines from the main and the
// loading thread, and nothing is left in a buffer when the process dies.
static FILE* sLogFile = NULL;

// The CPU exception handler runs on this stack: the faulting one may be the
// reason it crashed.
static u8 sExceptionStack[0x4000] __attribute__((aligned(8)));

extern "C" void SexyLogSinkWrite(const char* theLine)
{
	if (theLine == NULL)
		return;

	size_t aLength = strlen(theLine);
	if (sLogFile != NULL)
	{
		fwrite(theLine, 1, aLength, sLogFile);
		// Commit to the card per line, so after a crash or a hang the log ends
		// at the last thing the game did. Costs some speed while loading;
		// build with -DSEXY_LOG_LEVEL=0 to turn per-line logging off.
		fsync(fileno(sLogFile));
	}
	fwrite(theLine, 1, aLength, stdout);
}

// Logs the reason and shows it on the system error screen (Luma3DS prints the
// message), then ends the process. Safe from any thread.
static void __attribute__((noreturn)) CtrLogFatal(const char* theWhat)
{
	SexyLogForcePrintf("[FATAL] %s\n", theWhat);
	ERRF_ThrowResultWithMessage(MAKERESULT(RL_FATAL, RS_INTERNAL, RM_APPLICATION, RD_INVALID_RESULT_VALUE), theWhat);
	svcExitProcess();
}

// Uncaught C++ exception in any thread, e.g. TodErrorMessageBox when a
// resource fails to load. Same recovery of the message as Ps2TerminateHandler.
static void CtrTerminateHandler()
{
	if (std::current_exception())
	{
		try { std::rethrow_exception(std::current_exception()); }
		catch (const std::exception& e) { CtrLogFatal(e.what()); }
		catch (...) { CtrLogFatal("uncaught exception of unknown type"); }
	}
	CtrLogFatal("std::terminate without an active exception");
}

// CPU exception (bad pointer, bad instruction) in any thread. Only the raw
// file descriptor is used: the faulting thread may hold the FILE or heap lock.
// The pc/lr addresses map to code with arm-none-eabi-addr2line -f -C -e
// against the .elf of the same build.
static void __attribute__((target("arm"))) CtrExceptionHandler(ERRF_ExceptionInfo* theInfo, CpuRegisters* theRegs)
{
	static const char* const kTypeNames[] = { "prefetch abort", "data abort", "undefined instruction", "VFP exception" };
	const int aType = (int)theInfo->type;

	char aLine[192];
	int aLength = snprintf(aLine, sizeof(aLine), "[CRASH] %s pc=%08lX lr=%08lX sp=%08lX far=%08lX\n",
		(aType >= 0 && aType < 4) ? kTypeNames[aType] : "CPU exception",
		(unsigned long)theRegs->pc, (unsigned long)theRegs->lr,
		(unsigned long)theRegs->sp, (unsigned long)theInfo->far);
	if (aLength > (int)sizeof(aLine) - 1)
		aLength = sizeof(aLine) - 1;

	if (sLogFile != NULL && aLength > 0)
	{
		write(fileno(sLogFile), aLine, aLength);
		fsync(fileno(sLogFile));
	}

	// The system error screen then shows the full register dump.
	ERRF_ExceptionHandler(theInfo, theRegs);
}

void CtrLogInit()
{
	mkdir(CTR_LOG_DIR, 0777);
	remove(CTR_LOG_OLD_PATH);
	rename(CTR_LOG_PATH, CTR_LOG_OLD_PATH);
	sLogFile = fopen(CTR_LOG_PATH, "w");
	if (sLogFile != NULL)
		setvbuf(sLogFile, NULL, _IONBF, 0);

	// Set from the main thread, this is the default for every thread.
	threadOnException(CtrExceptionHandler, sExceptionStack + sizeof(sExceptionStack), WRITE_DATA_TO_HANDLER_STACK);
	std::set_terminate(CtrTerminateHandler);

#ifdef PVZ_3DS_PROFILE_NEW3DS
	const char* aProfile = "NEW3DS";
#else
	const char* aProfile = "OLD3DS";
#endif
	bool anIsNew3DS = false;
	APT_CheckNew3DS(&anIsNew3DS);
	SexyLogForcePrintf("[3DS] PvZ %s build " __DATE__ " " __TIME__ " on %s, heap %luK, linear heap %luK\n",
		aProfile, anIsNew3DS ? "New 3DS" : "Old 3DS",
		(unsigned long)(envGetHeapSize() / 1024), (unsigned long)(envGetLinearHeapSize() / 1024));
}
