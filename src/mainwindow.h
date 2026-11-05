#pragma once
#include <QMainWindow>
#include <QTabWidget>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QListWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QProgressBar>
#include <QSlider>
#include <QPushButton>
#include <QListWidgetItem>
#include <QHash>
#include <QVector>
#include <QPixmap>
#include <QSplitter>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QFutureWatcher>
#include <QMovie>
#include <QTranslator>
#include <QActionGroup>
#include <QStackedWidget>

#include "core/logger.h"
#include "core/unpacker.h"
#include "core/patchrunner.h"
#include "dialogs/unpackleveldialog.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // --- File operations ---
    void onUnpackClicked();
    void onRepackToImageClicked();
    void onZipRomClicked();
    void onExportCTPClicked();

    // --- Boot media ---
    void onBootLogoButtonClicked();
    void onBootanimationButtonClicked();
    void onBootvideoButtonClicked();
    void onPlayBootanimationClicked();
    void onBootanimationChangeClicked();
    void onPlayBootvideoClicked();
    void onChangeBootvideoClicked();
    void onRefreshLogoClicked();

    // --- Apps tab ---
    void onAppsDirectoryChanged(int index);
    void onAddAppsClicked();
    void onRemoveAppsClicked();
    void onUnpackApkClicked();
    void onRepackApkClicked();
    void onAppSelectionChanged();

    // --- Kernel/Recovery ---
    void onOpenKernelClicked();
    void onOpenRecoveryClicked();
    void onReplaceKernelClicked();
    void onReplaceRecoveryClicked();
    void onRootFirmwareClicked();
    void onRemoveRootClicked();
    void onBootImgForMagiskClicked();

    // --- DTB ---
    void onDtbComboChanged(int index);
    void onSaveDtbClicked();
    void onDiscardDtbClicked();
    void onSearchNextClicked();
    void onSearchBackClicked();

    // --- Keys ---
    void onWriteKeysClicked();
    void onOpenWorkingClicked();

    // --- System/Vendor file ops ---
    void onOpenSystemClicked();
    void onOpenVendorClicked();
    void onOpenSystemBuildPropClicked();
    void onOpenVendorBuildPropClicked();
    void onOpenSystemKeymapClicked();
    void onOpenVendorKeymapClicked();
    void onOpenSystemGenericKlClicked();
    void onOpenVendorGenericKlClicked();
    void onOpenSystemRemoteConfClicked();
    void onOpenVendorRemoteConfClicked();
    void onOpenLogosClicked();

    // --- Patches ---
    void onPatchComboChanged(int index);
    void onApplyPatchClicked();
    void onAddPatchLabelClicked();

    // --- File search tab ---
    void onSearchTextChanged(const QString &text);

    // --- Locale ---
    void onLocaleComboChanged(int index);

    // --- Wifi ---
    void onMultiWifiDriversClicked();

    // --- Dev settings ---
    void onDevSettingsChanged();

    // --- Menu ---
    void onLogsClicked();
    void onTutorialItemClicked();

    // --- Language ---
    void setLanguage(const QString &localeCode);

    // --- Misc ---
    void onBootlogoTimer();
    void onPatchOutputAppended(const QString &text);

protected:
    void changeEvent(QEvent *event) override;

private:
    // ---- Setup helpers ----
    void setupUi();
    void retranslateUi();
    QWidget* buildInfoTab();
    QWidget* buildAppsTab();
    QWidget* buildBootMediaTab();
    QWidget* buildDtbTab();
    QWidget* buildKeysTab();
    QWidget* buildFilesystemTab();
    QWidget* buildPatchesTab();
    QWidget* buildPackTab();
    QWidget* buildFileSearchTab();
    QWidget* buildSettingsTab();
    QWidget* buildAboutTab();
    void applyAppTheme(int themeNum);
    void retranslateExtraPages();

    void setupMenuBar();
    void setupStatusBar();

    // ---- Business logic ----
    void loadRom();
    void loadInfo();
    void updateAppsView();
    void resetManifestInfo();
    void appManifestInfo(const QString &unpackedDir);
    void unpack(const QString &inputFile, const QString &unpackItems);
    void ext4PartitionUnpack(const QString &partition, const QString &outputDir);

    // Locate a partition file inside level1/ or level1/Image/ (vendor tools differ),
    // falling back to level2/super/ for logical partitions.
    QString findPartitionFile(const QString &name) const;

    // Locate a file such as "media/bootanimation.zip" inside the extracted partitions,
    // handling system-as-root layouts (level2/system/system/media/...) and vendor trees.
    QString findPartitionAsset(const QString &relativePath) const;
    void checkResources();
    QString getProp(const QString &property) const;
    QString checkSysDir() const;
    void applyDevSettings();
    void setStatusText(const QString &text);
    void setBusy(bool busy);
    QString binDir() const;
    QString tmpDir() const;
    void openFileManager(const QString &path);

    // ---- Member variables ----
    QString      m_appVersion = APP_VERSION;
    Logger      *m_logger     = nullptr;
    Unpacker    *m_unpacker   = nullptr;
    PatchRunner *m_patcher    = nullptr;

    QString m_sysDir;
    QString m_extPaths = "system,vendor,product,odm,oem,system_ext";

    enum class ImgType { Amlogic, Rockchip, Allwinner, None };
    ImgType m_imageType = ImgType::None;

    bool m_playingBootanim = false;
    int  m_searchIndex     = 1;
    bool m_uiReady = false;

    // ---- UI elements ----
    QTabWidget *m_mainTab = nullptr;          // page container; its tab bar is hidden
    QWidget    *m_sidebarContainer = nullptr;
    QListWidget *m_sidebarList = nullptr;     // visible navigation, drives m_mainTab
    QLabel     *m_sidebarVersion = nullptr;
    QLabel     *m_headerLogo = nullptr, *m_headerTitle = nullptr;
    void updateSidebarIcons();
    int         m_themeNum = 0;
    QComboBox  *m_themeCombo = nullptr, *m_langCombo = nullptr;
    QGroupBox  *m_setAppearanceGroup = nullptr, *m_setDevGroup = nullptr, *m_setToolsGroup = nullptr;
    QLabel     *m_setThemeCap = nullptr, *m_setLangCap = nullptr;
    QGroupBox  *m_aboutGroup = nullptr, *m_aboutLicenseGroup = nullptr;
    QLabel     *m_aboutTitle = nullptr, *m_aboutVersion = nullptr, *m_aboutLicense = nullptr, *m_aboutOrig = nullptr;
    QLabel     *m_aboutVersionCap = nullptr, *m_aboutLicenseCap = nullptr, *m_aboutOrigCap = nullptr;
    QLabel     *m_aboutSource = nullptr, *m_aboutSourceCap = nullptr;
    QLabel     *m_aboutDesc = nullptr, *m_aboutLicenseText = nullptr, *m_aboutSupport = nullptr;
    void updateSidebarTexts();

    // Info tab
    // Read-only value labels (the card-style Info tab shows these as QLabel, not QLineEdit)
    QLabel *m_leVersion = nullptr, *m_leModel = nullptr, *m_leVendor = nullptr, *m_leProduct = nullptr;
    QLabel *m_leSDK = nullptr, *m_lePatch = nullptr, *m_leBuild = nullptr, *m_leBuildDisplayId = nullptr;
    QLabel *m_leSystemSize = nullptr, *m_leSystemFiles = nullptr, *m_leSystemApps = nullptr;
    QLabel *m_leVendorSize = nullptr, *m_leVendorFiles = nullptr, *m_leVendorApps = nullptr;
    QCheckBox *m_cbRamdiskSystem = nullptr, *m_cbContainsVendor = nullptr, *m_cbMultiDtb = nullptr;
    QCheckBox *m_cbBootupGunzipped = nullptr, *m_cbSuperPartition = nullptr;
    QCheckBox *m_cbCompressBrotli = nullptr, *m_cbAddPlatformConfig = nullptr;
    QCheckBox *m_cbSpecifyChipset = nullptr;
    QLineEdit *m_leRKChipset = nullptr;
    QLabel    *m_lblPackageType = nullptr;
    QComboBox *m_cbLocales = nullptr;
    QLineEdit *m_leUpgradePackage = nullptr;
    QPushButton *m_btnUnpackImage = nullptr;

    // Apps tab
    QListWidget *m_appsViewer = nullptr;
    QComboBox   *m_appsDirectory = nullptr;
    QGroupBox   *m_apkGroupbox = nullptr;
    QLabel      *m_lblAppPackage = nullptr, *m_lblAppLauncherApp = nullptr;
    QLabel      *m_lblAppVisibleLauncher = nullptr, *m_lblAppVisibleTvLauncher = nullptr;
    QPushButton *m_btnUnpackApk = nullptr, *m_btnRepackApk = nullptr;
    QPushButton *m_btnAddApps = nullptr, *m_btnRemoveApps = nullptr;

    // Map an Android app folder (/system/app, /vendor/priv-app, …) to the extracted tree
    QString resolveAppDir(const QString &virtualPath) const;

    // Boot media tab
    QLabel      *m_bootlogoPicture = nullptr;
    QLabel      *m_bootanimPicture = nullptr;
    QLabel      *m_lblBootanimFPS = nullptr;

    // Bootanimation playback (desc.txt driven)
    QTimer      *m_bootanimTimer = nullptr;
    QStringList  m_bootanimFrames;      // all frame paths, parts concatenated
    QVector<int> m_bootanimPartStart;   // index in m_bootanimFrames where each part begins
    QVector<int> m_bootanimPartLoops;   // remaining loops per part (0 = loop forever)
    QVector<int> m_bootanimPartPause;   // pause in seconds after each part
    QHash<QString, QPixmap> m_bootanimCache;
    int          m_bootanimPart = 0;
    int          m_bootanimFrame = 0;
    int          m_bootanimFps = 25;
    bool         m_bootanimPlaying = false;

    void advanceBootanimFrame();
    void showBootanimFrame();
    void stopBootanimPreview();
    QMediaPlayer *m_videoPlayer = nullptr;
    QVideoWidget *m_videoWidget = nullptr;
    QWidget     *m_bootlogoPanel = nullptr, *m_bootanimPanel = nullptr, *m_bootvideoPanel = nullptr;
    QPushButton *m_btnBootLogo = nullptr, *m_btnBootanim = nullptr, *m_btnBootvideo = nullptr;
    QTimer      *m_bootlogoTimer = nullptr;

    // DTB tab
    QTextEdit   *m_dtbEditor = nullptr;
    QComboBox   *m_dtbCombo = nullptr;
    QLineEdit   *m_leSearchBox = nullptr;
    QWidget     *m_dtbPanel = nullptr;

    // Keys tab
    QListWidget *m_keysChecklist = nullptr;

    // Filesystem tab
    QGroupBox *m_systemGroupBox = nullptr, *m_vendorGroupBox = nullptr, *m_wifiGroupBox = nullptr;

    // Patches tab
    QComboBox   *m_patchCombo = nullptr;
    QTextEdit   *m_patchRTB = nullptr;
    QLabel      *m_lblPatchAuthor = nullptr, *m_lblPatchVersion = nullptr, *m_lblPatchDescription = nullptr;
    QWidget     *m_patchesPanel = nullptr;

    // Pack tab
    QSlider     *m_compressionSlider = nullptr;
    QLabel      *m_lblCompressionStatus = nullptr;
    QSlider     *m_ctpCompressionSlider = nullptr;
    QLabel      *m_lblCTPCompressionStatus = nullptr;

    // Status bar
    QLabel      *m_statusLabel = nullptr;
    QLabel      *m_versionLabel = nullptr;
    QProgressBar *m_statusProgress = nullptr;

    // File search tab
    QLineEdit   *m_leSearchFile = nullptr;
    QListWidget *m_filesList = nullptr;

    // Dev settings (menu checkboxes)
    QAction *m_actPythonSuperUtils = nullptr, *m_actNewDtbTools = nullptr, *m_actNewLogoTools = nullptr;
    QAction *m_actPythonImgExtractor = nullptr, *m_actArchMakeExt4fs = nullptr;
    QAction *m_actPythonSimg2img = nullptr, *m_actUseLegacyAWTools = nullptr, *m_actUseNewRKTools = nullptr;

    QNetworkAccessManager m_network;
    UnpackLevelDialog *m_unpackLevelDlg = nullptr;

    // Translation & Menus
    QTranslator m_translator;
    QString     m_currentLang;
    QMenu      *m_menuFile = nullptr;
    QMenu      *m_menuTools = nullptr;
    QMenu      *m_menuDev = nullptr;
    QMenu      *m_menuLanguage = nullptr;
    QMenu      *m_menuHelp = nullptr;
    QAction    *m_actOpen = nullptr;
    QAction    *m_actExit = nullptr;
    QAction    *m_actCheckRes = nullptr;
    QAction    *m_actOpenWorking = nullptr;
    QAction    *m_actOpenLogs = nullptr;
    QAction    *m_actTutorial = nullptr;
    QAction    *m_actLangEn = nullptr;
    QAction    *m_actLangZhCn = nullptr;
    QAction    *m_actLangZhTw = nullptr;
    QAction    *m_actLangJa = nullptr;
    QAction    *m_actLangRu = nullptr;

    // Dynamic UI pointers for retranslation
    QGroupBox *m_propGroup = nullptr, *m_statsGroup = nullptr, *m_flagsGroup = nullptr;
    QLabel    *m_lblPropVer = nullptr, *m_lblPropModel = nullptr, *m_lblPropVendor = nullptr, *m_lblPropProduct = nullptr;
    QLabel    *m_lblPropSdk = nullptr, *m_lblPropPatch = nullptr, *m_lblPropBuild = nullptr, *m_lblPropDispId = nullptr;
    QLabel    *m_lblStatsSysSize = nullptr, *m_lblStatsSysFiles = nullptr, *m_lblStatsSysApps = nullptr;
    QLabel    *m_lblStatsVenSize = nullptr, *m_lblStatsVenFiles = nullptr, *m_lblStatsVenApps = nullptr;
    QLabel    *m_lblPkgTypeTitle = nullptr, *m_lblLocaleTitle = nullptr;
    QLabel    *m_lblAppsDir = nullptr;
    QTabWidget *m_mediaTabs = nullptr;
    QPushButton *m_btnRefreshLogo = nullptr, *m_btnOpenLogos = nullptr;
    QPushButton *m_btnPlayAnim = nullptr, *m_btnChangeAnim = nullptr;
    QPushButton *m_btnPlayVideo = nullptr, *m_btnChangeVideo = nullptr;
    QLabel    *m_lblDtsFile = nullptr;
    QPushButton *m_btnDtbNext = nullptr, *m_btnDtbBack = nullptr, *m_btnDtbSave = nullptr, *m_btnDtbDiscard = nullptr;
    QLabel    *m_lblKeysInfo = nullptr;
    QPushButton *m_btnWriteKeys = nullptr, *m_btnOpenWorkingKeys = nullptr;
    QPushButton *m_btnOpenSys = nullptr, *m_btnSysProp = nullptr, *m_btnSysKeymap = nullptr, *m_btnSysGenKl = nullptr, *m_btnSysRemConf = nullptr;
    QPushButton *m_btnOpenVen = nullptr, *m_btnVenProp = nullptr, *m_btnVenKeymap = nullptr, *m_btnVenGenKl = nullptr, *m_btnVenRemConf = nullptr;
    QGroupBox *m_kernelGroup = nullptr;
    QPushButton *m_btnOpenKern = nullptr, *m_btnOpenRec = nullptr, *m_btnReplKern = nullptr, *m_btnReplRec = nullptr, *m_btnMagiskBoot = nullptr;
    QPushButton *m_btnRoot = nullptr, *m_btnUnroot = nullptr, *m_btnWifi = nullptr;
    QLabel    *m_lblPatches = nullptr, *m_lblPatchExec = nullptr;
    QLabel    *m_lblAuthorTitle = nullptr, *m_lblVerMetaTitle = nullptr, *m_lblDescTitle = nullptr;
    QPushButton *m_btnApplyPatch = nullptr, *m_btnOpenPatches = nullptr;
    QGroupBox *m_imgGroup = nullptr, *m_zipGroup = nullptr, *m_ctpGroup = nullptr;
    QPushButton *m_btnRepackImg = nullptr, *m_btnZipRom = nullptr, *m_btnCtp = nullptr;
    QLabel    *m_lblCompSlider = nullptr, *m_lblCtpSlider = nullptr;
    QLabel    *m_lblSearchFile = nullptr;

    static long romSize(const QString &dir);
    static int  filesAmount(const QString &dir);
};
