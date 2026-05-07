#include "collisionsystem.h"
#include "../model/gamemodel.h"
#include "../core/constants.h"
#include "../core/utils.h"
#include <cmath>
#include <algorithm>

namespace Controller { namespace Collision {

using namespace Core;

bool isObstacleTile(const Model::GameModel &m, int col, int row)
{
    if (col<0 || col>=COLS || row<0 || row>=ROWS) return true;
    bool isPerim = (col==0 || col==COLS-1 || row==0 || row==ROWS-1);
    if (isPerim) {
        if (row==0      && col==DOOR_COL && m.doorOpen[0]) return false;
        if (col==COLS-1 && row==DOOR_ROW && m.doorOpen[1]) return false;
        if (row==ROWS-1 && col==DOOR_COL && m.doorOpen[2]) return false;
        if (col==0      && row==DOOR_ROW && m.doorOpen[3]) return false;
        return true;
    }
    return false;
}

bool collidesObstacle(const Model::GameModel &m, float x, float y, float r)
{
    int c0 = std::max(0, int((x-r)/TILE)), c1 = std::min(COLS-1, int((x+r)/TILE));
    int r0 = std::max(0, int((y-r)/TILE)), r1 = std::min(ROWS-1, int((y+r)/TILE));
    for (int row = r0; row <= r1; ++row)
        for (int col = c0; col <= c1; ++col) {
            if (!isObstacleTile(m, col, row)) continue;
            float tx0 = col*TILE, ty0 = row*TILE, tx1 = tx0+TILE, ty1 = ty0+TILE;
            float cx = clampF(x, tx0, tx1), cy = clampF(y, ty0, ty1);
            float dx = x-cx, dy = y-cy;
            if (dx*dx+dy*dy < r*r) return true;
        }
    return false;
}

void resolveCollision(const Model::GameModel &m, float &x, float &y, float r)
{
    for (int iter = 0; iter < 8; ++iter) {
        bool collided = false;
        int c0 = std::max(0, int((x-r)/TILE)), c1 = std::min(COLS-1, int((x+r)/TILE));
        int r0 = std::max(0, int((y-r)/TILE)), r1 = std::min(ROWS-1, int((y+r)/TILE));
        for (int row = r0; row <= r1; ++row)
            for (int col = c0; col <= c1; ++col) {
                if (!isObstacleTile(m, col, row)) continue;
                float tx0 = col*TILE, ty0 = row*TILE, tx1 = tx0+TILE, ty1 = ty0+TILE;
                float cx = clampF(x, tx0, tx1), cy = clampF(y, ty0, ty1);
                float dx = x-cx, dy = y-cy, d2 = dx*dx+dy*dy;
                if (d2 < r*r && d2 > 0.0001f) {
                    float d = std::sqrt(d2), push = r-d+0.5f;
                    x += (dx/d)*push; y += (dy/d)*push; collided = true;
                }
            }
        if (!collided) break;
    }
}

void tryMove(const Model::GameModel &m, float &x, float &y, float dx, float dy, float r)
{
    float dist = std::hypot(dx, dy);
    int steps = std::max(1, int(std::ceil(dist/(TILE*0.4f))));
    float sx = dx/steps, sy = dy/steps;
    for (int i = 0; i < steps; ++i) {
        float nx = x + sx;
        if (!collidesObstacle(m, nx, y, r)) x = nx;
        float ny = y + sy;
        if (!collidesObstacle(m, x, ny, r)) y = ny;
    }
}

}}
