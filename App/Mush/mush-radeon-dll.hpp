#ifndef MUSH_RADEON_DLL
#define MUSH_RADEON_DLL

#ifdef _WIN32
#ifdef MUSH_RADEON_EXPORTS
#define RADEONEXPORTS_API __declspec(dllexport) 
#else
#define RADEONEXPORTS_API __declspec(dllimport) 
#endif
#else 
#define RADEONEXPORTS_API 
#endif

#endif