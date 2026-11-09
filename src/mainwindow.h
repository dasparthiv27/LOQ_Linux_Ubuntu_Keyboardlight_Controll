#pragma once
#include "animationengine.h"
#include "animationpresets.h"
#include "hardware/keyboardbackend.h"
#include "keyboardinputlistener.h"
#include "keyboardpreview.h"
#include "themestore.h"
#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QStackedWidget>
#include <QListWidget>
#include <QComboBox>
#include <QSlider>
#include <QLineEdit>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QStatusBar>
#include <QTimer>
#include <QThread>

class QShortcut;

class MainWindow final:public QMainWindow {
    Q_OBJECT
public: explicit MainWindow(bool restoreMode=false,QWidget*parent=nullptr);
    ~MainWindow() override;
protected: void closeEvent(QCloseEvent*) override;
    void changeEvent(QEvent*) override;
private slots:
    void applyProfile(); void updatePreview(); void selectZone(int); void chooseColor(); void importTheme(); void exportTheme(); void newTheme(); void editTheme(); void duplicateTheme(); void deleteTheme(); void themeSelected(QListWidgetItem*); void navTo(int); void toggleAutostart(bool); void hardwareModeChanged(bool); void refreshInputDevices(); void toggleInputListener(bool);
    void hardwareApplyFinished(const Profile &profile, bool success, const QString &message);
private:
    QWidget* makePage(const QString&title,const QString&subtitle); QWidget* buildDashboard(); QWidget* buildPresets(); QWidget* buildSimplePresets(); QWidget* buildStudio(); QWidget* buildReactive(); QWidget* buildColorEditor(); QWidget* buildThemes(); QWidget* buildDevice(); QWidget* buildSettings(); QWidget* panel(const QString&title,QVBoxLayout**layout=nullptr);
    void loadControls(); void collectControls(); void refreshThemes(); void applyThemeToControls(const Profile&); void showMessage(const QString&,bool error=false); void createTray(); void setZoneColor(int zone, const QColor &color);
    void updateApplyButtons();
    void setHardwareStatus(const QString &status, bool error = false);
    void handlePreviewKey(int keyId, const QPointF &position, int zone);
    void updateAnimationControls();
    void refreshSimplePresetCards();
    bool saveCustomPresetCollection();
    void applyPreset(const AnimationPreset &preset, bool sendToDevice);
    void editPreset(const AnimationPreset &preset, bool creating);
    void cycleCustomPresets();
    Profile m_profile; QList<Profile> m_custom; int m_selectedZone=0; AnimationEngine m_engine; SimulationBackend m_backend;
    KeyboardLayout m_keyboardLayout;
    KeyboardZoneMap m_zoneMap;
    QList<AnimationPreset> m_presets;
    QList<AnimationPreset> m_customPresets;
    QVector<ColorStop> m_gradientStops{{0.0,QColor("#18d9ff")},{0.5,QColor("#a43cff")},{1.0,QColor("#ff3d88")}};
    QString m_activePresetId;
    PresetColorMode m_activeColorMode=PresetColorMode::Individual;
    QVBoxLayout *m_myPresetCards=nullptr;
    QVBoxLayout *m_builtinPresetCards=nullptr;
    QShortcut *m_cycleShortcut=nullptr;
    QStackedWidget*m_pages=nullptr; QListWidget*m_themeList=nullptr; QListWidget*m_presetsList=nullptr; QComboBox*m_presetCategory=nullptr; QLabel*m_presetDescription=nullptr; QLabel*m_presetCompatibility=nullptr; KeyboardPreview*m_preview=nullptr; KeyboardPreview*m_studioPreview=nullptr; KeyboardPreview*m_reactivePreview=nullptr; KeyboardPreview*m_colorPreview=nullptr; QLabel*m_status=nullptr; QLabel*m_liveEffect=nullptr; QLabel*m_animationProgress=nullptr; QLabel*m_reactiveStatus=nullptr;
    QComboBox*m_effect=nullptr; QComboBox*m_direction=nullptr; QComboBox*m_previewMode=nullptr; QComboBox*m_reactiveMode=nullptr; QComboBox*m_inputDevice=nullptr; QComboBox*m_calibrationZone=nullptr; QComboBox*m_gradientDirection=nullptr; QComboBox*m_paletteMode=nullptr; QSlider*m_brightness=nullptr; QSlider*m_speed=nullptr; QSlider*m_reactiveDecay=nullptr; QSlider*m_duration=nullptr; QSlider*m_fadeIn=nullptr; QSlider*m_fadeOut=nullptr; QSlider*m_gradientMidpoint=nullptr; QSlider*m_rippleSpeed=nullptr; QSlider*m_rippleRadius=nullptr; QSlider*m_waveCount=nullptr; QSlider*m_reactiveIntensity=nullptr; QSlider*m_rainbowSaturation=nullptr; QSlider*m_breathingDepth=nullptr; QLineEdit*m_hex=nullptr; QLineEdit*m_rgb=nullptr; QPushButton*m_zoneColor=nullptr; QPushButton*m_gradientStart=nullptr; QPushButton*m_gradientMiddle=nullptr; QPushButton*m_gradientEnd=nullptr; QCheckBox*m_looping=nullptr; QCheckBox*m_reverse=nullptr; QCheckBox*m_hardwareMode=nullptr; QCheckBox*m_zoneCalibration=nullptr; QCheckBox*m_inputEnabled=nullptr; QLabel*m_hardwareStatus=nullptr; QLabel*m_modeIndicator=nullptr; QLabel*m_dashboardDeviceStatus=nullptr; QList<QPushButton*> m_applyButtons;
    QCheckBox*m_autostart=nullptr; QSystemTrayIcon*m_tray=nullptr; bool m_quit=false; bool m_applyPending=false;
    QThread m_hardwareThread; HardwareApplyWorker *m_hardwareWorker=nullptr;
    KeyboardInputListener *m_inputListener=nullptr;
    QTimer m_animationTimer;
signals:
    void hardwareApplyRequested(const Profile &profile);
};
