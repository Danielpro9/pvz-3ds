#pragma once
#ifdef WII_PLATFORM

// GXRModeObj is a typedef of `struct _gx_rmodeobj`, not a struct tag, so it
// cannot be forward-declared as `struct GXRModeObj`. Pull in the real header.
#include <ogc/gx_struct.h>

// Diagnostic verbosity. Set by cmake/wii.cmake; the fallback here is so that
// compiling a file on its own does not silently pick a different level.
//
//   0  Off. wiiLog formats nothing and touches no channel, and opengx's own
//      warnings are silenced too (gx_wii.cpp). Nothing in the running game
//      writes to the SD card, the screen, or the Gecko port.
//
//   1  Boot and milestones. The default. Everything that fires once -- storage
//      probing, GX bring-up, world-load RAM milestones, texture failures -- and
//      nothing that fires on a timer or per asset.
//
//   2  Verbose. Adds periodic frame/world/heap reports, per-texture loads,
//      rate-limited GL/OpenGX errors and the model-transform trace. Useful, and
//      NOT free: see the cost note on wiiLog's channel 3.
#ifndef WII_LOG_LEVEL
#define WII_LOG_LEVEL 1
#endif

// Writes a diagnostic line to every channel that might be watched.
//
// libogc's printf goes to the framebuffer text console and nowhere else -- it
// does NOT reach Dolphin's log window, which is why that window stays empty even
// for a program that is running fine (retail games look the same). Dolphin's log
// is fed by SYS_Report, a separate call. This writes both, so a message survives
// whether the observer is looking at a TV, at Dolphin's log, or at a USB Gecko.
//
// Cheap during boot and still synchronous afterwards. The session file stays
// open, but every message is flushed so crash evidence reaches FAT immediately.
// Anything that can fire inside a frame belongs behind WII_LOG_LEVEL 2.
extern "C" void wiiLog(const char *fmt, ...)
	__attribute__((format(printf, 1, 2)));

// Cadence of the periodic liveness ticks, [WII][BEAT] and [WII][STEP].
//
// They are the exception to the rule above: they fire inside a frame, but they
// are the only thing that tells a hard hang apart from a livelock when the
// screen freezes, so they have to survive at the default level. At 1 Hz, though,
// two of them emit 120 lines a minute into OSREPORT -- the same channel every
// other diagnostic lands in -- and they bury it. A music failure or a texture
// error scrolls past between heartbeats.
//
// So: keep them, slow them down. Verbose restores the fast cadence, which is
// what you want when you are actually watching for a stall.
#define WII_LIVENESS_INTERVAL_MS ((WII_LOG_LEVEL >= 2) ? 1000u : 10000u)

// Called once, by wiigl_init, at the moment GX takes ownership of the
// framebuffer. After this wiiLog stops writing to the text console, because that
// console renders into the very buffer the game is now flipping -- see the
// channel 1 comment in WiiEarlyInit.cpp. Everything else keeps working.
void wiiLogEndBootPhase();

// Mounts the SD/USB filesystem, at most once, and reports whether it worked.
//
// Safe and cheap to call from anywhere, including from a static initializer:
// the first call does the work, every later call returns the cached result.
// See WiiEarlyInit.cpp for why this cannot simply live in main().
bool wiiEnsureStorage();

// The install directory: where assets are read from and worlds are written.
// "sd:/apps/PlantsVsZombies", "usb:/apps/PlantsVsZombies", or wherever the loader
// actually started the DOL from -- resolved once by wiiEnsureStorage(), which
// this calls for you.
//
// SD and USB are both supported and neither is a build-time choice; see the
// device-selection comment in WiiEarlyInit.cpp for how one is picked.
//
// Always returns a usable absolute path with a device prefix and no trailing
// slash, even when nothing mounted -- callers get a path that fails to open
// rather than a null pointer to check.
//
// Returns a `const char *` and not a std::string on purpose: it is read from
// static initialisers, and a namespace-scope std::string holding the answer
// would be constructed *after* the prioritised constructor that fills it in,
// blanking it. GameResources::getExeDir() copies it into a std::string at the
// point of use, which is safe.
const char *wiiGetAppDir();

// True when the install directory really holds a staged data/ tree. False means
// the game will fail on its first asset load, and the caller is expected to say
// so in a way the user can act on rather than dying inside a texture load.
bool wiiHasGameData();

// Brings up video and libogc's text console, at most once. Called automatically
// before main() so that early failures are readable on the TV instead of being
// a black screen; safe to call again.
void wiiEnsureEarlyVideo();

// The video mode and the first framebuffer chosen by the early init. gx_wii.cpp
// adopts both instead of allocating its own, so the diagnostic console costs no
// extra memory once the renderer takes over.
GXRModeObj *wiiGetRenderMode();
void       *wiiGetEarlyFramebuffer();

// Total bytes sbrk will ever be able to hand out: Arena2 sampled before the
// game's static initializers run. MALLOC_MEM2=1 selects MEM2; it does not create
// a MEM1-then-MEM2 overflow heap, so Arena1 must not be counted as headroom.
//
// java/Runtime's maxMemory() reports this rather than the physical 88 MB, so the
// F3 percentage is against memory the game can actually get.
u32 wiiGetHeapCeiling();

#endif // WII_PLATFORM
