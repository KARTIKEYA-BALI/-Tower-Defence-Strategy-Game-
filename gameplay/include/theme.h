#ifndef THEME_H
#define THEME_H

#include "gfx.h"

enum class Feature { Bush, Pine, Cactus, AutumnTree, LavaPool };
enum class Ambient { None, Snow, Dust, Leaves, Embers };

struct Theme {
    const char* label;
    Color groundDark;
    Color groundMid;
    Color groundLight;
    Color groundPatch;
    Color pathDark;
    Color pathLight;
    Color gravel;
    Color specks[3];
    int speckEvery;
    Color rockDark;
    Color rockMid;
    Color rockLight;
    Color rockCap;
    Feature feature;
    Ambient ambient;
    bool cracks;
    Color crack;
    Color vignette;
    Color previewA;
    Color previewB;
    Color previewPath;
    Color previewFeature;
    Color cliffTop;
    Color cliffBottom;
    Color skyTop;
    Color skyBottom;
};

const Theme& themeFor(int themeId);
void drawFeature(Gfx& g, Feature f, float cx, float cy, unsigned seed, float time);
float featureBase(Feature f);

#endif
