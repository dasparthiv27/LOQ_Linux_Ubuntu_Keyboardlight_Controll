#include "profile.h"
#include "animationengine.h"
#include "animationpresets.h"
#include "colorgradient.h"
#include "keyboardlayout.h"
#include "keyboardzonemap.h"
#include "keyboardinputlistener.h"
#include "hardware/keyboardbackend.h"
#include "hardware/ite8295protocol.h"
#include "themestore.h"
#include <QtTest>
#include <QJsonArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <linux/input-event-codes.h>

namespace {

QByteArray featureDescriptor()
{
    return QByteArray::fromHex("750885cc9520b102");
}

bool writeFile(const QString &path, const QByteArray &contents)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(contents) == contents.size();
}

class MockHidrawTransport final : public HidrawTransport {
public:
    QString path = QStringLiteral("/mock/hidraw-device");
    QString discoveryError;
    bool sendResult = true;
    QString sendError;
    int sendCount = 0;
    QByteArray sentReport;

    QString discoverInterface0(QString *error) override
    {
        if (error) {
            *error = discoveryError;
        }
        return discoveryError.isEmpty() ? path : QString();
    }

    bool sendFeatureReport(const QByteArray &report,
                           QString *error) override
    {
        ++sendCount;
        sentReport = report;
        if (error) {
            *error = sendError;
        }
        return sendResult;
    }
};

} // namespace

class CoreTests final:public QObject {
    Q_OBJECT
private slots:
    void builtInPresetsHaveFourZones(){const auto themes=builtInProfiles();QCOMPARE(themes.size(),10);for(const auto&p:themes){QVERIFY(p.isValid());QCOMPARE(p.zones.size(),size_t(4));}}
    void jsonRoundTrip(){Profile p;p.name="Round trip";p.effect=Effect::Wave;p.direction=Direction::RightToLeft;p.brightness=42;p.speed=77;p.zones={QColor("#123456"),QColor("#abcdef"),QColor("#010203"),QColor("#fefdfc")};auto j=p.toJson();j.insert("schemaVersion",1);Profile decoded;QString error;QVERIFY2(Profile::fromJson(j,&decoded,&error),qPrintable(error));QVERIFY(decoded==p);}
    void rejectsUnsupportedSchemaAndZoneCounts(){Profile out;QString error;QVERIFY(!Profile::fromJson({{"schemaVersion",2}},&out,&error));QVERIFY(error.contains("schema"));QJsonObject invalid={{"schemaVersion",1},{"name","Broken"},{"effect","Static"},{"brightness",80},{"speed",40},{"direction","ltr"},{"zones",QJsonArray{QJsonValue(QStringLiteral("#ffffff"))}}};QVERIFY(!Profile::fromJson(invalid,&out,&error));QVERIFY(error.contains("exactly four"));}
    void simulationBackendStoresValidProfile(){SimulationBackend backend;Profile p;p.name="Sim";QString error;QVERIFY(backend.apply(p,&error));QVERIFY(backend.isSimulation());QVERIFY(backend.lastApplied()==p);p.speed=101;QVERIFY(!backend.apply(p,&error));}
    void gradientInterpolatesAndValidatesStops(){
        ColorGradient gradient({{0.0,QColor("red")},{1.0,QColor("blue")}});
        QCOMPARE(gradient.sample(0.0),QColor("red"));
        QCOMPARE(gradient.sample(1.0),QColor("blue"));
        QCOMPARE(gradient.sample(0.5).red(),128);
        QCOMPARE(gradient.sample(0.5).blue(),128);
        QString error;
        QVERIFY(!gradient.setStops({{0.2,QColor("red")},{1.0,QColor("blue")}},&error));
        QVERIFY(error.contains("endpoints"));
        QVERIFY(!gradient.setStops({{0.0,QColor("red")}},&error));
        QVERIFY(gradient.setStops({{1.0,QColor("blue")},{0.5,QColor("green")},{0.0,QColor("red")}},&error));
        QCOMPARE(gradient.stops().size(),3);
        QCOMPARE(gradient.sample(0.0),QColor("red"));
        QCOMPARE(gradient.sample(1.0),QColor("blue"));
        QVERIFY(gradient.sample(0.5).green()>100);
    }
    void presetLibraryCoversRequestedCategories(){
        const auto presets=builtInAnimationPresets();
        QCOMPARE(presets.size(),8);
        for(const auto &preset:presets){
            QVERIFY(!preset.id.isEmpty());
            QVERIFY(!preset.name.isEmpty());
            QVERIFY(!preset.description.isEmpty());
            QVERIFY(preset.profile.isValid());
            QVERIFY(!preset.custom);
            QVERIFY(preset.profile.effect==Effect::Static ||
                    preset.profile.effect==Effect::Wave ||
                    preset.profile.effect==Effect::Breathing);
            QCOMPARE(preset.profile.zones.size(),size_t(4));
        }
        QCOMPARE(presets[0].profile.effect,Effect::Wave);
        QCOMPARE(presets[0].profile.direction,Direction::LeftToRight);
        QCOMPARE(presets[1].profile.direction,Direction::RightToLeft);
        QCOMPARE(presets[2].colorMode,PresetColorMode::FixedSections);
        QCOMPARE(presets[5].profile.effect,Effect::Breathing);
        QCOMPARE(presets[7].colorMode,PresetColorMode::Individual);
    }
    void presetColorModesRespectFourZones(){
        AnimationPreset preset;
        preset.profile.zones={QColor("#112233"),QColor("#445566"),
                              QColor("#778899"),QColor("#aabbcc")};
        applyPresetColorMode(&preset,PresetColorMode::Same);
        QCOMPARE(preset.profile.zones[1],QColor("#112233"));
        QCOMPARE(preset.profile.zones[2],QColor("#112233"));
        QCOMPARE(preset.profile.zones[3],QColor("#112233"));
        preset.profile.zones={QColor("#112233"),QColor("#445566"),
                              QColor("#778899"),QColor("#aabbcc")};
        applyPresetColorMode(&preset,PresetColorMode::Individual);
        QCOMPARE(preset.profile.zones[0],QColor("#112233"));
        QCOMPARE(preset.profile.zones[3],QColor("#aabbcc"));
        QCOMPARE(preset.colorMode,PresetColorMode::Individual);
    }
    void customPresetOperationsAndCycleWraparound(){
        const auto builtIns=builtInAnimationPresets();
        QList<AnimationPreset> custom;
        QString error;
        QVERIFY(!renameCustomPreset(custom,builtIns[0].id,"Changed",&error));
        QVERIFY(error.contains("not found"));
        QVERIFY(!deleteCustomPreset(custom,builtIns[0].id,&error));
        QCOMPARE(nextCustomPresetIndex(builtIns,""),-1);

        QVERIFY(duplicatePreset(builtIns,&custom,builtIns[0].id,"custom-a","Personal wave",&error));
        QVERIFY(duplicatePreset(builtIns+custom,&custom,builtIns[1].id,"custom-b","Personal reverse",&error));
        QList<AnimationPreset> all=builtIns+custom;
        QVERIFY(renameCustomPreset(custom,"custom-a","Renamed wave",&error));
        QCOMPARE(custom[0].name,QString("Renamed wave"));
        QCOMPARE(custom[0].profile.name,QString("Renamed wave"));
        QCOMPARE(nextCustomPresetIndex(all,""),builtIns.size());
        QCOMPARE(nextCustomPresetIndex(all,"custom-a"),builtIns.size()+1);
        QCOMPARE(nextCustomPresetIndex(all,"custom-b"),builtIns.size());
        QVERIFY(deleteCustomPreset(custom,"custom-a",&error));
        QCOMPARE(custom.size(),1);
        QVERIFY(!deleteCustomPreset(all,builtIns[0].id,&error));
    }
    void animationPresetJsonRoundTripAndValidation(){
        AnimationPreset source;
        source.id="custom-1";
        source.name="Reactive sample";
        source.description="Persisted key glow preset.";
        source.category=PresetCategory::Reactive;
        source.profile.name=source.name;
        source.profile.effect=Effect::Static;
        source.preview=PreviewEffect::KeyGlow;
        source.colorMode=PresetColorMode::Individual;
        source.durationSeconds=45;
        source.fadeInSeconds=3;
        source.fadeOutSeconds=5;
        source.looping=false;
        source.reverse=true;
        source.custom=true;
        AnimationPreset decoded;
        QString error;
        QVERIFY2(AnimationPreset::fromJson(source.toJson(),&decoded,&error),qPrintable(error));
        QCOMPARE(decoded.id,source.id);
        QCOMPARE(decoded.category,source.category);
        QCOMPARE(decoded.preview,source.preview);
        QCOMPARE(decoded.profile,source.profile);
        QCOMPARE(decoded.durationSeconds,45);
        QCOMPARE(decoded.fadeInSeconds,3);
        QCOMPARE(decoded.fadeOutSeconds,5);
        QVERIFY(!decoded.looping);
        QVERIFY(decoded.reverse);

        source.colorMode=PresetColorMode::Single;
        source.profile.effect=Effect::Breathing;
        applyPresetColorMode(&source,source.colorMode);
        QVERIFY2(AnimationPreset::fromJson(source.toJson(),&decoded,&error),qPrintable(error));
        QCOMPARE(decoded.colorMode,PresetColorMode::Single);

        source.fadeOutSeconds=1000;
        QVERIFY(!AnimationPreset::fromJson(source.toJson(),&decoded,&error));
        QVERIFY(error.contains("duration"));

        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        source.fadeOutSeconds=5;
        const QString path=directory.filePath("preset.json");
        QVERIFY2(ThemeStore::writePreset(path,source,&error),qPrintable(error));
        QVERIFY2(ThemeStore::readPreset(path,&decoded,&error),qPrintable(error));
        QCOMPARE(decoded.name,source.name);
    }
    void animationEngineSupportsPauseDurationAndReverse(){
        AnimationEngine engine;
        Profile p;p.name="Playback";p.effect=Effect::Wave;p.speed=50;p.brightness=100;
        engine.setProfile(p);
        engine.setPreviewEffect(PreviewEffect::Wave);
        engine.setPlayback(1.0,0.0,0.2,false,false);
        engine.advance(0.1);
        QVERIFY(engine.elapsed()>0.0);
        const qreal phase=engine.phase();
        engine.setPaused(true);
        engine.advance(0.2);
        QCOMPARE(engine.phase(),phase);
        engine.setPaused(false);
        engine.advance(0.2);
        QVERIFY(engine.elapsed()>0.1);
        for(int i=0;i<5;++i)engine.advance(0.2);
        QVERIFY(engine.isPaused());
        QCOMPARE(engine.elapsed(),1.0);

        engine.setPlayback(10.0,0.0,0.0,true,true);
        const qreal reverseStart=engine.phase();
        engine.advance(0.1);
        QVERIFY(engine.phase()<reverseStart);
    }
    void keyReactiveEffectsAccumulateAndCoolWithoutText(){
        const auto layout=KeyboardLayout::createLenovoLoq15Layout();
        const auto map=KeyboardZoneMap::createDefaultLoqMap(layout);
        int keyId=-1;
        const KeyGeometry *key=nullptr;
        for(const auto &candidate:layout.keys())if(candidate.label=="A"){keyId=candidate.id;key=&candidate;break;}
        QVERIFY(keyId>0&&key);
        AnimationEngine engine;
        Profile p;p.name="Reactive";p.brightness=100;
        engine.setProfile(p);
        engine.setReactiveDecay(0.5);
        engine.setReactiveColor(QColor("#ff3030"));
        engine.setPreviewEffect(PreviewEffect::KeyGlow);
        const QPointF center(key->x+key->width/2.0,key->y+key->height/2.0);
        engine.registerKeyPress(keyId,center);
        const auto lit=engine.keyFrame(layout,map);
        QVERIFY(lit.contains(keyId));
        QVERIFY(lit.value(keyId)!=engine.frame()[static_cast<size_t>(map.zoneForKey(keyId))]);
        for(int i=0;i<16;++i)engine.advance(0.25);
        const auto cooled=engine.keyFrame(layout,map);
        QCOMPARE(cooled.value(keyId),engine.frame()[static_cast<size_t>(map.zoneForKey(keyId))]);

        engine.setPreviewEffect(PreviewEffect::WaterRipple);
        engine.registerKeyPress(keyId,center);
        QVERIFY(!engine.keyFrame(layout,map).isEmpty());
    }
    void keyboardInputMapsOnlyCodesToGeometryLabels(){
        QCOMPARE(KeyboardInputListener::keyLabelForCode(KEY_Q),QStringLiteral("Q"));
        QCOMPARE(KeyboardInputListener::keyLabelForCode(KEY_A),QStringLiteral("A"));
        QCOMPARE(KeyboardInputListener::keyLabelForCode(KEY_LEFTSHIFT),QStringLiteral("LShift"));
        QCOMPARE(KeyboardInputListener::keyLabelForCode(KEY_KP7),QStringLiteral("7"));
        QVERIFY(KeyboardInputListener::isNumpadCode(KEY_KP7));
        QVERIFY(!KeyboardInputListener::isNumpadCode(KEY_7));
        QVERIFY(KeyboardInputListener::keyLabelForCode(KEY_RESERVED).isEmpty());
    }
    void hidrawIdentityHandlesSignedProductIdAndRejectsOtherDevices(){
        QVERIFY(LinuxHidrawTransport::matchesRawIdentity(0x03,0x048D,-13933));
        QVERIFY(!LinuxHidrawTransport::matchesRawIdentity(0x03,0x048D,0xC992));
        QVERIFY(!LinuxHidrawTransport::matchesRawIdentity(0x03,0x1234,-13933));
        QVERIFY(!LinuxHidrawTransport::matchesRawIdentity(0x01,0x048D,-13933));
    }
    void featureDescriptorParsesDataBytesExcludingReportId(){
        const QByteArray descriptor=featureDescriptor();
        QCOMPARE(Ite8295Protocol::featureDataLength(descriptor,0xCC),32);
        QCOMPARE(Ite8295Protocol::featureDataLength(descriptor,0xCD),-1);
        QCOMPARE(Ite8295Protocol::featureDataLength(QByteArray::fromHex("750885cc9520"),0xCC),-1);
        QCOMPARE(Ite8295Protocol::featureDataLength(QByteArray::fromHex("fe"),0xCC),-1);
    }
    void staticPacketHasDescriptorSizedHidrawFraming(){
        Profile p;
        p.name="Packet test";
        p.effect=Effect::Static;
        p.brightness=80;
        p.speed=50;
        p.zones={QColor("#18d9ff"),QColor("#a43cff"),QColor("#ff3d88"),QColor("#ffb02e")};
        QString error;
        const QByteArray report=Ite8295Protocol::buildFeatureReport(p,&error);
        QVERIFY2(!report.isEmpty(),qPrintable(error));
        QCOMPARE(report.size(),33);
        QCOMPARE(report,QByteArray::fromHex(
            "cc1601020218d9ffa43cffff3d88ffb02e00000000000000000000000000000000"));
    }
    void protocolRejectsZeroBrightnessAndInvalidEnum(){
        Profile p;
        p.brightness=0;
        QString error;
        QVERIFY(Ite8295Protocol::buildFeatureReport(p,&error).isEmpty());
        QVERIFY(error.contains("brightness"));
        p.brightness=80;
        p.effect=static_cast<Effect>(99);
        QVERIFY(Ite8295Protocol::buildFeatureReport(p,&error).isEmpty());
        QVERIFY(error.contains("effect"));
    }
    void protocolEncodesReferenceEffectsAndWaveDirections(){
        Profile p;
        p.name="Effect test";
        p.brightness=100;
        p.speed=100;
        QString error;
        const QList<QPair<Effect,unsigned char>> cases={
            {Effect::Static,0x01},{Effect::Breathing,0x03},
            {Effect::Wave,0x04},{Effect::Hue,0x06}};
        for(const auto &item:cases){
            p.effect=item.first;
            const QByteArray report=Ite8295Protocol::buildFeatureReport(p,&error);
            QVERIFY2(!report.isEmpty(),qPrintable(error));
            QCOMPARE(static_cast<unsigned char>(report.at(2)),item.second);
            QCOMPARE(static_cast<unsigned char>(report.at(3)),static_cast<unsigned char>(4));
            QCOMPARE(static_cast<unsigned char>(report.at(4)),static_cast<unsigned char>(2));
        }
        p.effect=Effect::Wave;
        p.direction=Direction::LeftToRight;
        QByteArray report=Ite8295Protocol::buildFeatureReport(p,&error);
        QCOMPARE(static_cast<unsigned char>(report.at(18)),static_cast<unsigned char>(0));
        QCOMPARE(static_cast<unsigned char>(report.at(19)),static_cast<unsigned char>(1));
        p.direction=Direction::RightToLeft;
        report=Ite8295Protocol::buildFeatureReport(p,&error);
        QCOMPARE(static_cast<unsigned char>(report.at(18)),static_cast<unsigned char>(1));
        QCOMPARE(static_cast<unsigned char>(report.at(19)),static_cast<unsigned char>(0));
    }
    void backendUsesMockTransportAndReportsFailure(){
        MockHidrawTransport transport;
        Ite8295Backend backend(&transport);
        Profile p;
        p.name="Mock";
        QString error;
        QVERIFY(backend.apply(p,&error));
        QCOMPARE(transport.sendCount,1);
        QCOMPARE(transport.sentReport.size(),33);

        transport.sendResult=false;
        transport.sendError="mock transport failure";
        QVERIFY(!backend.apply(p,&error));
        QCOMPARE(error,QStringLiteral("mock transport failure"));
        QCOMPARE(transport.sendCount,2);

    }
    void discoverySelectsOnlyMatchingInterfaceZero(){
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString sysClass=temp.filePath("sys/class/hidraw");
        const QString deviceRoot=temp.filePath("dev");
        const QString usbRoot=temp.filePath("sys/devices/pci0000/usb1/1-1");
        const QString interface0=usbRoot+"/1-1:1.0";
        const QString interface1=usbRoot+"/1-1:1.1";
        const QString hid0=interface0+"/0003:048D:C993.0001";
        const QString hid1=interface1+"/0003:048D:C993.0002";
        QVERIFY(QDir().mkpath(sysClass+"/hidraw0"));
        QVERIFY(QDir().mkpath(sysClass+"/hidraw1"));
        QVERIFY(QDir().mkpath(hid0));
        QVERIFY(QDir().mkpath(hid1));
        QVERIFY(QDir().mkpath(deviceRoot));
        QVERIFY(writeFile(usbRoot+"/idVendor","048d"));
        QVERIFY(writeFile(usbRoot+"/idProduct","c993"));
        QVERIFY(writeFile(interface0+"/bInterfaceNumber","00"));
        QVERIFY(writeFile(interface1+"/bInterfaceNumber","01"));
        QVERIFY(writeFile(hid0+"/report_descriptor",featureDescriptor()));
        QVERIFY(writeFile(hid1+"/report_descriptor",featureDescriptor()));
        QVERIFY(writeFile(deviceRoot+"/hidraw0",""));
        QVERIFY(writeFile(deviceRoot+"/hidraw1",""));
        QVERIFY(QFile::link(hid1,sysClass+"/hidraw0/device"));
        QVERIFY(QFile::link(hid0,sysClass+"/hidraw1/device"));

        LinuxHidrawTransport transport(sysClass,deviceRoot);
        QString error;
        QCOMPARE(transport.discoverInterface0(&error),deviceRoot+"/hidraw1");
        QVERIFY2(error.isEmpty(),qPrintable(error));

        QFile::remove(sysClass+"/hidraw0/device");
        QFile::remove(sysClass+"/hidraw1/device");
        QVERIFY(QFile::link(hid0,sysClass+"/hidraw0/device"));
        QVERIFY(QFile::link(hid1,sysClass+"/hidraw1/device"));
        QCOMPARE(transport.discoverInterface0(&error),deviceRoot+"/hidraw0");
        QVERIFY2(error.isEmpty(),qPrintable(error));

        QFile::remove(sysClass+"/hidraw1/device");
        QVERIFY(QFile::link(hid1,sysClass+"/hidraw1/device"));
        QVERIFY(writeFile(usbRoot+"/idVendor","1234"));
        QVERIFY(transport.discoverInterface0(&error).isEmpty());
        QVERIFY(error.contains("not 048D:C993"));
    }
    void animationChangesFourZoneFrames(){AnimationEngine e;Profile p;p.effect=Effect::Hue;e.setProfile(p);auto first=e.frame();e.advance(.2);auto second=e.frame();QCOMPARE(first.size(),size_t(4));QVERIFY(first!=second);}
    void softwarePreviewEffectsStayFourZone(){AnimationEngine e;Profile p;e.setProfile(p);e.setPreviewEffect(PreviewEffect::Fire);auto a=e.frame();e.advance(.17);auto b=e.frame();QCOMPARE(a.size(),size_t(4));QVERIFY(a!=b);e.setPreviewEffect(PreviewEffect::AudioVisualizer);QCOMPARE(e.frame().size(),size_t(4));e.setAudioLevel(0.8);QCOMPARE(e.frame().size(),size_t(4));}
    void directionalKeyPreviewEffectsRender(){
        const KeyboardLayout layout=KeyboardLayout::createLenovoLoq15Layout();
        const KeyboardZoneMap map=KeyboardZoneMap::createDefaultLoqMap(layout);
        AnimationEngine engine;
        Profile profile;
        profile.brightness=100;
        engine.setProfile(profile);
        for(const PreviewEffect effect:{PreviewEffect::OutsideIn,PreviewEffect::VerticalWave,
                                        PreviewEffect::BidirectionalWave,
                                        PreviewEffect::AlternatingDirection}){
            engine.setPreviewEffect(effect);
            QVERIFY(!engine.keyFrame(layout,map).isEmpty());
        }
    }
    void fourZonePresetEffectsPreviewModes(){
        AnimationEngine engine;
        Profile profile;
        profile.brightness=100;
        profile.speed=50;
        profile.effect=Effect::Wave;
        profile.zones={QColor("#ff0000"),QColor("#00ff00"),
                       QColor("#0000ff"),QColor("#ffff00")};
        engine.setProfile(profile);
        engine.setPreviewEffect(PreviewEffect::Profile);
        engine.setPresetColorMode(PresetColorMode::FixedSections);
        const auto fixedStart=engine.frame();
        engine.advance(0.25);
        const auto fixedLater=engine.frame();
        for(int zone=0;zone<4;++zone)
            QCOMPARE(fixedStart[static_cast<size_t>(zone)].hue(),
                     fixedLater[static_cast<size_t>(zone)].hue());
        profile.direction=Direction::RightToLeft;
        engine.setProfile(profile);
        const auto reverseWave=engine.frame();
        QVERIFY(reverseWave!=fixedLater);

        engine.setPresetColorMode(PresetColorMode::Multi);
        const auto multiStart=engine.frame();
        engine.advance(0.25);
        const auto multiLater=engine.frame();
        QVERIFY(multiStart!=multiLater);

        profile.effect=Effect::Breathing;
        profile.zones.fill(profile.zones[0]);
        engine.setProfile(profile);
        engine.setPresetColorMode(PresetColorMode::Single);
        const auto single=engine.frame();
        for(int zone=1;zone<4;++zone)
            QCOMPARE(single[static_cast<size_t>(zone)].hue(),single[0].hue());
        engine.setPresetColorMode(PresetColorMode::Multi);
        const auto breathingMultiStart=engine.frame();
        engine.advance(0.25);
        QVERIFY(engine.frame()!=breathingMultiStart);
        profile.zones={QColor("#ff0000"),QColor("#00ff00"),
                       QColor("#0000ff"),QColor("#ffff00")};
        engine.setProfile(profile);
        engine.setPresetColorMode(PresetColorMode::Individual);
        const auto individual=engine.frame();
        for(int zone=0;zone<4;++zone)
            QCOMPARE(individual[static_cast<size_t>(zone)].hue(),
                     profile.zones[static_cast<size_t>(zone)].hue());
    }
    void themeJsonFileRoundTrip(){QTemporaryDir dir;QVERIFY(dir.isValid());Profile p;p.name="Exportable";p.effect=Effect::Breathing;QString error;const QString file=dir.filePath("theme.json");QVERIFY2(ThemeStore::writeTheme(file,p,&error),qPrintable(error));Profile decoded;QVERIFY2(ThemeStore::readTheme(file,&decoded,&error),qPrintable(error));QVERIFY(decoded==p);}

    void fourZoneMappingIntegrity(){
        const auto layout=KeyboardLayout::createLenovoLoq15Layout();
        const auto map=KeyboardZoneMap::createDefaultLoqMap(layout);
        QVERIFY(layout.keys().size()>=90);
        int zoneKeyCount[4]={0,0,0,0};
        for(const auto&k:layout.keys()){
            const int z=map.zoneForKey(k.id);
            QVERIFY(z>=0&&z<4);
            zoneKeyCount[z]++;
        }
        for(int z=0;z<4;++z){
            QVERIFY(zoneKeyCount[z]>10);
            const auto keys=map.keysInZone(z);
            QCOMPARE(keys.size(),zoneKeyCount[z]);
            for(int id:keys){
                QCOMPARE(map.zoneForKey(id),z);
            }
        }
    }

    void clickingKeySelectsCorrectZone(){
        const auto layout=KeyboardLayout::createLenovoLoq15Layout();
        const auto map=KeyboardZoneMap::createDefaultLoqMap(layout);
        auto testKey=[&](const QString&label,int expectedZone,bool numpad=false){
            bool found=false;
            for(const auto&k:layout.keys()){
                if(k.label==label&&k.isNumpad==numpad){
                    found=true;
                    const int hitId=layout.keyAt(k.x+k.width*0.5,k.y+k.height*0.5);
                    QCOMPARE(hitId,k.id);
                    QCOMPARE(map.zoneForKey(hitId),expectedZone);
                    break;
                }
            }
            QVERIFY2(found,qPrintable(QString("Key not found: %1 (numpad=%2)").arg(label).arg(numpad)));
        };
        // Zone 0: Left QWERTY cluster
        testKey("Esc",0);
        testKey("Tab",0);
        testKey("Q",0);
        testKey("Caps",0);
        // Zone 1: Center-Left cluster
        testKey("F6",1);
        testKey("T",1);
        testKey("G",1);
        testKey("Space",1);
        // Zone 2: Center-Right and Arrow cluster
        testKey("F10",2);
        testKey("Enter",2);
        testKey("▲",2);
        testKey("►",2);
        // Zone 3: Dedicated Numpad
        testKey("Num",3,true);
        testKey("7",3,true);
        testKey("0",3,true);
    }

    void colourChangesPropagatePerZoneAndIsolateOthers(){
        const auto layout=KeyboardLayout::createLenovoLoq15Layout();
        const auto map=KeyboardZoneMap::createDefaultLoqMap(layout);
        Profile p;
        p.effect=Effect::Static;
        p.brightness=100;
        p.zones={QColor("#ff0000"),QColor("#00ff00"),QColor("#0000ff"),QColor("#ffff00")};
        AnimationEngine engine;
        engine.setProfile(p);
        auto initialFrame=engine.frame();
        QCOMPARE(initialFrame[1],QColor("#00ff00"));

        // Change only Zone 1 (Center-Left)
        p.zones[1]=QColor("#ff00ff");
        engine.setProfile(p);
        auto updatedFrame=engine.frame();

        // Zone 1 updated, all other zones preserved
        QCOMPARE(updatedFrame[1],QColor("#ff00ff"));
        QCOMPARE(updatedFrame[0],QColor("#ff0000"));
        QCOMPARE(updatedFrame[2],QColor("#0000ff"));
        QCOMPARE(updatedFrame[3],QColor("#ffff00"));

        // Check that keys in Zone 1 reflect the updated color while keys in Zone 0 do not
        const auto z1Keys=map.keysInZone(1);
        for(int id:z1Keys){
            const int z=map.zoneForKey(id);
            QCOMPARE(updatedFrame[static_cast<size_t>(z)],QColor("#ff00ff"));
        }
    }

    void hexAndRgbParsingConsistency(){
        const QString hex="#18D9FF";
        const QString rgbStr="24, 217, 255";
        QColor fromHex(hex);
        QVERIFY(fromHex.isValid());

        const auto parts=rgbStr.split(',');
        QCOMPARE(parts.size(),3);
        bool ok=true;
        int v[3]={};
        for(int i=0;i<3;++i) v[i]=parts[i].trimmed().toInt(&ok);
        QVERIFY(ok);
        QColor fromRgb(v[0],v[1],v[2]);

        QCOMPARE(fromHex,fromRgb);
        QCOMPARE(fromHex.red(),24);
        QCOMPARE(fromHex.green(),217);
        QCOMPARE(fromHex.blue(),255);
    }

    void staticModePreservesColors(){
        Profile p;
        p.effect=Effect::Static;
        p.brightness=100;
        p.zones={QColor("#112233"),QColor("#445566"),QColor("#778899"),QColor("#aabbcc")};
        AnimationEngine e;
        e.setProfile(p);
        QCOMPARE(e.frame(),p.zones);
        e.advance(0.1);
        QCOMPARE(e.frame(),p.zones);
        e.advance(0.4);
        QCOMPARE(e.frame(),p.zones);
    }

    void unverifiedMappingExplicitlyTracked(){
        const auto layout=KeyboardLayout::createLenovoLoq15Layout();
        const auto map=KeyboardZoneMap::createDefaultLoqMap(layout);
        QVERIFY(!map.isVerified());
        QVERIFY(!map.provenance().isEmpty());
        QVERIFY(map.provenance().contains("Lenovo Vantage",Qt::CaseInsensitive));
    }
    void zoneMapCanBeCalibratedWithoutChangingGeometry(){
        const auto layout=KeyboardLayout::createLenovoLoq15Layout();
        auto map=KeyboardZoneMap::createDefaultLoqMap(layout);
        int leftShift=-1,rightShift=-1;
        for(const auto &key:layout.keys()){
            if(key.label=="LShift")leftShift=key.id;
            if(key.label=="RShift")rightShift=key.id;
        }
        QVERIFY(leftShift>0&&rightShift>0);
        QCOMPARE(map.zoneForKey(leftShift),0);
        QCOMPARE(map.zoneForKey(rightShift),2);
        QCOMPARE(map.zoneForKey(-100),-1);
        map.setKeyZone(leftShift,3);
        QCOMPARE(map.zoneForKey(leftShift),3);
        QVERIFY(layout.findKey(leftShift));
        QCOMPARE(layout.findKey(leftShift)->label,QStringLiteral("LShift"));
    }
};
QTEST_GUILESS_MAIN(CoreTests)
#include "tst_core.moc"
