#ifndef GEMC_H
#define GEMC_H

/*C interface for Gempyre, wrapper on C++ */

#ifdef WINDOWS_EXPORT
#define GEMC_EX __declspec( dllexport )
#else
#define GEMC_EX
#endif


enum Gemc_DebugLevel{Gem_DebugLevel_Quiet,
                    Gem_DebugLevel_Fatal,
                    Gem_DebugLevel_Error,
                    Gem_DebugLevel_Warning,
                    Gem_DebugLevel_Info,
                    Gem_DebugLeve_Debug,
                    Gem_DebugLeve_Debug_Trace};

struct Gemc_Rect {
    int x;
    int y;
    int width;
    int height;
};


struct Gemc_Ui {
    void* Imp;
};


GEMC_EX void gemc_set_debug(Gem_DebugLevel level);

GEMC_EX Gemc_Ui* gemc_new();


Gemc_Ui


#endif // GEMC_H
