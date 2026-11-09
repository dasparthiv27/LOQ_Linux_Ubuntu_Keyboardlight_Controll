#include "mainwindow.h"
#include <QApplication>
#include <QCheckBox>
#include <QColorDialog>
#include <QCloseEvent>
#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonObject>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardPaths>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QDir>
#include <QFile>
#include <QMenu>
#include <QStyle>
#include <QIcon>
#include <QDialog>
#include <QDialogButtonBox>
#include <QShortcut>
#include <QKeySequence>
#include <QScrollArea>
#include <QPainter>
#include <QPixmap>
#include <QUuid>
#include <algorithm>

namespace {
QLabel* label(const QString&s,const QString&style={}){auto*l=new QLabel(s);if(!style.isEmpty())l->setStyleSheet(style);return l;}
QPushButton* button(const QString&s,const QString&extra={}){auto*b=new QPushButton(s);b->setCursor(Qt::PointingHandCursor);if(!extra.isEmpty())b->setProperty("accent",extra);return b;}
}
MainWindow::MainWindow(bool restoreMode,QWidget*p):QMainWindow(p){
    qRegisterMetaType<Profile>("Profile");
    setWindowTitle("LOQ RGB Studio");resize(1180,790);setMinimumSize(980,680);m_custom=ThemeStore::loadCustom();QString ignored;ThemeStore::loadLastApplied(&m_profile,&ignored);m_engine.setProfile(m_profile);
    auto*central=new QWidget(this);setCentralWidget(central);auto*root=new QHBoxLayout(central);root->setContentsMargins(0,0,0,0);root->setSpacing(0);auto*sidebar=new QWidget(central);sidebar->setObjectName("sidebar");sidebar->setFixedWidth(218);auto*side=new QVBoxLayout(sidebar);side->setContentsMargins(18,24,18,18);side->setSpacing(8);
    auto*brand=new QLabel("◈  LOQ <span style='color:#18d9ff'>RGB</span><br><span style='font-size:11px;color:#78849a'>STUDIO</span>");brand->setTextFormat(Qt::RichText);brand->setStyleSheet("font-size:20px;font-weight:800;padding:4px 8px 24px");side->addWidget(brand);
    const QStringList names={"✦   Presets","▣   Hardware","⚙   Settings"};m_pages=new QStackedWidget;
    for(int i=0;i<names.size();++i){auto*b=new QPushButton(names[i]);b->setCheckable(true);b->setProperty("nav",true);side->addWidget(b);connect(b,&QPushButton::clicked,this,[this,i]{navTo(i);});}
    side->addStretch();m_modeIndicator=label("●  SIMULATION MODE","color:#5df0b0;font-size:11px;font-weight:700;padding:10px 8px;background:#14251f;border-radius:8px");side->addWidget(m_modeIndicator);
    root->addWidget(sidebar);root->addWidget(m_pages,1);
    m_keyboardLayout=KeyboardLayout::createLenovoLoq15Layout();
    m_zoneMap=KeyboardZoneMap::createDefaultLoqMap(m_keyboardLayout);
    m_presets=builtInAnimationPresets();
    QString presetLoadError;
    m_customPresets=ThemeStore::loadCustomPresets(&presetLoadError);
    if(!presetLoadError.isEmpty())qWarning()<<"Could not load custom presets:"<<presetLoadError;
    m_presets.append(m_customPresets);
    m_pages->addWidget(buildDashboard());m_pages->addWidget(buildSimplePresets());m_pages->addWidget(buildStudio());m_pages->addWidget(buildReactive());m_pages->addWidget(buildColorEditor());m_pages->addWidget(buildThemes());m_pages->addWidget(buildDevice());m_pages->addWidget(buildSettings());
    m_previewMode->clear();
    m_previewMode->addItems({"Profile effect","Rainbow Flow","Fire Flicker","Water Ripple",
                             "Colour transition","Breathing","Wave","Aurora","Colour pulse",
                             "Alternating zones","Centre outward","Back and forth","Key glow","Heat map",
                             "Outside inward","Vertical wave","Bidirectional wave","Alternating direction"});
    connect(m_previewMode,qOverload<int>(&QComboBox::currentIndexChanged),this,[this](int index){
        const PreviewEffect effects[]={PreviewEffect::Profile,PreviewEffect::Rainbow,PreviewEffect::Fire,
            PreviewEffect::WaterRipple,PreviewEffect::ColorTransition,PreviewEffect::Breathing,
            PreviewEffect::Wave,PreviewEffect::Aurora,PreviewEffect::ColorPulse,
            PreviewEffect::AlternatingZones,PreviewEffect::CenterOut,PreviewEffect::PingPong,
            PreviewEffect::KeyGlow,PreviewEffect::HeatMap,PreviewEffect::OutsideIn,
            PreviewEffect::VerticalWave,PreviewEffect::BidirectionalWave,
            PreviewEffect::AlternatingDirection};
        m_engine.setPreviewEffect(effects[qBound(0,index,17)]);
    });
    QVBoxLayout *timelineLayout=nullptr;
    auto*timeline=panel("TIMELINE · SOFTWARE PREVIEW",&timelineLayout);
    auto*timelineForm=new QFormLayout;
    m_duration=new QSlider(Qt::Horizontal);m_duration->setRange(1,120);m_duration->setValue(30);
    m_fadeIn=new QSlider(Qt::Horizontal);m_fadeIn->setRange(0,10);m_fadeIn->setValue(0);
    m_fadeOut=new QSlider(Qt::Horizontal);m_fadeOut->setRange(0,10);m_fadeOut->setValue(0);
    m_looping=new QCheckBox("Loop");m_looping->setChecked(true);
    m_reverse=new QCheckBox("Reverse");
    timelineForm->addRow("Duration (seconds)",m_duration);
    timelineForm->addRow("Fade in (seconds)",m_fadeIn);
    timelineForm->addRow("Fade out (seconds)",m_fadeOut);
    timelineLayout->addLayout(timelineForm);
    auto*playbackRow=new QHBoxLayout;
    playbackRow->addWidget(m_looping);playbackRow->addWidget(m_reverse);
    auto*pause=button("Pause preview");
    auto*reset=button("Reset");
    m_animationProgress=label("Preview timeline ready","color:#8c98ad");
    playbackRow->addWidget(pause);playbackRow->addWidget(reset);
    playbackRow->addStretch();
    timelineLayout->addLayout(playbackRow);
    timelineLayout->addWidget(m_animationProgress);
    auto*studioLayout=qobject_cast<QVBoxLayout*>(m_pages->widget(2)->layout());
    if(studioLayout)studioLayout->addWidget(timeline);
    auto updatePlayback=[this]{
        m_engine.setPlayback(m_duration->value(),m_fadeIn->value(),m_fadeOut->value(),
                             m_looping->isChecked(),m_reverse->isChecked());
    };
    connect(m_duration,&QSlider::valueChanged,this,[updatePlayback]{updatePlayback();});
    connect(m_fadeIn,&QSlider::valueChanged,this,[updatePlayback]{updatePlayback();});
    connect(m_fadeOut,&QSlider::valueChanged,this,[updatePlayback]{updatePlayback();});
    connect(m_looping,&QCheckBox::toggled,this,[updatePlayback]{updatePlayback();});
    connect(m_reverse,&QCheckBox::toggled,this,[updatePlayback]{updatePlayback();});
    connect(pause,&QPushButton::clicked,this,[this,pause]{
        const bool paused=!m_engine.isPaused();
        m_engine.setPaused(paused);
        pause->setText(paused?"Resume preview":"Pause preview");
    });
    connect(reset,&QPushButton::clicked,this,[this,updatePlayback,pause]{
        m_engine.clearReactiveState();
        updatePlayback();
        m_engine.setPaused(false);
        pause->setText("Pause preview");
        updatePreview();
    });
    updatePlayback();
    for(int pageIndex=1;pageIndex<=4;++pageIndex){
        QWidget*content=m_pages->widget(pageIndex);
        m_pages->removeWidget(content);
        auto*scroll=new QScrollArea;
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setWidgetResizable(true);
        scroll->setWidget(content);
        m_pages->insertWidget(pageIndex,scroll);
    }
    for(KeyboardPreview *preview:{m_preview,m_studioPreview,m_reactivePreview,m_colorPreview}){
        if(!preview)continue;
        preview->setZoneMap(m_zoneMap);
        connect(preview,&KeyboardPreview::keyActivated,this,&MainWindow::handlePreviewKey);
    }
    m_liveEffect->setText(QString("%1  ·  %2% brightness").arg(effectName(m_profile.effect)).arg(m_profile.brightness));
    m_status=label("Ready · simulation only","color:#8d98ad;font-size:12px");QStatusBar*bar=statusBar();bar->addWidget(m_status);bar->addPermanentWidget(label("ITE8295  ·  4 zones  ·  v1.0.0","color:#657187;font-size:11px"));
    setStyleSheet("QMainWindow,QWidget{background:#0b0e14;color:#e8edf7;font-family:'Inter','Noto Sans',sans-serif;font-size:13px}#sidebar{background:#10141d;border-right:1px solid #202737}QPushButton{background:#1b2230;border:1px solid #293246;border-radius:9px;padding:10px 14px;color:#dfe6f3}QPushButton:hover{background:#252f41;border-color:#3e4b64}QPushButton[nav=true]{text-align:left;background:transparent;border:0;padding:12px 13px;color:#98a4b8}QPushButton[nav=true]:checked,QPushButton[nav=true]:hover{background:#1c2533;color:#18d9ff}QFrame[panel=true]{background:#111722;border:1px solid #232c3d;border-radius:16px}QComboBox,QLineEdit{background:#0d121a;border:1px solid #2a3447;border-radius:8px;padding:9px;color:#e8edf7}QSlider::groove:horizontal{height:5px;background:#2b3446;border-radius:2px}QSlider::sub-page:horizontal{background:#18d9ff;border-radius:2px}QSlider::handle:horizontal{background:#e9fbff;width:15px;margin:-6px 0;border-radius:7px}QListWidget{background:#0e131c;border:1px solid #242d3e;border-radius:10px;padding:4px}QListWidget::item{padding:10px;border-radius:7px}QListWidget::item:selected{background:#1c2a38;color:#25dcff}QStatusBar{background:#10141d;border-top:1px solid #202737;color:#8d98ad}");
    createTray();m_animationTimer.setInterval(32);connect(&m_animationTimer,&QTimer::timeout,this,[this]{m_engine.advance(0.032);updateAnimationControls();});m_animationTimer.start();connect(&m_engine,&AnimationEngine::frameChanged,this,&MainWindow::updatePreview);
    m_inputListener=new KeyboardInputListener(this);
    connect(m_inputListener,&KeyboardInputListener::keyPressed,this,[this](unsigned int code,qint64){
        const QString label=KeyboardInputListener::keyLabelForCode(code);
        if(label.isEmpty()||!m_reactivePreview)return;
        for(const KeyGeometry &key:m_reactivePreview->layout().keys()){
            if(key.label==label&&key.isNumpad==KeyboardInputListener::isNumpadCode(code)){
                handlePreviewKey(key.id,QPointF(key.x+key.width/2.0,key.y+key.height/2.0),m_zoneMap.zoneForKey(key.id));
                break;
            }
        }
    });
    connect(m_inputListener,&KeyboardInputListener::listenerError,this,[this](const QString &message){
        if(m_reactiveStatus)m_reactiveStatus->setText(message+" Use the clickable preview instead.");
        if(m_inputEnabled){m_inputEnabled->blockSignals(true);m_inputEnabled->setChecked(false);m_inputEnabled->blockSignals(false);}
    });
    connect(m_inputListener,&KeyboardInputListener::listenerStopped,this,[this]{
        if(m_inputEnabled&&m_inputEnabled->isChecked()){m_inputEnabled->blockSignals(true);m_inputEnabled->setChecked(false);m_inputEnabled->blockSignals(false);}
    });
    m_hardwareWorker=new HardwareApplyWorker;
    m_hardwareWorker->moveToThread(&m_hardwareThread);
    connect(&m_hardwareThread,&QThread::finished,m_hardwareWorker,&QObject::deleteLater);
    connect(this,&MainWindow::hardwareApplyRequested,m_hardwareWorker,&HardwareApplyWorker::applyProfile,Qt::QueuedConnection);
    connect(m_hardwareWorker,&HardwareApplyWorker::applyFinished,this,&MainWindow::hardwareApplyFinished,Qt::QueuedConnection);
    m_hardwareThread.setObjectName("ite8295-hid-worker");
    m_hardwareThread.start();
    m_cycleShortcut=new QShortcut(QKeySequence(QStringLiteral("Ctrl+Alt+N")),this);
    m_cycleShortcut->setContext(Qt::WindowShortcut);
    m_cycleShortcut->setAutoRepeat(false);
    connect(m_cycleShortcut,&QShortcut::activated,this,&MainWindow::cycleCustomPresets);
    if(restoreMode){applyProfile();QTimer::singleShot(250,[this]{hide();if(!m_tray||!m_tray->isVisible())qApp->quit();});}navTo(0);
}
MainWindow::~MainWindow(){if(m_inputListener)m_inputListener->stop();m_hardwareThread.quit();m_hardwareThread.wait();}
QWidget* MainWindow::makePage(const QString&t,const QString&s){auto*w=new QWidget;auto*l=new QVBoxLayout(w);l->setContentsMargins(34,28,34,24);l->setSpacing(18);l->addWidget(label(t,"font-size:25px;font-weight:750;color:#f4f7ff"));l->addWidget(label(s,"font-size:13px;color:#8792a6"));return w;}
QWidget* MainWindow::panel(const QString&t,QVBoxLayout**layout){auto*f=new QFrame;f->setProperty("panel",true);auto*l=new QVBoxLayout(f);l->setContentsMargins(20,18,20,18);l->setSpacing(12);if(!t.isEmpty())l->addWidget(label(t,"font-size:14px;font-weight:700;color:#dfe7f5"));if(layout)*layout=l;return f;}
QWidget* MainWindow::buildDashboard(){auto*w=makePage("Dashboard","Your four-zone keyboard, tuned your way.");auto*l=qobject_cast<QVBoxLayout*>(w->layout());auto*top=new QHBoxLayout;auto*welcome=panel("LIVE PROFILE");auto*wel=qobject_cast<QVBoxLayout*>(welcome->layout());m_liveEffect=label("Static  ·  80% brightness","font-size:19px;font-weight:700;color:#f2f6ff");wel->addWidget(m_liveEffect);wel->addWidget(label("Preview animation is software-rendered. Hardware changes require explicit confirmation.","color:#8792a6"));top->addWidget(welcome,2);auto*health=panel("DEVICE STATUS");auto*hl=qobject_cast<QVBoxLayout*>(health->layout());m_dashboardDeviceStatus=label("●  Simulation active","color:#61e6ac;font-weight:700");hl->addWidget(m_dashboardDeviceStatus);hl->addWidget(label("ITE8295  ·  048D:C993  ·  interface 0","color:#9aa6ba"));top->addWidget(health,1);l->addLayout(top);
    QVBoxLayout*pl;auto*pv=panel("FOUR-ZONE PREVIEW",&pl);m_preview=new KeyboardPreview;pl->addWidget(m_preview);connect(m_preview,&KeyboardPreview::zoneClicked,this,&MainWindow::selectZone);l->addWidget(pv,1);
    auto*actions=new QHBoxLayout;auto*go=button("✦  Open animation studio");connect(go,&QPushButton::clicked,this,[this]{navTo(2);});actions->addWidget(go);auto*apply=button("Apply simulation profile","accent");m_applyButtons.append(apply);apply->setStyleSheet("background:#0ba6c4;border-color:#18d9ff;font-weight:700;padding:12px 22px");connect(apply,&QPushButton::clicked,this,&MainWindow::applyProfile);actions->addStretch();actions->addWidget(apply);l->addLayout(actions);return w;}
QWidget* MainWindow::buildPresets(){
    auto*w=makePage("Preset library","Choose a profile to preview; hardware support is not implied.");
    auto*l=qobject_cast<QVBoxLayout*>(w->layout());
    auto*filter=new QHBoxLayout;
    m_presetCategory=new QComboBox;
    m_presetCategory->addItems({"All categories","Static","Gradient","Animated","Directional","Reactive"});
    filter->addWidget(label("CATEGORY","font-size:11px;color:#8c98ad;font-weight:700"));
    filter->addWidget(m_presetCategory,1);
    l->addLayout(filter);
    auto*row=new QHBoxLayout;
    auto*listPanel=panel("PRESETS · BUILT-IN AND CUSTOM");
    auto*listLayout=qobject_cast<QVBoxLayout*>(listPanel->layout());
    m_presetsList=new QListWidget;
    m_presetsList->setMinimumWidth(290);
    listLayout->addWidget(m_presetsList);
    row->addWidget(listPanel,2);
    QVBoxLayout*details;
    auto*detailPanel=panel("PRESET PREVIEW",&details);
    m_presetDescription=label("Select a preset to view its description.","color:#a6b1c4");
    m_presetDescription->setWordWrap(true);
    details->addWidget(m_presetDescription);
    m_presetCompatibility=label("Hardware compatibility: not verified","color:#e6c978");
    m_presetCompatibility->setWordWrap(true);
    details->addWidget(m_presetCompatibility);
    auto*buttons=new QHBoxLayout;
    auto*preview=button("Preview preset","accent");
    auto*studio=button("Open animation studio");
    buttons->addWidget(preview);
    buttons->addWidget(studio);
    details->addLayout(buttons);
    connect(preview,&QPushButton::clicked,this,[this]{
        if(!m_presetsList||!m_presetsList->currentItem())return;
        const QString id=m_presetsList->currentItem()->data(Qt::UserRole).toString();
        for(const AnimationPreset &preset:m_presets){
            if(preset.id!=id)continue;
            m_profile=preset.profile;
            m_activePresetId=preset.id;
            m_engine.setProfile(m_profile);
            m_engine.setPreviewEffect(preset.preview);
            int previewIndex=0;
            switch(preset.preview){
            case PreviewEffect::Profile:previewIndex=0;break;
            case PreviewEffect::Rainbow:previewIndex=1;break;
            case PreviewEffect::Fire:previewIndex=2;break;
            case PreviewEffect::Ripple:case PreviewEffect::WaterRipple:previewIndex=3;break;
            case PreviewEffect::ColorTransition:previewIndex=4;break;
            case PreviewEffect::Breathing:previewIndex=5;break;
            case PreviewEffect::Wave:previewIndex=6;break;
            case PreviewEffect::Aurora:previewIndex=7;break;
            case PreviewEffect::ColorPulse:previewIndex=8;break;
            case PreviewEffect::AlternatingZones:previewIndex=9;break;
            case PreviewEffect::CenterOut:previewIndex=10;break;
            case PreviewEffect::PingPong:previewIndex=11;break;
            case PreviewEffect::KeyGlow:previewIndex=12;break;
            case PreviewEffect::HeatMap:previewIndex=13;break;
            case PreviewEffect::OutsideIn:previewIndex=14;break;
            case PreviewEffect::VerticalWave:previewIndex=15;break;
            case PreviewEffect::BidirectionalWave:previewIndex=16;break;
            case PreviewEffect::AlternatingDirection:previewIndex=17;break;
            case PreviewEffect::AudioVisualizer:case PreviewEffect::SystemMonitor:previewIndex=0;break;
            }
            m_previewMode->setCurrentIndex(previewIndex);
            m_duration->setValue(preset.durationSeconds);
            m_fadeIn->setValue(preset.fadeInSeconds);
            m_fadeOut->setValue(preset.fadeOutSeconds);
            m_looping->setChecked(preset.looping);
            m_reverse->setChecked(preset.reverse);
            m_engine.setPlayback(preset.durationSeconds,preset.fadeInSeconds,
                                 preset.fadeOutSeconds,preset.looping,preset.reverse);
            loadControls();
            updatePreview();
            showMessage("Previewing "+preset.name+" · simulation only.");
            break;
        }
    });
    connect(studio,&QPushButton::clicked,this,[this]{navTo(2);});
    row->addWidget(detailPanel,3);
    l->addLayout(row,1);
    auto refresh=[this]{
        if(!m_presetsList||!m_presetCategory)return;
        m_presetsList->clear();
        const QString category=m_presetCategory->currentText();
        for(const AnimationPreset &preset:m_presets){
            if(category!="All categories"&&presetCategoryName(preset.category)!=category)continue;
            auto*item=new QListWidgetItem(
                QString("%1%2    ·    %3")
                    .arg(preset.id==m_activePresetId?QStringLiteral("▶  "):QString(),
                         preset.name,presetCategoryName(preset.category)));
            item->setData(Qt::UserRole,preset.id);
            item->setToolTip(preset.description+"\nHardware: not verified");
            QPixmap swatch(96,14);
            QPainter painter(&swatch);
            const int swatchWidth=swatch.width()/4;
            for(int index=0;index<4;++index){
                painter.fillRect(index*swatchWidth,0,
                    index==3?swatch.width()-index*swatchWidth:swatchWidth,
                    swatch.height(),preset.profile.zones[static_cast<size_t>(index)]);
            }
            item->setIcon(QIcon(swatch));
            m_presetsList->addItem(item);
        }
    };
    connect(m_presetCategory,qOverload<int>(&QComboBox::currentIndexChanged),this,[refresh]{refresh();});
    connect(m_presetsList,&QListWidget::currentItemChanged,this,[this](QListWidgetItem*current){
        if(!current)return;
        const QString id=current->data(Qt::UserRole).toString();
        for(const AnimationPreset &preset:m_presets){
            if(preset.id==id){
                m_presetDescription->setText(preset.description);
                m_presetCompatibility->setText(
                    preset.category==PresetCategory::Static
                        ? "Hardware compatibility: static transfer is user-reported working; not physically tested in this development run."
                        : "Hardware compatibility: preview-only. Native controller support is not verified.");
                break;
            }
        }
    });
    auto*manage=new QHBoxLayout;
    auto*save=button("Save current as preset");
    auto*duplicate=button("Duplicate");
    auto*rename=button("Rename");
    auto*remove=button("Delete custom");
    auto*import=button("Import");
    auto*exportPreset=button("Export");
    for(QPushButton*b:{save,duplicate,rename,remove,import,exportPreset})manage->addWidget(b);
    l->addLayout(manage);
    auto persistCustom=[this]{
        QString error;
        if(!ThemeStore::saveCustomPresets(m_customPresets,&error)){
            showMessage("Could not save custom presets: "+error,true);
            return false;
        }
        m_presets=builtInAnimationPresets();
        m_presets.append(m_customPresets);
        const int selectedCategory=m_presetCategory->currentIndex();
        m_presetCategory->setCurrentIndex(-1);
        m_presetCategory->setCurrentIndex(selectedCategory);
        return true;
    };
    auto selectedPreset=[this]() -> AnimationPreset* {
        if(!m_presetsList||!m_presetsList->currentItem())return nullptr;
        const QString id=m_presetsList->currentItem()->data(Qt::UserRole).toString();
        for(AnimationPreset &preset:m_presets)if(preset.id==id)return &preset;
        return nullptr;
    };
    connect(save,&QPushButton::clicked,this,[this,persistCustom]{
        bool ok=false;
        const QString name=QInputDialog::getText(this,"Save preset","Preset name:",
                                                  QLineEdit::Normal,m_profile.name,&ok).trimmed();
        if(!ok||name.isEmpty())return;
        AnimationPreset preset;
        preset.id=QUuid::createUuid().toString(QUuid::WithoutBraces);
        preset.name=name;
        preset.description="User-created animation preset.";
        preset.category=PresetCategory::Animated;
        preset.profile=m_profile;
        preset.profile.name=name;
        const PreviewEffect previewEffects[]={PreviewEffect::Profile,PreviewEffect::Rainbow,
            PreviewEffect::Fire,PreviewEffect::WaterRipple,PreviewEffect::ColorTransition,
            PreviewEffect::Breathing,PreviewEffect::Wave,PreviewEffect::Aurora,
            PreviewEffect::ColorPulse,PreviewEffect::AlternatingZones,PreviewEffect::CenterOut,
            PreviewEffect::PingPong,PreviewEffect::KeyGlow,PreviewEffect::HeatMap,
            PreviewEffect::OutsideIn,PreviewEffect::VerticalWave,PreviewEffect::BidirectionalWave,
            PreviewEffect::AlternatingDirection};
        preset.preview=previewEffects[qBound(0,m_previewMode->currentIndex(),17)];
        preset.custom=true;
        preset.durationSeconds=m_duration->value();
        preset.fadeInSeconds=m_fadeIn->value();
        preset.fadeOutSeconds=m_fadeOut->value();
        preset.looping=m_looping->isChecked();
        preset.reverse=m_reverse->isChecked();
        m_customPresets.append(preset);
        if(persistCustom())showMessage("Saved custom preset: "+name);
    });
    connect(duplicate,&QPushButton::clicked,this,[this,selectedPreset,persistCustom]{
        AnimationPreset *source=selectedPreset();
        if(!source)return;
        bool ok=false;
        const QString name=QInputDialog::getText(this,"Duplicate preset","New preset name:",
                                                  QLineEdit::Normal,source->name+" copy",&ok).trimmed();
        if(!ok||name.isEmpty())return;
        AnimationPreset copy=*source;
        copy.id=QUuid::createUuid().toString(QUuid::WithoutBraces);
        copy.name=name;copy.profile.name=name;copy.custom=true;
        m_customPresets.append(copy);
        if(persistCustom())showMessage("Created custom preset: "+name);
    });
    connect(rename,&QPushButton::clicked,this,[this,selectedPreset,persistCustom]{
        AnimationPreset *source=selectedPreset();
        if(!source||!source->custom){showMessage("Select a custom preset to rename.",true);return;}
        bool ok=false;
        const QString name=QInputDialog::getText(this,"Rename preset","Preset name:",
                                                  QLineEdit::Normal,source->name,&ok).trimmed();
        if(!ok||name.isEmpty())return;
        const QString id=source->id;
        for(AnimationPreset &preset:m_customPresets)if(preset.id==id){preset.name=name;preset.profile.name=name;}
        if(persistCustom())showMessage("Renamed custom preset.");
    });
    connect(remove,&QPushButton::clicked,this,[this,selectedPreset,persistCustom]{
        AnimationPreset *source=selectedPreset();
        if(!source||!source->custom){showMessage("Built-in presets cannot be deleted.",true);return;}
        const QString id=source->id;
        m_customPresets.erase(std::remove_if(m_customPresets.begin(),m_customPresets.end(),
            [&id](const AnimationPreset &preset){return preset.id==id;}),m_customPresets.end());
        if(persistCustom())showMessage("Deleted custom preset.");
    });
    connect(import,&QPushButton::clicked,this,[this,persistCustom]{
        const QString path=QFileDialog::getOpenFileName(this,"Import animation preset",{},
                                                        "LOQ animation preset (*.json)");
        if(path.isEmpty())return;
        AnimationPreset preset;QString error;
        if(!ThemeStore::readPreset(path,&preset,&error)){showMessage("Preset import failed: "+error,true);return;}
        if(std::any_of(m_customPresets.cbegin(),m_customPresets.cend(),
                       [&preset](const AnimationPreset &item){return item.id==preset.id;}))
            preset.id=QUuid::createUuid().toString(QUuid::WithoutBraces);
        preset.custom=true;m_customPresets.append(preset);
        if(persistCustom())showMessage("Imported preset: "+preset.name);
    });
    connect(exportPreset,&QPushButton::clicked,this,[this,selectedPreset]{
        AnimationPreset *preset=selectedPreset();
        if(!preset){showMessage("Select a preset to export.",true);return;}
        const QString path=QFileDialog::getSaveFileName(this,"Export animation preset",
            preset->name+".json","LOQ animation preset (*.json)");
        if(path.isEmpty())return;
        QString error;
        if(!ThemeStore::writePreset(path,*preset,&error))showMessage("Preset export failed: "+error,true);
        else showMessage("Exported preset: "+preset->name);
    });
    refresh();
    return w;
}
QWidget* MainWindow::buildSimplePresets()
{
    auto *page = makePage("Presets", "Choose a four-zone profile. Editing previews changes only; hardware updates require Apply.");
    auto *layout = qobject_cast<QVBoxLayout *>(page->layout());
    auto *toolbar = new QHBoxLayout;
    auto *create = button("Create Preset", "accent");
    auto *cycle = button("Cycle My Presets  ·  Ctrl+Alt+N");
    auto *apply = button("Apply active preset");
    auto *import = button("Import");
    auto *exportPreset = button("Export active");
    toolbar->addWidget(create);
    toolbar->addWidget(cycle);
    toolbar->addWidget(apply);
    toolbar->addStretch();
    toolbar->addWidget(import);
    toolbar->addWidget(exportPreset);
    layout->addLayout(toolbar);

    auto *content = new QHBoxLayout;
    QVBoxLayout *previewLayout = nullptr;
    auto *previewPanel = panel("KEYBOARD PREVIEW  ·  SIMULATION", &previewLayout);
    m_preview = new KeyboardPreview;
    m_preview->setMinimumHeight(300);
    previewLayout->addWidget(m_preview);
    previewLayout->addWidget(label("The preview is software-rendered. Only explicit Apply can send a profile to the keyboard.",
                                   "color:#8792a6"));
    previewLayout->addWidget(label("Static RGB is user-reported working. Wave and Breathing hardware behavior is unverified.",
                                   "color:#e6c978"));
    content->addWidget(previewPanel, 3);

    auto *library = new QWidget;
    auto *libraryLayout = new QVBoxLayout(library);
    libraryLayout->setContentsMargins(0, 0, 0, 0);
    libraryLayout->setSpacing(12);
    auto *mine = panel("MY PRESETS  ·  EDITABLE", &m_myPresetCards);
    auto *builtins = panel("BUILT-IN PRESETS  ·  READ-ONLY", &m_builtinPresetCards);
    auto *scrollContent = new QWidget;
    auto *scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setContentsMargins(0, 0, 0, 0);
    scrollLayout->setSpacing(12);
    scrollLayout->addWidget(mine);
    scrollLayout->addWidget(builtins);
    scrollLayout->addStretch();
    auto *scroll = new QScrollArea;
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setWidget(scrollContent);
    libraryLayout->addWidget(scroll);
    content->addWidget(library, 2);
    layout->addLayout(content, 1);
    connect(m_preview, &KeyboardPreview::zoneClicked, this, &MainWindow::selectZone);
    m_preview->setZoneMap(m_zoneMap);

    connect(create, &QPushButton::clicked, this, [this] {
        AnimationPreset preset;
        preset.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        preset.name = QStringLiteral("My preset");
        preset.description = QStringLiteral("User-created four-zone preset.");
        preset.profile = m_profile;
        preset.profile.effect = Effect::Static;
        preset.profile.name = preset.name;
        preset.colorMode = PresetColorMode::Same;
        editPreset(preset, true);
    });
    connect(cycle, &QPushButton::clicked, this, &MainWindow::cycleCustomPresets);
    connect(apply, &QPushButton::clicked, this, [this] {
        for (const AnimationPreset &preset : m_presets) {
            if (preset.id == m_activePresetId) {
                applyPreset(preset, true);
                return;
            }
        }
        showMessage("Select a preset before applying it.", true);
    });
    connect(import, &QPushButton::clicked, this, [this] {
        const QString path = QFileDialog::getOpenFileName(
            this, "Import preset", {}, "LOQ animation preset (*.json)");
        if (path.isEmpty()) return;
        AnimationPreset preset;
        QString error;
        if (!ThemeStore::readPreset(path, &preset, &error)) {
            showMessage("Preset import failed: " + error, true);
            return;
        }
        preset.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        preset.custom = true;
        m_customPresets.append(preset);
        if (!saveCustomPresetCollection()) {
            m_customPresets.removeLast();
            refreshSimplePresetCards();
            return;
        }
        showMessage("Imported preset: " + preset.name);
    });
    connect(exportPreset, &QPushButton::clicked, this, [this] {
        const auto it = std::find_if(m_presets.cbegin(), m_presets.cend(),
            [this](const AnimationPreset &preset) { return preset.id == m_activePresetId; });
        if (it == m_presets.cend()) {
            showMessage("Select a preset before exporting it.", true);
            return;
        }
        const QString path = QFileDialog::getSaveFileName(
            this, "Export preset", it->name + ".json", "LOQ animation preset (*.json)");
        if (path.isEmpty()) return;
        QString error;
        if (!ThemeStore::writePreset(path, *it, &error))
            showMessage("Preset export failed: " + error, true);
        else
            showMessage("Exported preset: " + it->name);
    });
    refreshSimplePresetCards();
    return page;
}

void MainWindow::refreshSimplePresetCards()
{
    const auto clear = [](QVBoxLayout *layout) {
        while (QLayoutItem *item = layout->takeAt(0)) {
            if (QWidget *widget = item->widget()) {
                widget->hide();
                widget->deleteLater();
            }
            delete item;
        }
    };
    if (!m_myPresetCards || !m_builtinPresetCards) return;
    clear(m_myPresetCards);
    clear(m_builtinPresetCards);

    const auto makeCard = [this](const AnimationPreset &preset, bool builtIn) {
        auto *card = new QFrame;
        card->setProperty("panel", true);
        auto *layout = new QVBoxLayout(card);
        layout->setContentsMargins(14, 12, 14, 12);
        layout->setSpacing(8);
        auto *heading = new QHBoxLayout;
        auto *name = label((preset.id == m_activePresetId ? "●  " : "") + preset.name,
                           "font-weight:700;color:#edf4ff");
        heading->addWidget(name, 1);
        if (preset.id == m_activePresetId)
            heading->addWidget(label("ACTIVE", "color:#18d9ff;font-size:10px;font-weight:700"));
        layout->addLayout(heading);
        layout->addWidget(label(QString("%1  ·  %2% brightness")
                                    .arg(effectName(preset.profile.effect))
                                    .arg(preset.profile.brightness),
                                "color:#9aa6ba;font-size:12px"));
        auto *swatches = new QHBoxLayout;
        for (const QColor &color : preset.profile.zones) {
            auto *swatch = new QFrame;
            swatch->setFixedHeight(12);
            swatch->setStyleSheet(QString("background:%1;border-radius:4px").arg(color.name()));
            swatches->addWidget(swatch, 1);
        }
        layout->addLayout(swatches);
        auto *actions = new QHBoxLayout;
        auto *secondaryActions = new QHBoxLayout;
        auto *apply = button("Apply", "accent");
        actions->addWidget(apply);
        connect(apply, &QPushButton::clicked, this,
                [this, preset] { applyPreset(preset, true); });
        if (builtIn) {
            auto *copy = button("Customize copy");
            actions->addWidget(copy);
            connect(copy, &QPushButton::clicked, this, [this, preset] {
                AnimationPreset copy = preset;
                copy.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
                copy.name += QStringLiteral(" Copy");
                copy.profile.name = copy.name;
                copy.custom = true;
                editPreset(copy, true);
            });
        } else {
            auto *edit = button("Edit");
            auto *rename = button("Rename");
            auto *duplicate = button("Duplicate");
            auto *remove = button("Delete");
            actions->addWidget(edit);
            actions->addWidget(rename);
            secondaryActions->addWidget(duplicate);
            secondaryActions->addWidget(remove);
            connect(edit, &QPushButton::clicked, this,
                    [this, preset] { editPreset(preset, false); });
            connect(rename, &QPushButton::clicked, this, [this, preset] {
                bool ok = false;
                const QString name = QInputDialog::getText(
                    this, "Rename preset", "Preset name:", QLineEdit::Normal,
                    preset.name, &ok).trimmed();
                if (!ok) return;
                const auto previous = m_customPresets;
                QString error;
                if (!renameCustomPreset(m_customPresets, preset.id, name, &error)) {
                    showMessage(error, true);
                    return;
                }
                if (!saveCustomPresetCollection()) {
                    m_customPresets = previous;
                    refreshSimplePresetCards();
                }
                else showMessage("Renamed preset.");
            });
            connect(duplicate, &QPushButton::clicked, this, [this, preset] {
                bool ok = false;
                const QString name = QInputDialog::getText(
                    this, "Duplicate preset", "New preset name:", QLineEdit::Normal,
                    preset.name + " Copy", &ok).trimmed();
                if (!ok || name.isEmpty()) return;
                const auto previous = m_customPresets;
                QString error;
                if (!duplicatePreset(m_presets, &m_customPresets, preset.id,
                                     QUuid::createUuid().toString(QUuid::WithoutBraces),
                                     name, &error)) {
                    showMessage(error, true);
                    return;
                }
                if (!saveCustomPresetCollection()) {
                    m_customPresets = previous;
                    refreshSimplePresetCards();
                }
                else showMessage("Duplicated preset.");
            });
            connect(remove, &QPushButton::clicked, this, [this, preset] {
                if (QMessageBox::question(this, "Delete preset",
                        QString("Delete “%1”? This cannot be undone.").arg(preset.name),
                        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel)
                    != QMessageBox::Yes)
                    return;
                const auto previous = m_customPresets;
                QString error;
                if (!deleteCustomPreset(m_customPresets, preset.id, &error)) {
                    showMessage(error, true);
                    return;
                }
                const QString previousActiveId = m_activePresetId;
                if (m_activePresetId == preset.id) m_activePresetId.clear();
                if (!saveCustomPresetCollection()) {
                    m_customPresets = previous;
                    m_activePresetId = previousActiveId;
                    refreshSimplePresetCards();
                }
                else showMessage("Deleted preset.");
            });
        }
        layout->addLayout(actions);
        if (secondaryActions->count() > 0) layout->addLayout(secondaryActions);
        return card;
    };

    if (m_customPresets.isEmpty()) {
        m_myPresetCards->addWidget(label("No saved presets yet. Create one or customize a built-in.",
                                         "color:#8792a6"));
    } else {
        for (const AnimationPreset &preset : m_customPresets)
            m_myPresetCards->addWidget(makeCard(preset, false));
    }
    for (const AnimationPreset &preset : builtInAnimationPresets())
        m_builtinPresetCards->addWidget(makeCard(preset, true));
}

bool MainWindow::saveCustomPresetCollection()
{
    QString error;
    if (!ThemeStore::saveCustomPresets(m_customPresets, &error)) {
        showMessage("Could not save custom presets: " + error, true);
        return false;
    }
    m_presets = builtInAnimationPresets();
    m_presets.append(m_customPresets);
    refreshSimplePresetCards();
    return true;
}

void MainWindow::applyPreset(const AnimationPreset &preset, bool sendToDevice)
{
    m_profile = preset.profile;
    m_activeColorMode = preset.colorMode;
    m_activePresetId = preset.id;
    m_engine.setProfile(m_profile);
    m_engine.setPresetColorMode(m_activeColorMode);
    m_engine.setPreviewEffect(PreviewEffect::Profile);
    updatePreview();
    refreshSimplePresetCards();
    showMessage(QString("Selected %1 · preview only until Apply.").arg(preset.name));
    if (sendToDevice) applyProfile();
}

void MainWindow::editPreset(const AnimationPreset &source, bool creating)
{
    const Profile previousProfile = m_profile;
    const PresetColorMode previousMode = m_activeColorMode;
    const QString previousActiveId = m_activePresetId;
    AnimationPreset draft = source;
    if (draft.profile.effect == Effect::Hue) {
        draft.profile.effect = Effect::Wave;
        draft.colorMode = PresetColorMode::Multi;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(creating ? "Create preset" : "Edit preset");
    dialog.setMinimumWidth(560);
    auto *layout = new QVBoxLayout(&dialog);
    auto *form = new QFormLayout;
    auto *name = new QLineEdit(draft.name);
    auto *effect = new QComboBox;
    effect->addItem("Static", static_cast<int>(Effect::Static));
    effect->addItem("Wave", static_cast<int>(Effect::Wave));
    effect->addItem("Breathing", static_cast<int>(Effect::Breathing));
    const int effectIndex = effect->findData(static_cast<int>(draft.profile.effect));
    effect->setCurrentIndex(qMax(0, effectIndex));
    const PresetColorMode requestedMode = draft.colorMode;
    auto *colorMode = new QComboBox;
    auto *direction = new QComboBox;
    direction->addItems({"Left to Right", "Right to Left"});
    direction->setCurrentIndex(draft.profile.direction == Direction::LeftToRight ? 0 : 1);
    auto *brightness = new QSlider(Qt::Horizontal);
    brightness->setRange(1, 100);
    brightness->setValue(qBound(1, draft.profile.brightness, 100));
    auto *speed = new QSlider(Qt::Horizontal);
    speed->setRange(1, 100);
    speed->setValue(qBound(1, draft.profile.speed, 100));
    form->addRow("Preset name", name);
    form->addRow("Effect", effect);
    form->addRow("Colour mode", colorMode);
    form->addRow("Direction", direction);
    form->addRow("Speed", speed);
    form->addRow("Brightness", brightness);
    layout->addLayout(form);

    QWidget *zoneRows[4]{};
    QLabel *zoneLabels[4]{};
    QLineEdit *hexInputs[4]{};
    QPushButton *colorButtons[4]{};
    for (int zone = 0; zone < 4; ++zone) {
        zoneRows[zone] = new QWidget(&dialog);
        auto *row = new QHBoxLayout(zoneRows[zone]);
        row->setContentsMargins(0, 0, 0, 0);
        zoneLabels[zone] = label(QString("Zone %1").arg(zone + 1));
        colorButtons[zone] = button("Choose");
        hexInputs[zone] = new QLineEdit(draft.profile.zones[static_cast<size_t>(zone)]
                                            .name(QColor::HexRgb).toUpper());
        hexInputs[zone]->setMaxLength(7);
        hexInputs[zone]->setPlaceholderText("#18D9FF");
        row->addWidget(zoneLabels[zone]);
        row->addStretch();
        row->addWidget(colorButtons[zone]);
        row->addWidget(hexInputs[zone]);
        layout->addWidget(zoneRows[zone]);
        connect(colorButtons[zone], &QPushButton::clicked, &dialog,
                [&, zone] {
            const QColor current(hexInputs[zone]->text());
            const QColor selected = QColorDialog::getColor(
                current.isValid() ? current : QColor("#18d9ff"), &dialog,
                QString("Zone %1 colour").arg(zone + 1));
            if (selected.isValid()) {
                hexInputs[zone]->setText(selected.name(QColor::HexRgb).toUpper());
                hexInputs[zone]->setStyleSheet(QString("border-left:5px solid %1")
                                                   .arg(selected.name()));
            }
        });
        connect(hexInputs[zone], &QLineEdit::editingFinished, &dialog,
                [this, hex = hexInputs[zone]] {
            const QColor color(hex->text().trimmed());
            if (!color.isValid() || hex->text().trimmed().size() != 7) {
                showMessage("Enter a HEX colour such as #18D9FF.", true);
                return;
            }
            hex->setStyleSheet(QString("border-left:5px solid %1").arg(color.name()));
        });
    }
    auto *modeDescription = label({}, "color:#8792a6");
    layout->addWidget(modeDescription);
    auto configureModes = [=] {
        const int selectedEffect = effect->currentData().toInt();
        const PresetColorMode priorMode = colorMode->count() == 0
            ? requestedMode
            : static_cast<PresetColorMode>(colorMode->currentData().toInt());
        colorMode->clear();
        if (selectedEffect == static_cast<int>(Effect::Static)) {
            colorMode->addItem("Same colour for all zones", static_cast<int>(PresetColorMode::Same));
            colorMode->addItem("Individual zone colours", static_cast<int>(PresetColorMode::Individual));
        } else if (selectedEffect == static_cast<int>(Effect::Wave)) {
            colorMode->addItem("Multi-colour wave", static_cast<int>(PresetColorMode::Multi));
            colorMode->addItem("Fixed section colours", static_cast<int>(PresetColorMode::FixedSections));
        } else {
            colorMode->addItem("Single colour", static_cast<int>(PresetColorMode::Single));
            colorMode->addItem("Multi-colour", static_cast<int>(PresetColorMode::Multi));
            colorMode->addItem("Individual zone colours", static_cast<int>(PresetColorMode::Individual));
        }
        int index = colorMode->findData(static_cast<int>(priorMode));
        if (index < 0) index = 0;
        colorMode->setCurrentIndex(index);
    };
    configureModes();
    auto updateVisibility = [=] {
        const Effect selectedEffect = static_cast<Effect>(effect->currentData().toInt());
        const PresetColorMode mode = static_cast<PresetColorMode>(colorMode->currentData().toInt());
        const bool same = mode == PresetColorMode::Same || mode == PresetColorMode::Single;
        const bool zones = mode == PresetColorMode::Individual ||
                           mode == PresetColorMode::FixedSections;
        for (int zone = 0; zone < 4; ++zone) {
            zoneRows[zone]->setVisible(zones || (same && zone == 0));
        }
        zoneLabels[0]->setText(mode == PresetColorMode::Same ? "All zones"
                                    : mode == PresetColorMode::Single ? "Single colour"
                                    : "Zone 1");
        direction->setVisible(selectedEffect == Effect::Wave);
        speed->setVisible(selectedEffect != Effect::Static);
        form->labelForField(direction)->setVisible(selectedEffect == Effect::Wave);
        form->labelForField(speed)->setVisible(selectedEffect != Effect::Static);
        modeDescription->setText(mode == PresetColorMode::Multi
            ? "Multi-colour effects are simulated across the four-zone preview; physical support is unverified."
            : "Only four zone colours are represented. Preview animation is software-rendered.");
        if (same) {
            for (int zone = 1; zone < 4; ++zone)
                hexInputs[zone]->setText(hexInputs[0]->text());
        }
    };
    connect(effect, qOverload<int>(&QComboBox::currentIndexChanged), &dialog, [=] {
        configureModes();
        updateVisibility();
    });
    connect(colorMode, qOverload<int>(&QComboBox::currentIndexChanged), &dialog, updateVisibility);
    updateVisibility();

    auto syncPreview = [this, &draft, name, effect, colorMode, direction, brightness, speed,
                        hexInputs] {
        draft.name = name->text().trimmed();
        draft.profile.name = draft.name.isEmpty() ? QStringLiteral("Preview") : draft.name;
        draft.profile.effect = static_cast<Effect>(effect->currentData().toInt());
        draft.profile.direction = direction->currentIndex() == 0
            ? Direction::LeftToRight : Direction::RightToLeft;
        draft.profile.brightness = brightness->value();
        draft.profile.speed = speed->value();
        draft.colorMode = static_cast<PresetColorMode>(colorMode->currentData().toInt());
        for (int zone = 0; zone < 4; ++zone) {
            const QColor color(hexInputs[zone]->text().trimmed());
            if (color.isValid() && hexInputs[zone]->text().trimmed().size() == 7)
                draft.profile.zones[static_cast<size_t>(zone)] = color;
        }
        applyPresetColorMode(&draft, draft.colorMode);
        m_profile = draft.profile;
        m_activeColorMode = draft.colorMode;
        m_engine.setProfile(m_profile);
        m_engine.setPresetColorMode(m_activeColorMode);
        m_engine.setPreviewEffect(PreviewEffect::Profile);
        updatePreview();
    };
    const QList<QObject *> controls{name, effect, colorMode, direction, brightness, speed};
    for (QObject *control : controls) {
        if (auto *combo = qobject_cast<QComboBox *>(control))
            connect(combo, qOverload<int>(&QComboBox::currentIndexChanged), &dialog,
                    [syncPreview] { syncPreview(); });
        else if (auto *slider = qobject_cast<QSlider *>(control))
            connect(slider, &QSlider::valueChanged, &dialog, [syncPreview] { syncPreview(); });
        else if (auto *line = qobject_cast<QLineEdit *>(control))
            connect(line, &QLineEdit::textChanged, &dialog, [syncPreview] { syncPreview(); });
    }
    for (QLineEdit *input : hexInputs)
        connect(input, &QLineEdit::editingFinished, &dialog, [syncPreview] { syncPreview(); });
    syncPreview();

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        if (name->text().trimmed().isEmpty()) {
            showMessage("Preset name cannot be empty.", true);
            return;
        }
        const PresetColorMode mode = static_cast<PresetColorMode>(colorMode->currentData().toInt());
        const int count = mode == PresetColorMode::Individual ||
                          mode == PresetColorMode::FixedSections ? 4 : 1;
        for (int zone = 0; zone < count; ++zone) {
            const QString text = hexInputs[zone]->text().trimmed();
            if (!QColor(text).isValid() || text.size() != 7) {
                showMessage(QString("Enter a valid HEX colour for zone %1.").arg(zone + 1), true);
                return;
            }
        }
        syncPreview();
        dialog.accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted) {
        m_profile = previousProfile;
        m_activeColorMode = previousMode;
        m_activePresetId = previousActiveId;
        m_engine.setProfile(m_profile);
        m_engine.setPresetColorMode(m_activeColorMode);
        updatePreview();
        return;
    }

    draft.id = creating ? QUuid::createUuid().toString(QUuid::WithoutBraces) : source.id;
    draft.name = name->text().trimmed();
    draft.description = "User-created four-zone preset.";
    draft.profile.name = draft.name;
    draft.category = draft.profile.effect == Effect::Static
        ? PresetCategory::Static : PresetCategory::Animated;
    draft.preview = PreviewEffect::Profile;
    draft.custom = true;
    const auto previous = m_customPresets;
    if (creating) {
        m_customPresets.append(draft);
    } else {
        auto it = std::find_if(m_customPresets.begin(), m_customPresets.end(),
            [&source](const AnimationPreset &preset) { return preset.id == source.id; });
        if (it == m_customPresets.end()) {
            showMessage("Only a custom preset can be edited.", true);
            m_profile = previousProfile;
            m_activeColorMode = previousMode;
            m_activePresetId = previousActiveId;
            m_engine.setProfile(m_profile);
            m_engine.setPresetColorMode(m_activeColorMode);
            updatePreview();
            return;
        }
        *it = draft;
    }
    if (!saveCustomPresetCollection()) {
        m_customPresets = previous;
        refreshSimplePresetCards();
        m_profile = previousProfile;
        m_activeColorMode = previousMode;
        m_activePresetId = previousActiveId;
        m_engine.setProfile(m_profile);
        m_engine.setPresetColorMode(m_activeColorMode);
        updatePreview();
        return;
    }
    m_activePresetId = draft.id;
    applyPreset(draft, false);
    showMessage(QString("Saved preset %1 · preview only.").arg(draft.name));
}

void MainWindow::cycleCustomPresets()
{
    const int index = nextCustomPresetIndex(m_presets, m_activePresetId);
    if (index < 0) {
        showMessage("Create a preset in My Presets before cycling.", true);
        return;
    }
    applyPreset(m_presets[index], true);
}

QWidget* MainWindow::buildStudio(){auto*w=makePage("Animation studio","Shape the lighting across four independent hardware zones.");auto*l=qobject_cast<QVBoxLayout*>(w->layout());auto*row=new QHBoxLayout;QVBoxLayout*leftLayout;auto*left=panel("KEYBOARD PREVIEW",&leftLayout);m_studioPreview=new KeyboardPreview;leftLayout->addWidget(m_studioPreview);connect(m_studioPreview,&KeyboardPreview::zoneClicked,this,&MainWindow::selectZone);row->addWidget(left,3);
    QVBoxLayout*controls;auto*right=panel("EFFECT CONTROLS",&controls);auto*form=new QFormLayout;form->setSpacing(13);m_effect=new QComboBox;m_effect->addItems({"Static","Breathing","Wave","Hue cycling"});m_direction=new QComboBox;m_direction->addItems({"Left to right","Right to left"});m_brightness=new QSlider(Qt::Horizontal);m_brightness->setRange(0,100);m_speed=new QSlider(Qt::Horizontal);m_speed->setRange(1,100);form->addRow("Effect",m_effect);form->addRow("Direction",m_direction);form->addRow("Brightness",m_brightness);form->addRow("Speed",m_speed);controls->addLayout(form);controls->addWidget(label("SOFTWARE PREVIEW ANIMATION","font-size:11px;color:#8c98ad;font-weight:700"));m_previewMode=new QComboBox;m_previewMode->addItems({"Profile effect","Rainbow","Fire","Ripple","Colour transitions"});controls->addWidget(m_previewMode);connect(m_previewMode,qOverload<int>(&QComboBox::currentIndexChanged),this,[this](int i){const PreviewEffect e[]={PreviewEffect::Profile,PreviewEffect::Rainbow,PreviewEffect::Fire,PreviewEffect::Ripple,PreviewEffect::ColorTransition};m_engine.setPreviewEffect(e[qBound(0,i,4)]);});controls->addWidget(label("ZONE COLOUR","font-size:11px;color:#8c98ad;font-weight:700"));auto*zrow=new QHBoxLayout;m_zoneColor=button("Zone 1 colour");connect(m_zoneColor,&QPushButton::clicked,this,&MainWindow::chooseColor);zrow->addWidget(m_zoneColor);m_hex=new QLineEdit;m_hex->setPlaceholderText("#18D9FF");m_hex->setMaxLength(7);zrow->addWidget(m_hex);m_rgb=new QLineEdit;m_rgb->setPlaceholderText("24, 217, 255");zrow->addWidget(m_rgb);controls->addLayout(zrow);auto*apply=button("Apply simulation profile");m_applyButtons.append(apply);connect(apply,&QPushButton::clicked,this,&MainWindow::applyProfile);controls->addWidget(apply);row->addWidget(right,2);l->addLayout(row,1);
    auto*pres=panel("QUICK COLOUR PRESETS");auto*pb=qobject_cast<QVBoxLayout*>(pres->layout());auto*buttons=new QHBoxLayout;for(const QColor&c:{QColor("#18d9ff"),QColor("#ff3d88"),QColor("#a43cff"),QColor("#ffb02e"),QColor("#47e889"),QColor("#eafaff")}){auto*b=button("●");b->setStyleSheet(QString("color:%1;font-size:20px").arg(c.name()));connect(b,&QPushButton::clicked,this,[this,c]{setZoneColor(m_selectedZone,c);});buttons->addWidget(b);}buttons->addStretch();pb->addLayout(buttons);l->addWidget(pres);connect(m_effect,qOverload<int>(&QComboBox::currentIndexChanged),this,[this]{collectControls();m_engine.setProfile(m_profile);});connect(m_direction,qOverload<int>(&QComboBox::currentIndexChanged),this,[this]{collectControls();m_engine.setProfile(m_profile);});connect(m_brightness,&QSlider::valueChanged,this,[this]{collectControls();m_engine.setProfile(m_profile);});connect(m_speed,&QSlider::valueChanged,this,[this]{collectControls();m_engine.setProfile(m_profile);});connect(m_hex,&QLineEdit::editingFinished,this,[this]{QColor c(m_hex->text().trimmed());if(c.isValid()){setZoneColor(m_selectedZone,c);}else showMessage("Enter a HEX colour such as #18D9FF.",true);});connect(m_rgb,&QLineEdit::editingFinished,this,[this]{const auto parts=m_rgb->text().split(',');bool ok=true;int v[3]={};if(parts.size()!=3)ok=false;else for(int i=0;i<3;++i){v[i]=parts[i].trimmed().toInt(&ok);if(!ok||v[i]<0||v[i]>255){ok=false;break;}}if(ok){setZoneColor(m_selectedZone,QColor(v[0],v[1],v[2]));}else showMessage("RGB format: 24, 217, 255 (each value 0–255).",true);});loadControls();return w;}
QWidget* MainWindow::buildReactive(){
    auto*w=makePage("Reactive effects","Key-level reactions are software preview only; physical lighting has four zones.");
    auto*l=qobject_cast<QVBoxLayout*>(w->layout());
    QVBoxLayout*previewLayout;
    auto*previewPanel=panel("CLICK A KEY TO DEMO",&previewLayout);
    m_reactivePreview=new KeyboardPreview;
    previewLayout->addWidget(m_reactivePreview);
    l->addWidget(previewPanel,1);

    QVBoxLayout*controls;
    auto*controlPanel=panel("REACTION CONTROLS",&controls);
    auto*form=new QFormLayout;
    m_reactiveMode=new QComboBox;
    m_reactiveMode->addItems({"Water ripple","Key press glow","Heat map"});
    form->addRow("Effect",m_reactiveMode);
    m_reactiveDecay=new QSlider(Qt::Horizontal);
    m_reactiveDecay->setRange(1,150);
    m_reactiveDecay->setValue(20);
    form->addRow("Decay (0.1–15 seconds)",m_reactiveDecay);
    m_rippleSpeed=new QSlider(Qt::Horizontal);m_rippleSpeed->setRange(2,200);m_rippleSpeed->setValue(40);
    m_rippleRadius=new QSlider(Qt::Horizontal);m_rippleRadius->setRange(2,24);m_rippleRadius->setValue(12);
    m_waveCount=new QSlider(Qt::Horizontal);m_waveCount->setRange(1,5);m_waveCount->setValue(1);
    m_reactiveIntensity=new QSlider(Qt::Horizontal);m_reactiveIntensity->setRange(0,100);m_reactiveIntensity->setValue(100);
    form->addRow("Ripple speed",m_rippleSpeed);
    form->addRow("Maximum radius",m_rippleRadius);
    form->addRow("Wave count",m_waveCount);
    form->addRow("Intensity",m_reactiveIntensity);
    controls->addLayout(form);
    auto*colorButton=button("Choose reactive colour");
    controls->addWidget(colorButton);
    auto*clear=button("Clear activity");
    controls->addWidget(clear);
    m_reactiveStatus=label("Manual preview demo active. No typed text is stored.","color:#61e6ac");
    m_reactiveStatus->setWordWrap(true);
    controls->addWidget(m_reactiveStatus);

    m_inputDevice=new QComboBox;
    auto*refresh=button("Refresh keyboard devices");
    m_inputEnabled=new QCheckBox("Enable global key preview (keycodes only)");
    controls->addWidget(m_inputDevice);
    controls->addWidget(refresh);
    controls->addWidget(m_inputEnabled);
    controls->addWidget(label("Linux evdev reads only key-down codes and timestamps from the selected, user-accessible event node. It does not grab input, record text, elevate privileges, or change permissions. If access is denied, use the clickable preview.","color:#a6b1c4"));
    l->addWidget(controlPanel);

    connect(m_reactiveMode,qOverload<int>(&QComboBox::currentIndexChanged),this,[this](int index){
        const PreviewEffect effects[]={PreviewEffect::WaterRipple,PreviewEffect::KeyGlow,PreviewEffect::HeatMap};
        m_engine.setPreviewEffect(effects[qBound(0,index,2)]);
    });
    connect(m_reactiveDecay,&QSlider::valueChanged,this,[this](int value){
        m_engine.setReactiveDecay(value/10.0);
    });
    auto updateRipple=[this]{
        m_engine.setRippleSettings(m_rippleSpeed->value()/10.0,m_rippleRadius->value(),
                                   m_waveCount->value(),m_reactiveIntensity->value()/100.0);
    };
    connect(m_rippleSpeed,&QSlider::valueChanged,this,[updateRipple]{updateRipple();});
    connect(m_rippleRadius,&QSlider::valueChanged,this,[updateRipple]{updateRipple();});
    connect(m_waveCount,&QSlider::valueChanged,this,[updateRipple]{updateRipple();});
    connect(m_reactiveIntensity,&QSlider::valueChanged,this,[updateRipple]{updateRipple();});
    connect(colorButton,&QPushButton::clicked,this,[this]{
        const QColor color=QColorDialog::getColor(QColor("#18d9ff"),this,"Reactive preview colour");
        if(color.isValid())m_engine.setReactiveColor(color);
    });
    connect(clear,&QPushButton::clicked,this,[this]{m_engine.clearReactiveState();});
    connect(refresh,&QPushButton::clicked,this,&MainWindow::refreshInputDevices);
    connect(m_inputEnabled,&QCheckBox::toggled,this,&MainWindow::toggleInputListener);
    m_engine.setReactiveDecay(m_reactiveDecay->value()/10.0);
    updateRipple();
    refreshInputDevices();
    return w;
}

QWidget* MainWindow::buildColorEditor(){
    auto*w=makePage("Four-zone colour editor","Edit colours independently; the on-screen key-to-zone map is an estimate.");
    auto*l=qobject_cast<QVBoxLayout*>(w->layout());
    QVBoxLayout*previewLayout;
    auto*previewPanel=panel("ZONE PREVIEW",&previewLayout);
    m_colorPreview=new KeyboardPreview;
    previewLayout->addWidget(m_colorPreview);
    l->addWidget(previewPanel,1);
    QVBoxLayout*editor;
    auto*editorPanel=panel("COLOUR AND CALIBRATION",&editor);
    auto*zoneButtons=new QHBoxLayout;
    for(int zone=0;zone<4;++zone){
        auto*select=button(QString("Zone %1 colour").arg(zone+1));
        connect(select,&QPushButton::clicked,this,[this,zone]{selectZone(zone);chooseColor();});
        zoneButtons->addWidget(select);
    }
    editor->addLayout(zoneButtons);
    m_calibrationZone=new QComboBox;
    for(int zone=0;zone<4;++zone)m_calibrationZone->addItem(KeyboardZoneMap::zoneName(zone),zone);
    m_zoneCalibration=new QCheckBox("Calibrate preview mapping: clicking a key assigns it to the selected zone");
    editor->addWidget(m_calibrationZone);
    editor->addWidget(m_zoneCalibration);
    editor->addWidget(label("Calibration changes only the software preview map and is not saved or treated as hardware evidence. Click a physical key in the preview and compare zones with a trusted Lenovo control-panel view before relying on the map.","color:#e6c978"));

    auto*gradientPanel=panel("CUSTOM GRADIENT");
    auto*gradientLayout=qobject_cast<QVBoxLayout*>(gradientPanel->layout());
    auto*stopRow=new QHBoxLayout;
    m_gradientStart=button("Start stop");
    m_gradientMiddle=button("Middle stop");
    m_gradientEnd=button("End stop");
    for(QPushButton*stop:{m_gradientStart,m_gradientMiddle,m_gradientEnd})stopRow->addWidget(stop);
    gradientLayout->addLayout(stopRow);
    m_gradientMidpoint=new QSlider(Qt::Horizontal);
    m_gradientMidpoint->setRange(5,95);
    m_gradientMidpoint->setValue(50);
    gradientLayout->addWidget(label("Middle stop position"));
    gradientLayout->addWidget(m_gradientMidpoint);
    m_gradientDirection=new QComboBox;
    m_gradientDirection->addItems({"Left to right","Right to left"});
    m_paletteMode=new QComboBox;
    m_paletteMode->addItems({"Custom gradient palette","Rainbow spectrum"});
    m_rainbowSaturation=new QSlider(Qt::Horizontal);m_rainbowSaturation->setRange(0,100);m_rainbowSaturation->setValue(94);
    m_breathingDepth=new QSlider(Qt::Horizontal);m_breathingDepth->setRange(0,100);m_breathingDepth->setValue(84);
    gradientLayout->addWidget(label("Gradient direction"));
    gradientLayout->addWidget(m_gradientDirection);
    gradientLayout->addWidget(label("Rainbow palette"));
    gradientLayout->addWidget(m_paletteMode);
    gradientLayout->addWidget(label("Saturation"));
    gradientLayout->addWidget(m_rainbowSaturation);
    gradientLayout->addWidget(label("Breathing depth"));
    gradientLayout->addWidget(m_breathingDepth);
    auto*gradientHelp=label("Three editable stops · linear RGB interpolation · left-to-right zone order. Used by software Aurora and heat-map previews.","color:#8c98ad");
    gradientHelp->setWordWrap(true);
    gradientLayout->addWidget(gradientHelp);
    editor->addWidget(gradientPanel);
    l->addWidget(editorPanel);

    const QList<QPushButton*> stopButtons{m_gradientStart,m_gradientMiddle,m_gradientEnd};
    for(int index=0;index<stopButtons.size();++index){
        connect(stopButtons[index],&QPushButton::clicked,this,[this,index,stopButtons]{
            QColor current=m_gradientStops[index].color;
            const QColor selected=QColorDialog::getColor(current,this,"Choose gradient stop");
            if(!selected.isValid())return;
            m_gradientStops[index].color=selected;
            stopButtons[index]->setStyleSheet(QString("background:%1").arg(selected.name()));
            QVector<ColorStop> stops=m_gradientStops;
            if(m_gradientDirection->currentIndex()==1){
                std::reverse(stops.begin(),stops.end());
                for(ColorStop &stop:stops)stop.position=1.0-stop.position;
            }
            m_engine.setPalette(ColorGradient(stops));
        });
        stopButtons[index]->setStyleSheet(QString("background:%1").arg(m_gradientStops[index].color.name()));
    }
    connect(m_gradientMidpoint,&QSlider::valueChanged,this,[this](int position){
        m_gradientStops[1].position=position/100.0;
        QVector<ColorStop> stops=m_gradientStops;
        if(m_gradientDirection->currentIndex()==1){
            std::reverse(stops.begin(),stops.end());
            for(ColorStop &stop:stops)stop.position=1.0-stop.position;
        }
        m_engine.setPalette(ColorGradient(stops));
    });
    auto applyPalette=[this]{
        QVector<ColorStop> stops=m_gradientStops;
        if(m_gradientDirection->currentIndex()==1){
            std::reverse(stops.begin(),stops.end());
            for(ColorStop &stop:stops)stop.position=1.0-stop.position;
        }
        m_engine.setPalette(ColorGradient(stops));
    };
    connect(m_gradientDirection,qOverload<int>(&QComboBox::currentIndexChanged),this,[applyPalette]{applyPalette();});
    connect(m_paletteMode,qOverload<int>(&QComboBox::currentIndexChanged),this,[this](int index){
        m_engine.setCustomRainbowPalette(index==0);
    });
    connect(m_rainbowSaturation,&QSlider::valueChanged,this,[this](int value){m_engine.setRainbowSaturation(value/100.0);});
    connect(m_breathingDepth,&QSlider::valueChanged,this,[this](int value){m_engine.setBreathingDepth(value/100.0);});
    applyPalette();
    m_engine.setCustomRainbowPalette(false);
    m_engine.setRainbowSaturation(m_rainbowSaturation->value()/100.0);
    m_engine.setBreathingDepth(m_breathingDepth->value()/100.0);
    return w;
}

void MainWindow::refreshInputDevices(){
    if(!m_inputDevice)return;
    if(m_inputListener&&m_inputListener->isRunning())m_inputListener->stop();
    m_inputDevice->clear();
    const auto devices=KeyboardInputListener::availableKeyboards();
    for(const auto &device:devices)m_inputDevice->addItem(device.name,device.path);
    if(devices.isEmpty()){
        m_inputDevice->addItem("No keyboard event devices found");
        if(m_reactiveStatus)m_reactiveStatus->setText("No keyboard event devices found. Click keys in the preview for manual demo.");
    }else if(m_reactiveStatus){
        m_reactiveStatus->setText("Select a keyboard to enable opt-in keycode-only events. Manual preview remains available.");
    }
}

void MainWindow::toggleInputListener(bool enabled){
    if(!m_inputListener||!m_inputDevice)return;
    if(!enabled){
        m_inputListener->stop();
        if(m_reactiveStatus)m_reactiveStatus->setText("Global input disabled. Manual preview demo remains available.");
        return;
    }
    const QString path=m_inputDevice->currentData().toString();
    QString error;
    if(path.isEmpty()||!m_inputListener->startDevice(path,&error)){
        if(m_reactiveStatus)m_reactiveStatus->setText(error.isEmpty()?"No keyboard event node is selected.":error);
        m_inputEnabled->blockSignals(true);m_inputEnabled->setChecked(false);m_inputEnabled->blockSignals(false);
    }else if(m_reactiveStatus){
        m_reactiveStatus->setText("Listening to key-down codes only; no text is collected or stored.");
    }
}

void MainWindow::handlePreviewKey(int keyId,const QPointF &position,int zone){
    if(m_zoneCalibration&&m_zoneCalibration->isChecked()&&m_calibrationZone){
        const int target=m_calibrationZone->currentData().toInt();
        m_zoneMap.setKeyZone(keyId,target);
        for(KeyboardPreview*preview:{m_preview,m_studioPreview,m_reactivePreview,m_colorPreview}){
            if(preview)preview->setZoneMap(m_zoneMap);
        }
        zone=target;
        if(m_reactiveStatus)m_reactiveStatus->setText(
            QString("Preview calibration: %1 assigned to %2. This mapping is not saved.")
                .arg(m_colorPreview&&m_colorPreview->layout().findKey(keyId)
                         ?m_colorPreview->layout().findKey(keyId)->label:QString::number(keyId),
                     KeyboardZoneMap::zoneName(target)));
    }
    m_engine.registerKeyPress(keyId,position);
    selectZone(qBound(0,zone,3));
}

void MainWindow::updateAnimationControls(){
    if(!m_animationProgress)return;
    const qreal duration=m_engine.duration();
    if(duration<=0.0){m_animationProgress->setText("Looping preview · phase "+QString::number(m_engine.phase(),'f',2));return;}
    const int percent=qBound(0,static_cast<int>(100.0*m_engine.elapsed()/duration),100);
    m_animationProgress->setText(QString("Timeline %1% · %2 / %3 s%4")
        .arg(percent).arg(m_engine.elapsed(),0,'f',1).arg(duration,0,'f',0)
        .arg(m_engine.isPaused()?" · paused/complete":""));
}
QWidget* MainWindow::buildThemes(){auto*w=makePage("Themes","Curated colour stories and your own saved profiles.");auto*l=qobject_cast<QVBoxLayout*>(w->layout());auto*row=new QHBoxLayout;auto*listPanel=panel("LIBRARY");auto*lp=qobject_cast<QVBoxLayout*>(listPanel->layout());m_themeList=new QListWidget;lp->addWidget(m_themeList);connect(m_themeList,&QListWidget::itemClicked,this,&MainWindow::themeSelected);row->addWidget(listPanel,2);QVBoxLayout*pvLayout;auto*preview=panel("SELECTED PROFILE",&pvLayout);pvLayout->addWidget(label("Built-in themes can be customized and saved as a new custom theme.","color:#8792a6"));auto*buttons=new QVBoxLayout;auto*create=button("＋  Create from current profile");auto*edit=button("Edit / rename selected");auto*dup=button("Duplicate selected");auto*del=button("Delete selected custom theme");auto*imp=button("Import JSON");auto*exp=button("Export selected");for(auto*b:{create,edit,dup,del,imp,exp})buttons->addWidget(b);pvLayout->addLayout(buttons);connect(create,&QPushButton::clicked,this,&MainWindow::newTheme);connect(edit,&QPushButton::clicked,this,&MainWindow::editTheme);connect(dup,&QPushButton::clicked,this,&MainWindow::duplicateTheme);connect(del,&QPushButton::clicked,this,&MainWindow::deleteTheme);connect(imp,&QPushButton::clicked,this,&MainWindow::importTheme);connect(exp,&QPushButton::clicked,this,&MainWindow::exportTheme);row->addWidget(preview,1);l->addLayout(row,1);refreshThemes();return w;}
QWidget* MainWindow::buildDevice(){auto*w=makePage("Hardware","Explicitly apply a selected four-zone profile to the verified interface-0 device.");auto*l=qobject_cast<QVBoxLayout*>(w->layout());QVBoxLayout*pl;auto*p=panel("ITE8295 DEVICE PROFILE",&pl);for(const QString&s:{"Lenovo LOQ 15AHP9 · ITE8295 · USB 048D:C993","Runtime discovery verifies the device and interface 0","Interface 1 is never opened or communicated with","Four independent physical RGB zones","Static RGB write is user-reported working; not verified during this development run","Wave and Breathing hardware effects are reference-derived and unverified","Fn + Space remains the native manual recovery control"})pl->addWidget(label(s,"font-size:14px;padding:7px;color:#b3bed0"));m_hardwareMode=new QCheckBox("Use hardware when I apply a profile");connect(m_hardwareMode,&QCheckBox::toggled,this,&MainWindow::hardwareModeChanged);pl->addWidget(m_hardwareMode);m_hardwareStatus=label("Simulation mode · no hardware communication","color:#61e6ac;padding:12px;background:#12231d;border-radius:8px");m_hardwareStatus->setWordWrap(true);pl->addWidget(m_hardwareStatus);auto*apply=button("Apply selected profile","accent");m_applyButtons.append(apply);connect(apply,&QPushButton::clicked,this,[this]{if(m_activePresetId.isEmpty())showMessage("Select a preset before applying it.",true);else applyProfile();});pl->addWidget(apply);l->addWidget(p);auto*note=label("The interface-0 descriptor declares 32 Feature data bytes (33-byte hidraw report including ID). The observed GET returned 11 bytes. The candidate SET payload is reference-derived; framing is descriptor-supported, but field meanings and Wave/Breathing behavior are unverified. Preview animation never sends hardware commands. Each hardware Apply requires confirmation.","color:#e6c978;padding:16px;background:#242117;border-radius:10px");note->setWordWrap(true);l->addWidget(note);l->addStretch();return w;}
QWidget* MainWindow::buildSettings(){auto*w=makePage("Settings","Application behavior, persistence and startup.");auto*l=qobject_cast<QVBoxLayout*>(w->layout());QVBoxLayout*pl;auto*p=panel("STARTUP",&pl);m_autostart=new QCheckBox("Restore last successfully applied profile on login");pl->addWidget(m_autostart);pl->addWidget(label("Adds or removes this app's entry from ~/.config/autostart. Login restoration starts in simulation mode and does not send hardware commands.","color:#8792a6"));connect(m_autostart,&QCheckBox::toggled,this,&MainWindow::toggleAutostart);l->addWidget(p);auto*info=panel("PERSISTENCE");auto*il=qobject_cast<QVBoxLayout*>(info->layout());il->addWidget(label("Themes and custom animation presets use versioned JSON. The last-applied profile is updated only after a successful operation.","color:#a6b1c4"));l->addWidget(info);l->addStretch();const QString path=QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)+"/autostart/loq-rgb-studio.desktop";m_autostart->setChecked(QFile::exists(path));return w;}
void MainWindow::loadControls(){if(m_effect){m_effect->setCurrentIndex(static_cast<int>(m_profile.effect));m_direction->setCurrentIndex(m_profile.direction==Direction::LeftToRight?0:1);m_brightness->setValue(m_profile.brightness);m_speed->setValue(m_profile.speed);const QColor c=m_profile.zones[static_cast<size_t>(m_selectedZone)];m_hex->setText(c.name(QColor::HexRgb).toUpper());m_rgb->setText(QString("%1, %2, %3").arg(c.red()).arg(c.green()).arg(c.blue()));m_zoneColor->setText(QString("Zone %1 colour").arg(m_selectedZone+1));m_zoneColor->setStyleSheet(QString("border-left:5px solid %1").arg(c.name()));}}
void MainWindow::collectControls(){if(!m_effect||!m_pages||m_pages->currentIndex()!=2)return;m_profile.effect=static_cast<Effect>(m_effect->currentIndex());m_profile.direction=m_direction->currentIndex()==0?Direction::LeftToRight:Direction::RightToLeft;m_profile.brightness=m_brightness->value();m_profile.speed=m_speed->value();m_engine.setProfile(m_profile);if(m_liveEffect)m_liveEffect->setText(QString("%1  ·  %2% brightness").arg(effectName(m_profile.effect)).arg(m_profile.brightness));}
void MainWindow::updatePreview(){
    const auto colors=m_engine.frame();
    const auto keyColors=m_engine.keyFrame(m_keyboardLayout,m_zoneMap);
    for(KeyboardPreview*preview:{m_preview,m_studioPreview,m_reactivePreview,m_colorPreview}){
        if(preview){preview->setColors(colors);preview->setKeyColors(keyColors);preview->setSelectedZone(m_selectedZone);}
    }
}
void MainWindow::selectZone(int z){m_selectedZone=qBound(0,z,3);loadControls();updatePreview();}
void MainWindow::chooseColor(){const QColor c=QColorDialog::getColor(m_profile.zones[static_cast<size_t>(m_selectedZone)],this,"Choose zone colour");if(c.isValid()){setZoneColor(m_selectedZone,c);}}
void MainWindow::setZoneColor(int zone, const QColor &c){if(!c.isValid())return;zone=qBound(0,zone,3);m_profile.zones[static_cast<size_t>(zone)]=c;if(m_previewMode&&m_previewMode->currentIndex()!=0)m_previewMode->setCurrentIndex(0);m_engine.setProfile(m_profile);loadControls();updatePreview();}
void MainWindow::applyProfile(){
    if(m_applyPending){showMessage("An apply operation is already in progress.",true);return;}
    collectControls();
    QString error;
    if(!m_profile.isValid(&error)){showMessage(error,true);return;}

    if(m_hardwareMode&&m_hardwareMode->isChecked()){
        const QByteArray report=Ite8295Backend::buildFeatureReport(m_profile,&error);
        if(report.isEmpty()){showMessage(error,true);return;}
        const QString details=QString(
            "This sends one HID Feature SET report to the discovered USB 048D:C993 "
            "interface 0. Interface 1 is not used.\n\n"
            "The packet is based on the LegionAura reference and the descriptor's "
            "33-byte hidraw framing. The device's 11-byte GET response leaves the "
            "SET field meanings and acceptance unverified. This may change keyboard "
            "lighting; Fn + Space remains the manual recovery control. The packet "
            "contains only the selected profile effect and base zone colours; the "
            "software preview mode, timeline, and reactive key events are not streamed.\n\n"
            "Candidate packet (%1 bytes):\n%2\n\n"
            "Continue with this hardware write?")
            .arg(report.size()).arg(QString::fromLatin1(report.toHex(' ').toUpper()));
        if(QMessageBox::warning(this,"Confirm unverified keyboard write",details,
                                QMessageBox::Yes|QMessageBox::Cancel,
                                QMessageBox::Cancel)!=QMessageBox::Yes){
            showMessage("Hardware apply cancelled; no command sent.");
            return;
        }
        m_applyPending=true;
        updateApplyButtons();
        setHardwareStatus("Hardware apply in progress · interface 0 only");
        emit hardwareApplyRequested(m_profile);
        showMessage("Sending the confirmed Feature SET request on the hardware worker.");
        return;
    }

    if(!m_backend.apply(m_profile,&error)){showMessage(error,true);return;}
    if(!ThemeStore::saveLastApplied(m_profile,&error)){showMessage("Profile applied in simulation, but could not save it: "+error,true);return;}
    showMessage("Applied and saved: "+m_profile.name+" (simulation)");
}
void MainWindow::hardwareModeChanged(bool enabled){
    updateApplyButtons();
    if(m_modeIndicator){
        m_modeIndicator->setText(enabled?"●  HARDWARE MODE SELECTED":"●  SIMULATION MODE");
        m_modeIndicator->setStyleSheet(enabled
            ?"color:#e6c978;font-size:11px;font-weight:700;padding:10px 8px;background:#242117;border-radius:8px"
            :"color:#5df0b0;font-size:11px;font-weight:700;padding:10px 8px;background:#14251f;border-radius:8px");
    }
    if(m_dashboardDeviceStatus){
        m_dashboardDeviceStatus->setText(enabled?"●  Hardware mode selected · awaiting Apply":"●  Simulation active");
        m_dashboardDeviceStatus->setStyleSheet(enabled?"color:#e6c978;font-weight:700":"color:#61e6ac;font-weight:700");
    }
    setHardwareStatus(enabled
        ? "Hardware mode selected · no command is sent until Apply and a second confirmation."
        : "Simulation mode · no hardware communication");
    showMessage(enabled?"Hardware mode selected; Apply will show the candidate packet and request confirmation."
                       :"Simulation mode selected.");
}
void MainWindow::hardwareApplyFinished(const Profile &profile,bool success,const QString &message){
    m_applyPending=false;
    updateApplyButtons();
    if(!success){
        setHardwareStatus("Hardware apply failed: "+message,true);
        if(m_dashboardDeviceStatus){
            m_dashboardDeviceStatus->setText("●  Hardware apply failed");
            m_dashboardDeviceStatus->setStyleSheet("color:#ff8794;font-weight:700");
        }
        showMessage("Hardware apply failed: "+message,true);
        return;
    }
    QString saveError;
    const bool saved=ThemeStore::saveLastApplied(profile,&saveError);
    const QString transferMessage="Feature SET transfer completed; physical lighting effect is not independently verified.";
    setHardwareStatus(transferMessage+QStringLiteral(" Interface: 0 only."));
    if(m_dashboardDeviceStatus){
        m_dashboardDeviceStatus->setText("●  Transfer completed · effect unverified");
        m_dashboardDeviceStatus->setStyleSheet("color:#e6c978;font-weight:700");
    }
    if(saved){
        showMessage("Hardware transfer completed and profile saved. Physical lighting is unverified.");
    }else{
        showMessage("Hardware transfer completed, but profile could not be saved: "+saveError,true);
    }
}
void MainWindow::updateApplyButtons(){
    const bool hardware=m_hardwareMode&&m_hardwareMode->isChecked();
    for(QPushButton*button:m_applyButtons){
        button->setText(m_applyPending?"Apply in progress…":
                       hardware?"Review and send hardware profile…":"Apply simulation profile");
        button->setEnabled(!m_applyPending);
    }
    if(m_hardwareMode)m_hardwareMode->setEnabled(!m_applyPending);
}
void MainWindow::setHardwareStatus(const QString &status,bool error){
    if(!m_hardwareStatus)return;
    m_hardwareStatus->setText(status);
    m_hardwareStatus->setStyleSheet(error
        ?"color:#ff8794;padding:12px;background:#2a171c;border-radius:8px"
        :"color:#61e6ac;padding:12px;background:#12231d;border-radius:8px");
}
void MainWindow::showMessage(const QString&s,bool e){m_status->setText((e?"Error · ":"Ready · ")+s);m_status->setStyleSheet(e?"color:#ff7184":"color:#61e6ac");}
void MainWindow::navTo(int i){static const int pageIndexes[]={1,6,7};if(i<0||i>2)return;m_pages->setCurrentIndex(pageIndexes[i]);const auto buttons=findChildren<QPushButton*>();int n=0;for(auto*b:buttons)if(b->property("nav").toBool())b->setChecked(n++==i);}
void MainWindow::refreshThemes(){if(!m_themeList)return;m_themeList->clear();for(const auto&p:builtInProfiles()){auto*i=new QListWidgetItem("✦  "+p.name+"    ·    "+effectName(p.effect));i->setData(Qt::UserRole,"builtin");i->setData(Qt::UserRole+1,p.toJson());m_themeList->addItem(i);}if(!m_custom.isEmpty())m_themeList->addItem("—  MY THEMES  —");for(const auto&p:m_custom){auto*i=new QListWidgetItem("◈  "+p.name+"    ·    "+effectName(p.effect));i->setData(Qt::UserRole,"custom");i->setData(Qt::UserRole+1,p.toJson());m_themeList->addItem(i);}}
void MainWindow::applyThemeToControls(const Profile&p){m_profile=p;loadControls();m_engine.setProfile(m_profile);updatePreview();}
void MainWindow::themeSelected(QListWidgetItem*i){if(!i)return;const QVariant data=i->data(Qt::UserRole+1);if(!data.canConvert<QJsonObject>())return;auto o=data.toJsonObject();if(o.isEmpty())return;o.insert("schemaVersion",1);Profile p;QString e;if(Profile::fromJson(o,&p,&e)){applyThemeToControls(p);m_profile.name=p.name;} }
void MainWindow::newTheme(){collectControls();bool ok=false;QString n=QInputDialog::getText(this,"Create theme","Theme name",QLineEdit::Normal,m_profile.name,&ok);if(!ok)return;m_profile.name=n.trimmed();QString e;if(!m_profile.isValid(&e)){showMessage(e,true);return;}m_custom.append(m_profile);if(!ThemeStore::saveCustom(m_custom,&e)){m_custom.removeLast();showMessage(e,true);return;}refreshThemes();showMessage("Theme saved.");}
void MainWindow::editTheme(){auto*i=m_themeList->currentItem();if(!i||i->data(Qt::UserRole).toString()!="custom"){showMessage("Select a custom theme to edit.",true);return;}int idx=m_themeList->row(i)-builtInProfiles().size()-1;if(idx<0||idx>=m_custom.size())return;Profile p=m_custom[idx];QDialog dialog(this);dialog.setWindowTitle("Edit custom theme");dialog.setMinimumWidth(420);auto*layout=new QVBoxLayout(&dialog);auto*form=new QFormLayout;auto*name=new QLineEdit(p.name);auto*effect=new QComboBox;effect->addItems({"Static","Breathing","Wave","Hue cycling"});effect->setCurrentIndex(static_cast<int>(p.effect));auto*direction=new QComboBox;direction->addItems({"Left to right","Right to left"});direction->setCurrentIndex(p.direction==Direction::LeftToRight?0:1);auto*brightness=new QSlider(Qt::Horizontal);brightness->setRange(0,100);brightness->setValue(p.brightness);auto*speed=new QSlider(Qt::Horizontal);speed->setRange(1,100);speed->setValue(p.speed);form->addRow("Name",name);form->addRow("Effect",effect);form->addRow("Direction",direction);form->addRow("Brightness",brightness);form->addRow("Speed",speed);layout->addLayout(form);QColor zones[4];for(int z=0;z<4;++z){zones[z]=p.zones[static_cast<size_t>(z)];auto*b=button(QString("Zone %1 · %2").arg(z+1).arg(zones[z].name().toUpper()));b->setStyleSheet(QString("border-left:5px solid %1").arg(zones[z].name()));connect(b,&QPushButton::clicked,&dialog,[&,z,b]{QColor c=QColorDialog::getColor(zones[z],&dialog,QString("Zone %1 colour").arg(z+1));if(c.isValid()){zones[z]=c;b->setText(QString("Zone %1 · %2").arg(z+1).arg(c.name().toUpper()));b->setStyleSheet(QString("border-left:5px solid %1").arg(c.name()));}});layout->addWidget(b);}auto*buttons=new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel);connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);layout->addWidget(buttons);if(dialog.exec()!=QDialog::Accepted)return;p.name=name->text().trimmed();p.effect=static_cast<Effect>(effect->currentIndex());p.direction=direction->currentIndex()==0?Direction::LeftToRight:Direction::RightToLeft;p.brightness=brightness->value();p.speed=speed->value();for(int z=0;z<4;++z)p.zones[static_cast<size_t>(z)]=zones[z];QString e;if(!p.isValid(&e)){showMessage(e,true);return;}m_custom[idx]=p;if(!ThemeStore::saveCustom(m_custom,&e)){showMessage(e,true);return;}refreshThemes();showMessage("Theme updated.");}
void MainWindow::duplicateTheme(){if(m_themeList->currentItem())themeSelected(m_themeList->currentItem());m_profile.name+=" Copy";newTheme();}
void MainWindow::deleteTheme(){auto*i=m_themeList->currentItem();if(!i||i->data(Qt::UserRole).toString()!="custom"){showMessage("Only custom themes can be deleted.",true);return;}int idx=m_themeList->row(i)-builtInProfiles().size()-1;if(idx<0||idx>=m_custom.size())return;m_custom.removeAt(idx);QString e;if(!ThemeStore::saveCustom(m_custom,&e))showMessage(e,true);else{refreshThemes();showMessage("Theme deleted.");}}
void MainWindow::importTheme(){const QString f=QFileDialog::getOpenFileName(this,"Import theme",{},"LOQ RGB themes (*.json)");if(f.isEmpty())return;Profile p;QString e;if(!ThemeStore::readTheme(f,&p,&e)){showMessage("Import failed: "+e,true);return;}m_custom.append(p);if(!ThemeStore::saveCustom(m_custom,&e)){m_custom.removeLast();showMessage(e,true);return;}refreshThemes();applyThemeToControls(p);showMessage("Theme imported.");}
void MainWindow::exportTheme(){if(m_themeList->currentItem())themeSelected(m_themeList->currentItem());QString f=QFileDialog::getSaveFileName(this,"Export theme",m_profile.name+".json","LOQ RGB themes (*.json)");if(f.isEmpty())return;if(!f.endsWith(".json",Qt::CaseInsensitive))f+=".json";QString e;if(!ThemeStore::writeTheme(f,m_profile,&e))showMessage("Export failed: "+e,true);else showMessage("Theme exported.");}
void MainWindow::toggleAutostart(bool enabled){const QString dir=QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)+"/autostart",path=dir+"/loq-rgb-studio.desktop";if(!enabled){QFile::remove(path);showMessage("Login restoration disabled.");return;}QDir().mkpath(dir);QFile f(path);if(!f.open(QIODevice::WriteOnly|QIODevice::Truncate)){showMessage("Cannot write autostart entry: "+f.errorString(),true);m_autostart->blockSignals(true);m_autostart->setChecked(false);m_autostart->blockSignals(false);return;}f.write("[Desktop Entry]\nType=Application\nName=LOQ RGB Studio\nExec=loq-rgb-studio --restore\nTerminal=false\nX-GNOME-Autostart-enabled=true\n");showMessage("Login restoration enabled (simulation mode).");}
void MainWindow::createTray(){m_tray=new QSystemTrayIcon(QIcon::fromTheme("loq-rgb-studio"),this);if(m_tray->icon().isNull())m_tray->setIcon(style()->standardIcon(QStyle::SP_ComputerIcon));auto*menu=new QMenu;menu->addAction("Open LOQ RGB Studio",this,[this]{showNormal();raise();activateWindow();});menu->addAction("Apply saved profile",this,[this]{QString e;Profile p;if(ThemeStore::loadLastApplied(&p,&e)){applyThemeToControls(p);applyProfile();}else showMessage("No saved profile: "+e,true);});menu->addSeparator();menu->addAction("Quit",qApp,&QApplication::quit);m_tray->setContextMenu(menu);connect(m_tray,&QSystemTrayIcon::activated,this,[this](QSystemTrayIcon::ActivationReason r){if(r==QSystemTrayIcon::Trigger){showNormal();raise();activateWindow();}});if(QSystemTrayIcon::isSystemTrayAvailable())m_tray->show();}
void MainWindow::closeEvent(QCloseEvent*e){if(!m_tray||!m_tray->isVisible()){e->accept();QTimer::singleShot(0,qApp,&QApplication::quit);return;}hide();m_tray->showMessage("LOQ RGB Studio","Still running in the system tray.",QSystemTrayIcon::Information,1800);e->ignore();}
void MainWindow::changeEvent(QEvent*e){QMainWindow::changeEvent(e);if(e->type()==QEvent::WindowStateChange&&isMinimized()&&m_tray&&m_tray->isVisible())QTimer::singleShot(0,this,[this]{hide();});}
