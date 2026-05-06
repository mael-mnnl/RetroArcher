#ifndef GAMEWIDGET_H
#define GAMEWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QSet>
#include <QHash>
#include <QElapsedTimer>
#include <QImage>
#include <QColor>
#include <QString>
#include <QVector>
#include <vector>

enum EnemyType {
    ET_None, ET_Slime, ET_Skel, ET_Bat, ET_Brute, ET_Mage,
    ET_Minion, ET_MiniBoss, ET_FinalBoss
};

enum AnimType { AN_Idle, AN_Walk, AN_Atk, AN_Hurt, AN_Death };

enum GameState {
    GS_Menu, GS_SkinSelect, GS_Discoveries,
    GS_Playing, GS_RoomCleared, GS_FadeOut, GS_SkillSelect, GS_FadeIn,
    GS_GameOver, GS_Victory
};

struct Particle {
    float x, y, vx, vy;
    QColor color;
    float life, maxLife;
    int size;
    bool noGravity = false;
};

struct Bullet {
    float x = 0, y = 0;
    float vx = 0, vy = 0;
    float damage = 0;
    bool pierce = false;
    int pierceLeft = 0;
    int bounce = 0;
    bool enemy = false;
    bool dead = false;
    float angle = 0;
    bool grenade = false;
    float grenadeFuse = 0;
    float grenadeTotal = 0;
    bool splitChild = false;
    bool icy = false;
    bool poison = false;
    bool burn = false;
    bool homing = false;
    bool ghost = false;
    QSet<int> hitIds;
};

struct Enemy {
    int id = 0;
    float x = 0, y = 0;
    float hp = 0, maxHp = 0;
    float speed = 0;
    float damage = 1;
    float size = 28;
    EnemyType type = ET_Slime;
    bool dead = false;
    bool corpse = false;
    float corpseFadeTimer = 0;
    float hitFlash = 0;
    float animTimer = 0;
    AnimType anim = AN_Idle;
    bool facingLeft = false;

    float shootCd = 0;
    float aimAngle = 0;
    float zigDir = 1;
    float zigTimer = 0;

    int phase = 1;
    float burstCd = 0;
    float moveCd = 0;
    float moveTargetX = 0;
    float moveTargetY = 0;
    float specialCd = 0;
    int subType = 0;     // 0=fireworm 1=undead 2=demon 3=mino 4=frost

    float slowTimer = 0;
    float poisonTimer = 0;
    float poisonDps = 0;
    float burnTimer = 0;
    float burnDps = 0;
    float deathMark = 0;     // SK_DEATH_MARK : extra dmg multiplier

    // Boss patterns
    float chargeCd = 0;
    bool  charging = false;
    float chargeVx = 0, chargeVy = 0;
    float chargeT = 0;
    float summonCd = 0;
    float trailCd = 0;
};

struct Player {
    float x = 0, y = 0;
    float hp = 5, maxHp = 5;
    float speed = 155;
    float fireRate = 0.7f;
    float damage = 10;
    float fireCd = 0;
    float invincibility = 0;
    std::vector<int> skills;
    float size = 22;
    float facing = 0;
    bool moving = false;
    float animTimer = 0;
    AnimType anim = AN_Idle;
    bool facingLeft = false;
    float atkTimer = 0;

    int skinIndex = 0;
    int atkVariant = 0;
    int grenadeAmmo = 0;
    float grenadeCd = 0;

    // États pour skills actifs
    float dashCd = 0;
    float dashActive = 0;
    float timeStopCd = 0;
    float timeStopActive = 0;
    bool  timeStopUsed = false;
    bool  phoenixUsed = false;
    float regenTimer = 0;
    float regenAfterKillCd = 0;
    int   killStreakCount = 0;
    float killStreakTimer = 0;
    int   rageStacks = 0;
    float rageStackTimer = 0;
    float adrenalineTimer = 0;
    float shieldReadyTimer = 0;     // for SK_SHIELD/SK_FROST_ARMOR
    float invulBurstTimer = 0;      // SK_INVUL_BURST
    int   bossesKilled = 0;
    float stillSec = 0;
    int   frostNovaKills = 0;
    int   nextEnemyMarked = -1;     // SK_DEATH_MARK target
    float burstQueueTimer = 0;      // SK_BURST_FIRE secondary shot delay
    float chargeShotPower = 0;      // SK_CHARGE_SHOT held duration
};

struct Skill {
    int id;
    int worldTier;       // 1..5
    QString name;
    QString desc;
    QString icon;
    QColor color;
};

struct WorldInfo {
    QString name;
    QString flavor;
    QString bossKey;
    QString bossNameFr;
    int     bossFrameCount;
    int     bossFrameW;
    int     bossFrameH;
    float   bossFps;
    QColor  accent;
};

class GameWidget : public QWidget {
    Q_OBJECT
public:
    explicit GameWidget(QWidget *parent = nullptr);
    ~GameWidget();
protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
private slots:
    void tick();
private:
    void loadAssets();
    void loadWorlds();
    void buildSkinSprites();
    QImage hueShift(const QImage &src, int degrees, double sat, double val);

    void startGame();
    void buildRoom(int roomIndex, int entryDoor);
    int  worldOf(int room) const;
    bool isBossRoom(int room) const;
    bool isFinalBossRoom(int room) const;
    void spawnMinion(float x, float y, int hp, float speed, EnemyType type = ET_Minion);

    void updateGame(float dt);
    void updatePlayer(float dt);
    void updateEnemies(float dt);
    void updateBullets(float dt);
    void updateParticles(float dt);
    void shootPlayer();
    void throwGrenade();
    void activateDash();
    void activateTimeStop();
    void hurtEnemy(Enemy &e, float dmg, bool fromExplosion = false);
    void hurtPlayer(float dmg);
    void checkDoorTransition();
    void generateSkills();
    void applySkill(int skillId);

    bool hasSkill(int id) const;
    int  skillStacks(int id) const;
    float damageMul() const;
    float speedMul() const;
    float fireRateMul() const;
    float incomingDmgMul() const;
    int   pierceLevel() const;
    int   bounceLevel() const;

    bool isObstacleTile(int col, int row) const;
    bool collidesObstacle(float x, float y, float r) const;
    void resolveCollision(float &x, float &y, float r);
    void tryMove(float &x, float &y, float dx, float dy, float r);

    void loadSettings();
    void saveSettings();

    void renderGame(QPainter &p);
    void drawRoom(QPainter &p);
    void drawHUD(QPainter &p);
    void drawMenu(QPainter &p);
    void drawSkinSelect(QPainter &p);
    void drawSkillSelect(QPainter &p);
    void drawDiscoveries(QPainter &p);
    void drawEndScreen(QPainter &p, bool win);
    void drawProgressionBar(QPainter &p, int x, int y, int w);
    void drawSpriteAt(QPainter &p, const QImage &sheet, int frameCount, float fps,
                      float x, float y, float scale, bool flipX, float timer);
    void drawBossInGame(QPainter &p, const Enemy &boss, float scaleMul);
    QImage soldierSheet(AnimType anim) const;
    int    soldierFrameCount(AnimType anim) const;
    float  soldierFps(AnimType anim) const;

    GameState m_state = GS_Menu;
    int m_currentRoom = 1;
    Player m_player;
    std::vector<Enemy> m_enemies;
    std::vector<Bullet> m_bullets;
    std::vector<Particle> m_particles;
    int m_bossIndex = -1;
    int m_nextEnemyId = 1;

    int m_layoutIdx = 0;
    bool m_doorOpen[4] = {false,false,false,false};
    int m_exitDoor = 0;
    float m_fadeAmount = 0;
    int m_discoveryHover = 0;

    QSet<int> m_keys;
    QElapsedTimer m_elapsed;
    qint64 m_lastTickNs = 0;
    QTimer m_timer;
    float m_globalTime = 0;

    std::vector<Skill> m_allSkills;
    std::vector<int> m_skillChoices;

    int m_highScore = 0;
    int m_worldsUnlocked = 1;
    int m_menuSelectedSkin = 0;
    int m_menuSelectedAtk = 0;

    QImage m_sprSoldierBase[5];
    QImage m_sprSoldierAtkVariants[3];
    QImage m_sprSoldierSkin[4][5];
    QImage m_sprSoldierAtkSkin[4][3];
    QImage m_sprArrow;
    QImage m_sprOrc[6][5];

    QVector<WorldInfo> m_worlds;
    QHash<QString, QImage> m_bossSheets;

    static const int s_walkFrameCount;
    static const int s_idleFrameCount;
    static const int s_hurtFrameCount;
    static const int s_deathFrameCount;
    static const int s_atk1Frames;
    static const int s_atk2Frames;
    static const int s_atk3Frames;
};

#endif
