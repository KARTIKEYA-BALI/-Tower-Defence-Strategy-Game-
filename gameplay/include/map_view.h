#ifndef MAP_VIEW_H
#define MAP_VIEW_H

#include "game_config.h"
#include "gfx.h"
#include <vector>

class MapView {
public:
    MapView();
    ~MapView();
    MapView(const MapView&) = delete;
    MapView& operator=(const MapView&) = delete;

    void syncWithEngine(Gfx& g);
    void invalidate();

    void drawGround(Gfx& g) const;
    void drawGrid(Gfx& g, float alpha) const;
    void drawRoute(Gfx& g) const;
    void drawRocks(Gfx& g) const;
    void drawPortal(Gfx& g) const;
    void drawCastle(Gfx& g, float hitFlash) const;
    void drawVignette(Gfx& g) const;

    int spawnCell() const { return spawn; }
    int goalCell() const { return goal; }

private:
    SDL_Texture* terrain;
    int types[MAP_W * MAP_H];
    bool valid;
    float builtScale;
    int spawn;
    int goal;
    std::vector<SDL_FPoint> route;

    void rebuildTerrain(Gfx& g);
    void rebuildRoute();
};

#endif
