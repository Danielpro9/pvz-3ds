#pragma once

#ifdef __3DS__

// devkitPro ships no SDL2 for 3DS. The engine only needs SDL's timer
// helpers outside the SDL audio/video backends, so they map onto the POSIX
// clock and sleep calls libctru implements (same role as platform/wii/SDL.h).

#include <stdint.h>
#include <time.h>
#include <unistd.h>

typedef uint32_t Uint32;
typedef uint64_t Uint64;

#define SDL_INIT_TIMER 0x00000001u
#define SDL_QUIT       0x100u

typedef struct SDL_QuitEvent
{
	Uint32 type;
	Uint32 timestamp;
} SDL_QuitEvent;

typedef union SDL_Event
{
	Uint32 type;
	SDL_QuitEvent quit;
} SDL_Event;

// Parameters stay named: -include puts this header in C sources too.
static inline int SDL_Init(Uint32 theFlags)
{
	(void)theFlags;
	return 0;
}

static inline Uint64 SDL_GetPerformanceCounter()
{
	struct timespec aTime;
	clock_gettime(CLOCK_MONOTONIC, &aTime);
	return (Uint64)aTime.tv_sec * 1000000000ull + (Uint64)aTime.tv_nsec;
}

static inline Uint64 SDL_GetPerformanceFrequency()
{
	return 1000000000ull;
}

static inline Uint32 SDL_GetTicks()
{
	return (Uint32)(SDL_GetPerformanceCounter() / 1000000ull);
}

static inline void SDL_Delay(Uint32 ms)
{
	usleep((useconds_t)ms * 1000u);
}

static inline int SDL_PushEvent(SDL_Event* theEvent)
{
	(void)theEvent;
	return 0;
}

#endif
