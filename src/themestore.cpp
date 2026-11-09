#include "themestore.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QSet>
#include <QLoggingCategory>
#include <QSaveFile>

Q_LOGGING_CATEGORY(themeLog,"loq.themes")

QString ThemeStore::configDirectory(){return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);}
namespace {
bool writeJson(const QString&path,const QJsonObject&obj,QString*error){QSaveFile f(path);if(!f.open(QIODevice::WriteOnly)){if(error)*error=f.errorString();qCWarning(themeLog)<<"Unable to write theme data:"<<path<<f.errorString();return false;}const QByteArray bytes=QJsonDocument(obj).toJson(QJsonDocument::Indented);if(f.write(bytes)!=bytes.size()||!f.commit()){if(error)*error=f.errorString();qCWarning(themeLog)<<"Theme data write failed:"<<path<<f.errorString();return false;}return true;}
bool readJson(const QString&path,QJsonObject*out,QString*error){QFile f(path);if(!f.open(QIODevice::ReadOnly)){if(error)*error=f.errorString();qCWarning(themeLog)<<"Unable to read theme data:"<<path<<f.errorString();return false;}QJsonParseError pe;auto d=QJsonDocument::fromJson(f.readAll(),&pe);if(pe.error!=QJsonParseError::NoError||!d.isObject()){if(error)*error=pe.errorString();qCWarning(themeLog)<<"Invalid theme JSON:"<<path<<pe.errorString();return false;}*out=d.object();return true;}
bool ensureDir(QString*e){QDir d;if(d.mkpath(ThemeStore::configDirectory()))return true;if(e)*e="Could not create application config directory.";return false;}
}
QList<Profile> ThemeStore::loadCustom(QString*error){QList<Profile> out;const QString path=configDirectory()+"/themes.json";if(!QFile::exists(path))return out;QJsonObject root;if(!readJson(path,&root,error))return out;if(root.value("schemaVersion").toInt()!=1){if(error)*error="Unsupported themes file schema.";return out;}for(const auto&v:root.value("themes").toArray()){Profile p;QString e;if(!Profile::fromJson(v.toObject(),&p,&e)){if(error)*error=e;return {};}out.append(p);}return out;}
bool ThemeStore::saveCustom(const QList<Profile>&themes,QString*error){if(!ensureDir(error))return false;QJsonArray a;for(auto p:themes){if(!p.isValid(error))return false;auto o=p.toJson();o.insert("schemaVersion",1);a.append(o);}return writeJson(configDirectory()+"/themes.json",{{"schemaVersion",1},{"themes",a}},error);}
bool ThemeStore::saveLastApplied(const Profile&p,QString*error){if(!ensureDir(error))return false;if(!p.isValid(error))return false;auto o=p.toJson();o.insert("schemaVersion",1);return writeJson(configDirectory()+"/last-applied.json",o,error);}
bool ThemeStore::loadLastApplied(Profile*p,QString*error){QJsonObject o;if(!readJson(configDirectory()+"/last-applied.json",&o,error))return false;return Profile::fromJson(o,p,error);}
bool ThemeStore::writeTheme(const QString&path,const Profile&p,QString*error){if(!p.isValid(error))return false;auto o=p.toJson();o.insert("schemaVersion",1);return writeJson(path,o,error);}
bool ThemeStore::readTheme(const QString&path,Profile*p,QString*error){QJsonObject o;if(!readJson(path,&o,error))return false;return Profile::fromJson(o,p,error);}
QList<AnimationPreset> ThemeStore::loadCustomPresets(QString*error){
    QList<AnimationPreset> result;
    const QString path=configDirectory()+"/presets.json";
    if(!QFile::exists(path))return result;
    QJsonObject root;
    if(!readJson(path,&root,error))return {};
    if(root.value("schemaVersion").toInt(-1)!=1){
        if(error)*error="Unsupported preset collection schema.";
        return {};
    }
    QSet<QString> ids;
    for(const QJsonValue &value:root.value("presets").toArray()){
        AnimationPreset preset;
        if(!AnimationPreset::fromJson(value.toObject(),&preset,error))return {};
        if(ids.contains(preset.id)){
            if(error)*error="Custom preset identifiers must be unique.";
            return {};
        }
        ids.insert(preset.id);
        result.append(preset);
    }
    if(error)error->clear();
    return result;
}
bool ThemeStore::saveCustomPresets(const QList<AnimationPreset>&presets,QString*error){
    if(!ensureDir(error))return false;
    QJsonArray values;
    QSet<QString> ids;
    for(const AnimationPreset &preset:presets){
        if(!preset.custom||preset.id.isEmpty()||ids.contains(preset.id)||
           !preset.profile.isValid(error)){
            if(error&&error->isEmpty())*error="Only unique, valid custom presets can be stored.";
            return false;
        }
        ids.insert(preset.id);
        values.append(preset.toJson());
    }
    return writeJson(configDirectory()+"/presets.json",
                     {{"schemaVersion",1},{"presets",values}},error);
}
bool ThemeStore::writePreset(const QString&path,const AnimationPreset&p,QString*error){
    if(!p.profile.isValid(error))return false;
    return writeJson(path,p.toJson(),error);
}
bool ThemeStore::readPreset(const QString&path,AnimationPreset*p,QString*error){
    QJsonObject object;
    if(!readJson(path,&object,error))return false;
    return AnimationPreset::fromJson(object,p,error);
}
