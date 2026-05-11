#include "biomefxsystem.h"
#include "../model/gamemodel.h"
#include "../core/utils.h"
#include "../core/constants.h"
#include "../core/palette.h"
#include <algorithm>

namespace Controller { namespace BiomeFX {

using namespace Model;
using namespace Core;

void spawnFx(GameModel &m, const char *key, float x, float y, float scale)
{
    FxEffect fx;
    fx.sheetKey = QString::fromUtf8(key);
    if      (fx.sheetKey == "fire")      { fx.frameCount=8; fx.frameW=72; fx.frameH=72; fx.fps=14.f; }
    else if (fx.sheetKey == "portal")    { fx.frameCount=8; fx.frameW=72; fx.frameH=72; fx.fps=12.f; }
    else if (fx.sheetKey == "sparkle")   { fx.frameCount=4; fx.frameW=72; fx.frameH=72; fx.fps=12.f; }
    else if (fx.sheetKey == "explosion") { fx.frameCount=4; fx.frameW=72; fx.frameH=72; fx.fps=10.f; }
    else if (fx.sheetKey == "ice")       { fx.frameCount=8; fx.frameW=72; fx.frameH=72; fx.fps=14.f; }
    else if (fx.sheetKey == "poison")    { fx.frameCount=8; fx.frameW=72; fx.frameH=72; fx.fps=12.f; }
    else if (fx.sheetKey == "void")      { fx.frameCount=6; fx.frameW=72; fx.frameH=72; fx.fps=10.f; }
    fx.x = x; fx.y = y; fx.scale = scale;
    m.fxEffects.push_back(fx);
}

void updateFx(GameModel &m, float dt)
{
    for (auto &fx : m.fxEffects) {
        fx.timer += dt;
        float total = fx.frameCount / fx.fps;
        if (fx.timer >= total) fx.alive = false;
    }
    m.fxEffects.erase(std::remove_if(m.fxEffects.begin(), m.fxEffects.end(),
                      [](const FxEffect &f){ return !f.alive; }), m.fxEffects.end());
}

void update(GameModel &m, float dt)
{
    int worldIdx = std::min(6, ((m.currentRoom-1)/5) + 1) - 1;

    // Spawn rate
    int rate = 1;
    if (m.biomeFx.size() > 40) rate = 0;

    for (int i = 0; i < rate; ++i) {
        BiomeParticle bp;
        bp.x = rndF(TILE, GW-TILE);
        bp.y = (worldIdx == 4) ? -8 : rndF(TILE, GH-TILE);
        bp.size = (int)rndF(1.5f, 3.5f);
        bp.life = bp.maxLife = rndF(1.0f, 3.0f);
        if (worldIdx == 0) { // Forge ardente : braises
            bp.color = QColor("#ff7722"); bp.vx = rndF(-8,8); bp.vy = -rndF(15,35);
            bp.y = GH - TILE - rndF(0,8);
        } else if (worldIdx == 1) { // Catacombes : brume
            bp.color = QColor(160,160,200,80); bp.vx = rndF(-3,3); bp.vy = rndF(-5,-1);
            bp.size = (int)rndF(3,5);
        } else if (worldIdx == 2) { // Marais : poison
            bp.color = QColor("#88ff44"); bp.vx = rndF(-2,2); bp.vy = -rndF(8,20);
            bp.size = (int)rndF(1,3);
        } else if (worldIdx == 3) { // Arene : poussiere
            bp.color = QColor(200,180,140,120); bp.vx = rndF(-15,15); bp.vy = rndF(-15,5);
        } else if (worldIdx == 4) { // Citadelle : neige
            bp.color = QColor(220,230,255); bp.vx = rndF(-12,12); bp.vy = rndF(30,55);
            bp.size = (int)rndF(2,4);
        } else { // Abysses : void
            bp.color = QColor(180,100,220,100); bp.vx = rndF(-10,10); bp.vy = rndF(-10,10);
            bp.size = (int)rndF(2,4);
        }
        m.biomeFx.push_back(bp);
    }

    for (auto &bp : m.biomeFx) {
        bp.x += bp.vx*dt; bp.y += bp.vy*dt;
        bp.life -= dt;
    }
    m.biomeFx.erase(std::remove_if(m.biomeFx.begin(), m.biomeFx.end(),
                    [](const BiomeParticle &p){ return p.life <= 0; }), m.biomeFx.end());
}

}}
