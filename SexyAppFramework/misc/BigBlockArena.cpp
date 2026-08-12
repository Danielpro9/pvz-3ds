#include "BigBlockArena.h"

#include <cstdlib>
#include <cstring>
#include <new>
#include <pthread.h>

#ifdef WII_PLATFORM
#include <malloc.h>
#endif
#ifdef PS2_PLATFORM
#include "platform/ps2/Ps2BigArena.h"
#endif

#ifdef WII_PLATFORM
namespace
{
const size_t kBigAlign = 64u;
const unsigned int kBigMagic = 0xB16B10C5u;

struct BigBlock
{
	size_t mSize;
	unsigned int mFree;
	unsigned int mMagic;
};

static unsigned char* sArena = nullptr;
static pthread_mutex_t sLock;
static pthread_once_t sOnce = PTHREAD_ONCE_INIT;
static size_t sUsed = 0;

size_t AlignUp(size_t value)
{
	return (value + kBigAlign - 1) & ~(kBigAlign - 1);
}

size_t HeaderBytes()
{
	return AlignUp(sizeof(BigBlock));
}

void InitArena()
{
	pthread_mutex_init(&sLock, nullptr);
	// MALLOC_MEM2 is selected before static initialisation, so this backing
	// allocation comes from MEM2. It happens on the first compiled definition,
	// while the heap is still contiguous, and is never returned to the general
	// heap where small allocations could fragment it.
	sArena = static_cast<unsigned char*>(memalign(kBigAlign, CONSOLE_BIG_ARENA_BYTES));
	if (sArena == nullptr)
		return;
	std::memset(sArena, 0, CONSOLE_BIG_ARENA_BYTES);
	BigBlock* first = reinterpret_cast<BigBlock*>(sArena);
	first->mSize = CONSOLE_BIG_ARENA_BYTES - HeaderBytes();
	first->mFree = 1;
	first->mMagic = kBigMagic;
}

bool InRange(const void* ptr)
{
	const unsigned char* bytes = static_cast<const unsigned char*>(ptr);
	return sArena != nullptr && bytes >= sArena &&
		bytes < sArena + CONSOLE_BIG_ARENA_BYTES;
}

BigBlock* Next(BigBlock* block)
{
	return reinterpret_cast<BigBlock*>(
		reinterpret_cast<unsigned char*>(block) + HeaderBytes() + block->mSize);
}
}
#endif

extern "C" void* ConsoleBigBlockAlloc(size_t theSize)
{
#ifdef PS2_PLATFORM
	return Ps2BigAlloc((unsigned int)theSize);
#elif defined(WII_PLATFORM)
	if (theSize == 0 || theSize > CONSOLE_BIG_ARENA_BYTES - HeaderBytes())
		return nullptr;
	pthread_once(&sOnce, InitArena);
	if (sArena == nullptr)
		return nullptr;

	const size_t need = AlignUp(theSize);
	void* result = nullptr;
	pthread_mutex_lock(&sLock);
	BigBlock* block = reinterpret_cast<BigBlock*>(sArena);
	while (InRange(block))
	{
		if (block->mMagic != kBigMagic)
			break;
		if (block->mFree)
		{
			BigBlock* next = Next(block);
			while (InRange(next) && next->mMagic == kBigMagic && next->mFree)
			{
				block->mSize += HeaderBytes() + next->mSize;
				next = Next(block);
			}
			if (block->mSize >= need)
			{
				if (block->mSize >= need + HeaderBytes() + kBigAlign)
				{
					BigBlock* rest = reinterpret_cast<BigBlock*>(
						reinterpret_cast<unsigned char*>(block) + HeaderBytes() + need);
					rest->mSize = block->mSize - need - HeaderBytes();
					rest->mFree = 1;
					rest->mMagic = kBigMagic;
					block->mSize = need;
				}
				block->mFree = 0;
				sUsed += block->mSize;
				result = reinterpret_cast<unsigned char*>(block) + HeaderBytes();
				break;
			}
		}
		block = Next(block);
	}
	pthread_mutex_unlock(&sLock);
	return result;
#else
	(void)theSize;
	return nullptr;
#endif
}

extern "C" int ConsoleBigBlockOwns(const void* thePtr)
{
#ifdef PS2_PLATFORM
	return Ps2BigOwns(thePtr);
#elif defined(WII_PLATFORM)
	pthread_once(&sOnce, InitArena);
	return thePtr != nullptr && InRange(thePtr);
#else
	(void)thePtr;
	return 0;
#endif
}

extern "C" void ConsoleBigBlockFree(void* thePtr)
{
#ifdef PS2_PLATFORM
	Ps2BigFree(thePtr);
#elif defined(WII_PLATFORM)
	if (!ConsoleBigBlockOwns(thePtr))
		return;
	pthread_mutex_lock(&sLock);
	BigBlock* block = reinterpret_cast<BigBlock*>(
		static_cast<unsigned char*>(thePtr) - HeaderBytes());
	if (block->mMagic == kBigMagic && !block->mFree)
	{
		block->mFree = 1;
		sUsed -= block->mSize;
	}
	pthread_mutex_unlock(&sLock);
#else
	(void)thePtr;
#endif
}

extern "C" size_t ConsoleBigBlockUsedBytes(void)
{
#ifdef PS2_PLATFORM
	return Ps2BigUsedBytes();
#elif defined(WII_PLATFORM)
	return sUsed;
#else
	return 0;
#endif
}

extern "C" size_t ConsoleBigBlockLargestFreeBytes(void)
{
#ifdef PS2_PLATFORM
	// The legacy PS2 arena does not expose its block walk; capacity minus used
	// is still useful telemetry and allocation remains authoritative.
	return PS2_BIG_ARENA_BYTES - Ps2BigUsedBytes();
#elif defined(WII_PLATFORM)
	pthread_once(&sOnce, InitArena);
	if (sArena == nullptr)
		return 0;
	size_t largest = 0;
	pthread_mutex_lock(&sLock);
	BigBlock* block = reinterpret_cast<BigBlock*>(sArena);
	while (InRange(block) && block->mMagic == kBigMagic)
	{
		if (block->mFree && block->mSize > largest)
			largest = block->mSize;
		block = Next(block);
	}
	pthread_mutex_unlock(&sLock);
	return largest;
#else
	return 0;
#endif
}

ConsoleScratchBuffer::ConsoleScratchBuffer(size_t theSize)
	: mData(theSize != 0 ? ConsoleBigBlockAlloc(theSize) : nullptr),
	  mSize(theSize), mArenaBacked(mData != nullptr)
{
	if (mData == nullptr && theSize != 0)
		mData = ::operator new[](theSize, std::nothrow);
}

ConsoleScratchBuffer::~ConsoleScratchBuffer()
{
	if (mArenaBacked)
		ConsoleBigBlockFree(mData);
	else
		::operator delete[](mData);
}
