#include "dialogsystem.h"
#include "../model/gamemodel.h"
#include <QString>
#include <QColor>

namespace Controller { namespace Dialog {

void schedule(Model::GameModel &m, const QString &speaker, const QString &text, float dur, QColor color)
{
    m.dlgSpeaker = speaker;
    m.dlgText    = text;
    m.dlgTimer   = dur;
    m.dlgColor   = color;
    m.dlgFadeIn  = 0.25f;
}

void triggerBoss(Model::GameModel &m, int subType, int stage)
{
    static const char* dlgIntro[6] = {
        "Tu oses defier la Forge ardente, mortel ? Mon fragment te brulera.",
        "Mes serviteurs des catacombes ont faim... viens nourrir leurs os.",
        "Tu sens cette puanteur ? C'est le pouvoir de mon fragment qui te ronge.",
        "Aucun n'a quitte cette arene vivant. Mon fragment restera mien.",
        "Lord Malificus m'a confie ce fragment. Tu mourras gele, Heros.",
        "Tu as reuni les fragments... mais la Pierre m'appartient ! ABYSSES, ENGLOUTISSEZ-LE !"
    };
    static const char* dlgPhase[6][5] = {
        {"Tu m'as touche ? Brule donc dans mes braises !", "", "", "", ""},
        {"L'eternite m'appartient ! Mes legions ! VENEZ !", "", "", "", ""},
        {"Sa-saviez vous que je peux me dedoubler ? Hahaha !", "", "", "", ""},
        {"AAAARGH ! TU VAS PAYER POUR CETTE INSULTE !", "", "", "", ""},
        {"Tu progresses... le Gardien doit eveiller sa fureur.",
         "Le froid t'envahit. Je vais convoquer mes spectres.",
         "TES PAS S'ARRETERONT ICI ! GLACE ETERNELLE !",
         "PHASE FINALE ! POUR LORD MALIFICUS !", ""},
        {"Insolent... la Pierre te rejette deja.",
         "Les abysses s'ouvrent ! Goute mes portails !",
         "Tu m'amuses, Heros Inconnu. Mais je vais m'en lasser.",
         "ASSEZ ! Je vais te briser comme la Pierre que tu cherches !",
         "NON ! Je ne PEUX PAS perdre ! La Pierre est MIENNE !"}
    };
    static const char* dlgDeath[6] = {
        "Le fragment... s'echappe... que la Forge s'eteigne...",
        "Mes os... se dispersent... le fragment est tien...",
        "Comment... un mortel... peut-il... aaarrrgh...",
        "Tu... tu m'as... vaincu... prends... le fragment...",
        "Lord Malificus... pardonnez-moi... le fragment... vous attend...",
        "La Pierre... reforgee... le monde renait... toi, Heros... tu es son gardien."
    };

    if (subType < 0 || subType >= 6 || subType >= m.worlds.size()) return;
    QString speaker = m.worlds[subType].bossNameFr.toUpper();
    QColor col = m.worlds[subType].accent;

    if (stage == 0) {
        schedule(m, speaker, QString::fromUtf8(dlgIntro[subType]), 4.5f, col);
    } else if (stage == 99) {
        schedule(m, speaker, QString::fromUtf8(dlgDeath[subType]), 3.5f, col);
    } else if (subType == 5 && stage >= 1 && stage <= 5) {
        schedule(m, speaker, QString::fromUtf8(dlgPhase[5][stage-1]), 3.2f, col);
    } else if (subType == 4 && stage >= 2 && stage <= 4) {
        schedule(m, speaker, QString::fromUtf8(dlgPhase[4][stage-1]), 3.0f, col);
    } else if (stage == 1) {
        schedule(m, speaker, QString::fromUtf8(dlgPhase[subType][0]), 3.0f, col);
    }
}

}}
