#ifndef __TODDEFINITIONSTREAMING_H__
#define __TODDEFINITIONSTREAMING_H__

#if defined(PS2_PLATFORM) || defined(WII_PLATFORM)

#include <stddef.h>

class DefMap;

bool DefinitionReadCompiledStream(const void* theCompressedBuffer,
    size_t theCompressedBufferSize, DefMap* theDefMap, void* theDefinition);

#endif

#endif
