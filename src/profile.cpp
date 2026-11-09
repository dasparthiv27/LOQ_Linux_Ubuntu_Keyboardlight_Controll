#include "profile.h"
#include <QJsonArray>

QString effectName(Effect e) { switch(e) { case Effect::Static:return "Static"; case Effect::Breathing:return "Breathing"; case Effect::Wave:return "Wave"; case Effect::Hue:return "Hue cycling"; } return "Static"; }
QString directionName(Direction d) { return d == Direction::LeftToRight ? "Left to right" : "Right to left"; }
Effect effectFromName(const QString &n, bool *ok) {
    const auto s=n.trimmed().toLower(); bool valid=true; Effect e=Effect::Static;
    if(s=="static")e=Effect::Static; else if(s=="breathing"||s=="breath")e=Effect::Breathing; else if(s=="wave")e=Effect::Wave; else if(s=="hue cycling"||s=="hue")e=Effect::Hue; else valid=false;
    if(ok)*ok=valid; return e;
}
bool Profile::operator==(const Profile &o) const { return name==o.name&&effect==o.effect&&zones==o.zones&&brightness==o.brightness&&speed==o.speed&&direction==o.direction; }
bool Profile::isValid(QString *error) const {
    auto fail=[&](const QString&s){if(error)*error=s;return false;};
    if(name.trimmed().isEmpty())return fail("Theme name cannot be empty.");
    if(brightness<0||brightness>100)return fail("Brightness must be between 0 and 100.");
    if(speed<1||speed>100)return fail("Speed must be between 1 and 100.");
    switch(effect){case Effect::Static:case Effect::Breathing:case Effect::Wave:case Effect::Hue:break;default:return fail("Unsupported lighting effect.");}
    switch(direction){case Direction::LeftToRight:case Direction::RightToLeft:break;default:return fail("Unsupported wave direction.");}
    for(const QColor &c:zones)if(!c.isValid())return fail("Each of the four zones needs a valid colour.");
    return true;
}
QJsonObject Profile::toJson() const {
    QJsonArray colors; for(const QColor &c:zones)colors.append(c.name(QColor::HexRgb));
    return {{"name",name},{"effect",effectName(effect)},{"brightness",brightness},{"speed",speed},{"direction",direction==Direction::LeftToRight?"ltr":"rtl"},{"zones",colors}};
}
bool Profile::fromJson(const QJsonObject &j, Profile *out, QString *error) {
    auto fail=[&](const QString&s){if(error)*error=s;return false;};
    if(!out)return fail("Missing profile output target.");
    if(j.value("schemaVersion").toInt(-1)!=1)return fail("Unsupported theme schema version (expected 1).");
    Profile p; p.name=j.value("name").toString(); bool effectOk=false; p.effect=effectFromName(j.value("effect").toString(),&effectOk);
    if(!effectOk)return fail("Unknown lighting effect.");
    p.brightness=j.value("brightness").toInt(-1); p.speed=j.value("speed").toInt(-1);
    const QString dir=j.value("direction").toString(); if(dir!="ltr"&&dir!="rtl")return fail("Direction must be ltr or rtl.");
    p.direction=dir=="ltr"?Direction::LeftToRight:Direction::RightToLeft;
    const auto a=j.value("zones").toArray(); if(a.size()!=4)return fail("A theme must define exactly four zone colours.");
    for(int i=0;i<4;++i){const QString hex=a[i].toString();QColor c(hex);if(!c.isValid()||!hex.startsWith('#')||hex.size()!=7)return fail("Zone colours must be #RRGGBB values.");p.zones[static_cast<size_t>(i)]=c;}
    QString validation; if(!p.isValid(&validation))return fail(validation); *out=p; return true;
}
QList<Profile> builtInProfiles() {
    struct Entry{const char*name;const char*a;const char*b;const char*c;const char*d;Effect e;};
    const Entry entries[]={
        {"Cyberpunk","#ff2bd6","#742bff","#08d9ff","#ff2b6d",Effect::Wave},
        {"Ocean","#00b7d9","#0077ff","#00e5c0","#164bba",Effect::Breathing},
        {"Fire","#ff2100","#ff5a00","#ffad00","#ff3600",Effect::Wave},
        {"Aurora","#00ffc6","#39e879","#41b8ff","#9a5bff",Effect::Breathing},
        {"Sunset","#ff3c72","#ff7c42","#ffb64a","#9e3d91",Effect::Static},
        {"Arctic","#b1faff","#57cfff","#467dff","#ecfbff",Effect::Static},
        {"Matrix","#39ff14","#09c92c","#007b25","#55ff70",Effect::Wave},
        {"Neon","#fff000","#ff00db","#00e9ff","#5eff00",Effect::Hue},
        {"Purple Galaxy","#4811a8","#9b38ff","#ec60ff","#3914d4",Effect::Breathing},
        {"Rainbow","#ff3b30","#ffcc00","#34c759","#007aff",Effect::Hue}};
    QList<Profile> result; for(const auto &x:entries){Profile p;p.name=x.name;p.effect=x.e;p.zones={QColor(x.a),QColor(x.b),QColor(x.c),QColor(x.d)};result.append(p);}return result;
}
