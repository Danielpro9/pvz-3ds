// Resource_wii.cpp — Wii implementation of Resource::getResource().
//
// The PS2 has to bake every resource into the ELF (see Resource_ps2.cpp) because
// it has no filesystem it can count on. The Wii has libfat, and a DOL is loaded
// whole into the scarce MEM1, so embedding ~10 MB of assets would spend memory
// to save nothing. Resources are read from the SD card or USB drive the game was
// installed on instead (GameResources::getExeDir() resolves which -- see
// WiiEarlyInit.cpp), which also means they can be replaced without rebuilding.
//
// Callers ask for paths rooted at the resource directory ("/terrain.png",
// "/gui/logo.png"), matching the desktop contract, so the leading slash is
// dropped and the rest is resolved under <app>/data/assets.
//
// The returned stream is owned by the caller, as on every other platform.
#ifdef WII_PLATFORM

#include "java/Resource.h"
#include "java/String.h"
#include "net/minecraft/src/GameResources.h"
#include "wii/WiiEarlyInit.h"

#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>

namespace Resource
{

std::istream *getResource(const jstring &name)
{
	// This runs from static initialisers (SharedConstants, ChatAllowedCharacters)
	// long before main(), so the filesystem cannot be assumed to be up yet.
	wiiEnsureStorage();

	std::string n = static_cast<const std::string &>(name);
	if (!n.empty() && (n[0] == '/' || n[0] == '\\'))
		n.erase(0, 1);

	const std::string path = GameResources::getExeDir() + "/data/assets/" + n;

	auto is = std::make_unique<std::ifstream>(path, std::ios::binary);
	if (!is->is_open() || !is->good())
	{
		// Same contract as the PS2 and desktop versions: a missing resource is
		// an exception, not a null stream. On this platform it almost always
		// means data/ was never copied to the SD card, so say so.
		throw std::runtime_error("Failed to open resource " + path +
		                         " (did you run: build wii.bat data ?)");
	}

	return is.release();
}

} // namespace Resource

#endif // WII_PLATFORM
