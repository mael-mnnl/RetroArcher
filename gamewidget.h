#ifndef GAMEWIDGET_H
#define GAMEWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QSet>
#include <QElapsedTimer>
#include <QImage>
#include <QColor>
#include <QString>
#include <QVector>
#include <vector>

enum EnemyType {
    ET_None,
    ET_Slime,
    ET_Skel,
    ET_Bat,
    ET_Brute,
    ET_Mage,
    ET_MiniBoss,
    ET_FinalBoss
};

enum AnimType { AN_Idle, AN_Walk, AN_Atk, AN_Hurt, AN_Death };

enum GameState {
    GS_Menu,
    GS_SkinSelect,
    GS_Playing,
    GS_RoomCleared,
    GS_FadeOut,
    GS_SkillSelect,
    GS_FadeIn,
    GS_GameOver,
    GS_Victory
};

struct Particle {
    float x, y, vx, vy;
    QColor color;
    float life, maxLife;
    int size;
};

struct Bullet {
    float x = 0, y = 0;
    float vx = 0, vy = 0;
    float damage = 0;
    bool pierce = false;
    int bounce = 0;
    bool enemy = false;
    bool dead = false;
    float angle = 0;
    bool grenade = false;
    float grenadeFuse = 0;
    float grenadeTotal = 0;
    bool splitChild = false;    // generee par SK_SPLT, ne peut pas re-splitter
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
    int subType = 0;

    float slowTimer = 0;   // SK_FRZ : enemi ralenti quand > 0
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
};

struct Skill {
    int id;
    QString name;
    QString desc;
    QString icon;
    QColor color;
};

class GameWidget : public QWidget
{
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
    // Assets
    void loadAssets();
    void loadBossGallery();
    void buildSkinSprites();
    QImage hueShift(const QImage &src, int degrees, double sat, double val);

    // Game flow
    void startGame();
    void buildRoom(int roomIndex, int entryDoor);
    bool isMiniBossRoom(int room) const;
    bool isFinalBossRoom(int room) const;
    bool isAnyBossRoom(int room) const;

    // Update
    void updateGame(float dt);
    void updatePlayer(float dt);
    void updateEnemies(float dt);
    void updateBullets(float dt);
    void updateParticles(float dt);
    void shootPlayer();
    void throwGrenade();
    void hurtEnemy(Enemy &e, float dmg);
    void hurtPlayer(float dmg);
    void checkDoorTransition();
    void generateSkills();
    void applySkill(int skillId);

    // Collision
    bool isObstacleTile(int col, int row) const;
    bool collidesObstacle(float x, float y, float r) const;
    void resolveCollision(float &x, float &y, float r);
    void tryMove(float &x, float &y, float dx, float dy, float r);

    // Persistence
    void loadHighScore();
    void saveHighScore(int roomReached);

    // Render
    void renderGame(QPainter &p);
    void drawRoom(QPainter &p);
    void drawHUD(QPainter &p);
    void drawMenu(QPainter &p);
    void drawSkinSelect(QPainter &p);
    void drawSkillSelect(QPainter &p);
    void drawEndScreen(QPainter &p, bool win);
    void drawProgressionBar(QPainter &p, int x, int y, int w);
    void drawSpriteAt(QPainter &p, const QImage &sheet, int frameCount, float fps,
                      float x, float y, float scale, bool flipX, float timer);
    QImage soldierSheet(AnimType anim) const;
    int    soldierFrameCount(AnimType anim) const;
    float  soldierFps(AnimType anim) const;

    // State
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

    QSet<int> m_keys;
    QElapsedTimer m_elapsed;
    qint64 m_lastTickNs = 0;
    QTimer m_timer;
    float m_globalTime = 0;

    std::vector<Skill> m_allSkills;
    std::vector<int> m_skillChoices;

    int m_highScore = 0;
    int m_menuSelectedSkin = 0;
    int m_menuSelectedAtk = 0;

    // Sprites joueur / ennemis
    QImage m_sprSoldierBase[5];
    QImage m_sprSoldierAtkVariants[3];
    QImage m_sprSoldierSkin[4][5];
    QImage m_sprSoldierAtkSkin[4][3];
    QImage m_sprArrow;
    QImage m_sprOrc[6][5];

    // Galerie boss (menu)
    struct BossInfo {
        QImage  idleSheet;
        int     frameCount = 1;
        int     frameW = 100, frameH = 100;
        float   fps = 8.f;
        QString nameFr;
        QString roomTxt;
        QColor  accent;
    };
    QVector<BossInfo> m_bossGallery;

    static const int s_walkFrameCount;
    static const int s_idleFrameCount;
    static const int s_hurtFrameCount;
    static const int s_deathFrameCount;
    static const int s_atk1Frames;
    static const int s_atk2Frames;
    static const int s_atk3Frames;
};

#endif
