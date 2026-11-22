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

    static IsoView battlefieldView();
    static constexpr float ISLAND_DEPTH = 42.0f;

    void drawBackdrop(Gfx& g) const;
    void drawGround(Gfx& g) const;
    void drawGrid(Gfx& g, float alpha) const;
    void drawLava(Gfx& g) const;
    bool hasCellObject(int cell) const;
    void drawCellObject(Gfx& g, int cell) const;
    void drawPortals(Gfx& g) const;
    void drawCastle(Gfx& g, float hitFlash) const;
    void drawVignette(Gfx& g) const;
    void drawAmbient(Gfx& g) const;

    int themeId() const { return theme; }

    const std::vector<int>& spawnCells() const { return spawns; }
    int goalCell() const { return goal; }

private:
    SDL_Texture* terrain;
    int types[MAP_W * MAP_H];
    bool features[MAP_W * MAP_H];
    int theme;
    bool valid;
    float builtScale;
    std::vector<int> spawns;
    int goal;

    void rebuildTerrain(Gfx& g);
    void drawPortal(Gfx& g, int cell) const;
};

#endif
