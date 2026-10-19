#pragma once

#ifdef __3DS__

// Opens sdmc:/3ds/PlantsvsZombies/log.txt and installs the crash hooks (CPU
// exceptions and uncaught C++ exceptions). Call first thing in main().
void CtrLogInit();

#endif
