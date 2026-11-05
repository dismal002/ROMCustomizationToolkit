#include "mainwindow.h"
#include "dialogs/downloaddialog.h"
#include "dialogs/tutorialdialog.h"
#include "theme/materialtheme.h"
#include "theme/iconhelper.h"
#include "core/platform.h"

#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QTextStream>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMenuBar>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QStatusBar>
#include <QPixmap>
#include <QImage>
#include <QSettings>
#include <QtConcurrent>
#include <QThread>
#include <cstdio>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_logger(new Logger(Platform::logsDir() + "/app.log"))
    , m_unpacker(new Unpacker(m_logger))
    , m_patcher(new PatchRunner(m_logger, this))
    , m_videoPlayer(new QMediaPlayer(this))
    , m_videoWidget(new QVideoWidget(this))
    , m_bootlogoTimer(new QTimer(this))
{
    m_unpacker->binDir = binDir();
    m_patcher->imageType = "amlogic";

    // Ensure required runtime directories exist
    Platform::ensureLayout();

    m_logger->log("[MAIN] ROM Customization Toolkit Linux/Qt initialized. Version " + m_appVersion);

    setupMenuBar();      // creates the QAction objects only (no menu bar is shown)
    applyDevSettings();  // load persisted developer-tool toggles before the Settings page reads them
    setupUi();
    setupStatusBar();

    // Connect patcher output signal
    connect(m_patcher, &PatchRunner::outputAppended, this, &MainWindow::onPatchOutputAppended);

    // Boot logo preview refresh timer
    connect(m_bootlogoTimer, &QTimer::timeout, this, &MainWindow::onBootlogoTimer);
    m_bootlogoTimer->setInterval(3000);

    // Load saved language
    QSettings settings("ROM Customization Toolkit", "Settings");
    QString savedLang = settings.value("Language", "en").toString();
    setLanguage(savedLang);

    // Load existing unpacked ROM if present
    loadRom();
}

MainWindow::~MainWindow()
{
    if (m_bootlogoTimer && m_bootlogoTimer->isActive()) {
        m_bootlogoTimer->stop();
    }
    delete m_unpacker;
    delete m_logger;
}

QString MainWindow::binDir() const
{
    return Platform::binDir();
}

QString MainWindow::tmpDir() const
{
    return Platform::tmpDir();
}

void MainWindow::openFileManager(const QString &path)
{
    if (!QDir(path).exists() && !QFile::exists(path)) {
        QMessageBox::information(this, tr("Notice"), tr("Target path does not exist:\n%1").arg(path));
        return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void MainWindow::setStatusText(const QString &text)
{
    m_statusLabel->setText(text);
    m_logger->log("[STATUS] " + text);
    QApplication::processEvents();
}

void MainWindow::setBusy(bool busy)
{
    m_statusProgress->setVisible(busy);
    if (busy) {
        m_statusProgress->setRange(0, 0); // indeterminate
    } else {
        m_statusProgress->setRange(0, 100);
        m_statusProgress->setValue(0);
    }
    m_mainTab->setEnabled(!busy);
    m_btnUnpackImage->setEnabled(!busy);
    QApplication::processEvents();
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::setLanguage(const QString &localeCode)
{
    qApp->removeTranslator(&m_translator);
    m_currentLang = localeCode;

    if (localeCode != "en") {
        bool loaded = m_translator.load(":/translations/ct_" + localeCode + ".qm");
        if (!loaded) {
            loaded = m_translator.load(Platform::translationsDir() + "/ct_" + localeCode + ".qm");
        }
        if (loaded) {
            qApp->installTranslator(&m_translator);
        }
    }

    if (m_actLangEn)   m_actLangEn->setChecked(localeCode == "en");
    if (m_actLangZhCn) m_actLangZhCn->setChecked(localeCode == "zh_CN");
    if (m_actLangZhTw) m_actLangZhTw->setChecked(localeCode == "zh_TW");
    if (m_actLangJa)   m_actLangJa->setChecked(localeCode == "ja_JP");
    if (m_actLangRu)   m_actLangRu->setChecked(localeCode == "ru_RU");

    if (m_langCombo) {
        const QSignalBlocker blocker(m_langCombo);
        int idx = m_langCombo->findData(localeCode);
        if (idx >= 0) m_langCombo->setCurrentIndex(idx);
    }

    QSettings settings("ROM Customization Toolkit", "Settings");
    settings.setValue("Language", localeCode);

    if (m_uiReady) {
        retranslateUi();
    }
}

// -------------------------------------------------------------
// UI Construction
// -------------------------------------------------------------
void MainWindow::setupUi()
{
    resize(1200, 760);
    setMinimumSize(980, 620);
    setWindowTitle("Android Toolbox");
    m_themeNum = QSettings("ROM Customization Toolkit", "Settings").value("Theme", 0).toInt();
    if (m_themeNum < 0 || m_themeNum > 5) m_themeNum = 0;

    auto *centralWidget = new QWidget(this);
    centralWidget->setObjectName("centralRoot");
    auto *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ---------------- Header bar (styled by MaterialTheme via #headerBar) ----------------
    auto *headerBar = new QWidget(centralWidget);
    headerBar->setObjectName("headerBar");
    auto *headerLayout = new QHBoxLayout(headerBar);
    headerLayout->setContentsMargins(18, 0, 18, 0);
    headerLayout->setSpacing(14);

    m_headerLogo = new QLabel(headerBar);
    m_headerLogo->setPixmap(IconHelper::createPixmapFromSvg(":/icons/apps.svg", Qt::white, 26));
    m_headerLogo->setFixedSize(26, 26);

    m_headerTitle = new QLabel("ROM Customization Toolkit", headerBar);
    m_headerTitle->setObjectName("headerTitle");

    m_leUpgradePackage = new QLineEdit(headerBar);
    m_leUpgradePackage->setObjectName("firmwarePath");
    m_leUpgradePackage->setReadOnly(true);
    m_leUpgradePackage->setPlaceholderText("No firmware package loaded...");
    m_leUpgradePackage->setMinimumWidth(260);

    m_btnUnpackImage = new QPushButton("Unpack Image", headerBar);
    m_btnUnpackImage->setObjectName("unpackButton");
    m_btnUnpackImage->setCursor(Qt::PointingHandCursor);
    connect(m_btnUnpackImage, &QPushButton::clicked, this, &MainWindow::onUnpackClicked);

    headerLayout->addWidget(m_headerLogo);
    headerLayout->addWidget(m_headerTitle);
    headerLayout->addStretch(1);
    headerLayout->addWidget(m_leUpgradePackage, 2);
    headerLayout->addWidget(m_btnUnpackImage);

    mainLayout->addWidget(headerBar);

    // ---------------- Pages: QTabWidget with hidden tab bar ----------------
    // (A West-positioned QTabBar rotates the text and clips long labels, so navigation is
    //  done by the sidebar list below; the tab widget just holds/switches the pages.)
    m_mainTab = new QTabWidget(centralWidget);
    m_mainTab->setObjectName("mainTab");
    m_mainTab->setDocumentMode(true);
    m_mainTab->tabBar()->hide();

    m_mainTab->addTab(buildInfoTab(), "");
    m_mainTab->addTab(buildAppsTab(), "");
    m_mainTab->addTab(buildBootMediaTab(), "");
    m_mainTab->addTab(buildDtbTab(), "");
    m_mainTab->addTab(buildKeysTab(), "");
    m_mainTab->addTab(buildFilesystemTab(), "");
    m_mainTab->addTab(buildPatchesTab(), "");
    m_mainTab->addTab(buildPackTab(), "");
    m_mainTab->addTab(buildFileSearchTab(), "");
    m_mainTab->addTab(buildSettingsTab(), "");
    m_mainTab->addTab(buildAboutTab(), "");

    m_mainTab->setTabText(0, "ROM Info");
    m_mainTab->setTabText(1, "Apps");
    m_mainTab->setTabText(2, "Boot Media");
    m_mainTab->setTabText(3, "DTB Editor");
    m_mainTab->setTabText(4, "Keys");
    m_mainTab->setTabText(5, "Filesystem / Kernel");
    m_mainTab->setTabText(6, "Patches");
    m_mainTab->setTabText(7, "Pack / Build");
    m_mainTab->setTabText(8, "File Search");
    m_mainTab->setTabText(9, "Settings");
    m_mainTab->setTabText(10, "About");

    // ---------------- Sidebar ----------------
    m_sidebarContainer = new QWidget(centralWidget);
    m_sidebarContainer->setObjectName("sidebarContainer");
    m_sidebarContainer->setFixedWidth(215);
    auto *sideLayout = new QVBoxLayout(m_sidebarContainer);
    sideLayout->setContentsMargins(6, 12, 6, 12);
    sideLayout->setSpacing(6);

    m_sidebarList = new QListWidget(m_sidebarContainer);
    m_sidebarList->setObjectName("sidebarList");
    m_sidebarList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_sidebarList->setIconSize(QSize(22, 22));
    m_sidebarList->setFocusPolicy(Qt::NoFocus);
    for (int i = 0; i < m_mainTab->count(); ++i) {
        auto *item = new QListWidgetItem(m_mainTab->tabText(i), m_sidebarList);
        item->setSizeHint(QSize(175, 44));
    }
    sideLayout->addWidget(m_sidebarList, 1);

    m_sidebarVersion = new QLabel(QString("Android ADB Toolbox \u00b7 v%1").arg(m_appVersion), m_sidebarContainer);
    m_sidebarVersion->setAlignment(Qt::AlignCenter);
    m_sidebarVersion->setStyleSheet("color: #94A3B8; font-size: 11px; padding: 6px;");
    sideLayout->addWidget(m_sidebarVersion);

    connect(m_sidebarList, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row >= 0 && row < m_mainTab->count() && m_mainTab->currentIndex() != row)
            m_mainTab->setCurrentIndex(row);
    });
    connect(m_mainTab, &QTabWidget::currentChanged, this, [this](int idx) {
        if (m_sidebarList->currentRow() != idx) m_sidebarList->setCurrentRow(idx);
    });

    updateSidebarIcons();

    auto *body = new QWidget(centralWidget);
    auto *bodyLayout = new QHBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);
    bodyLayout->addWidget(m_sidebarContainer);
    bodyLayout->addWidget(m_mainTab, 1);
    mainLayout->addWidget(body, 1);

    setCentralWidget(centralWidget);
    m_sidebarList->setCurrentRow(0);
    m_uiReady = true;
}

QWidget* MainWindow::buildSettingsTab()
{
    auto *page = new QWidget;
    auto *scroll = new QScrollArea(page);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("QScrollArea { background: transparent; } QScrollArea > QWidget > QWidget { background: transparent; }");

    auto *inner = new QWidget;
    auto *lay = new QVBoxLayout(inner);
    lay->setContentsMargins(18, 18, 18, 18);
    lay->setSpacing(18);

    // ---- Appearance & language ----
    m_setAppearanceGroup = new QGroupBox(tr("Appearance && Language"), inner);
    auto *form = new QFormLayout(m_setAppearanceGroup);
    form->setHorizontalSpacing(18);
    form->setVerticalSpacing(14);
    form->setContentsMargins(18, 22, 18, 16);

    m_themeCombo = new QComboBox(m_setAppearanceGroup);
    for (int i = 0; i <= 5; ++i)
        m_themeCombo->addItem(MaterialTheme::themeName(static_cast<MaterialTheme::ThemeType>(i)));
    m_themeCombo->setCurrentIndex(m_themeNum);
    m_themeCombo->setMinimumWidth(280);
    m_themeCombo->setMaximumWidth(420);
    connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int i) {
        // apply on the next event-loop turn so the combo popup is fully closed first
        QTimer::singleShot(0, this, [this, i]() { applyAppTheme(i); });
    });

    m_langCombo = new QComboBox(m_setAppearanceGroup);
    m_langCombo->addItem(QString::fromUtf8("English"), "en");
    m_langCombo->addItem(QString::fromUtf8("简体中文 (Simplified Chinese)"), "zh_CN");
    m_langCombo->addItem(QString::fromUtf8("繁體中文 (Traditional Chinese)"), "zh_TW");
    m_langCombo->addItem(QString::fromUtf8("日本語 (Japanese)"), "ja_JP");
    m_langCombo->addItem(QString::fromUtf8("Русский (Russian)"), "ru_RU");
    m_langCombo->setMinimumWidth(280);
    m_langCombo->setMaximumWidth(420);
    {
        int idx = m_langCombo->findData(m_currentLang.isEmpty() ? QString("en") : m_currentLang);
        if (idx >= 0) m_langCombo->setCurrentIndex(idx);
    }
    connect(m_langCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int i) {
        setLanguage(m_langCombo->itemData(i).toString());
    });

    m_setThemeCap = new QLabel(tr("Color theme:"), m_setAppearanceGroup);
    m_setLangCap  = new QLabel(tr("Language:"), m_setAppearanceGroup);
    form->addRow(m_setThemeCap, m_themeCombo);
    form->addRow(m_setLangCap, m_langCombo);
    lay->addWidget(m_setAppearanceGroup);

    // ---- Developer tool settings (two-way bound to the existing QActions) ----
    m_setDevGroup = new QGroupBox(tr("Developer Tool Settings"), inner);
    auto *devGrid = new QGridLayout(m_setDevGroup);
    devGrid->setHorizontalSpacing(24);
    devGrid->setVerticalSpacing(14);
    devGrid->setContentsMargins(18, 22, 18, 16);
    const QList<QAction*> devActs = {
        m_actPythonSuperUtils, m_actNewDtbTools, m_actNewLogoTools, m_actPythonImgExtractor,
        m_actArchMakeExt4fs, m_actPythonSimg2img, m_actUseLegacyAWTools, m_actUseNewRKTools};
    int n = 0;
    for (QAction *act : devActs) {
        auto *cb = new QCheckBox(m_setDevGroup);
        auto plain = [act]() { return QString(act->text()).remove('&'); };
        cb->setText(plain());
        cb->setChecked(act->isChecked());
        connect(cb, &QCheckBox::toggled, act, &QAction::setChecked);
        connect(act, &QAction::toggled, cb, &QCheckBox::setChecked);
        connect(act, &QAction::changed, cb, [cb, plain]() { cb->setText(plain()); });
        devGrid->addWidget(cb, n / 2, n % 2);
        ++n;
    }
    lay->addWidget(m_setDevGroup);

    // ---- Tools (replaces the old Tools / Help menus) ----
    m_setToolsGroup = new QGroupBox(tr("Tools"), inner);
    auto *toolsGrid = new QGridLayout(m_setToolsGroup);
    toolsGrid->setHorizontalSpacing(12);
    toolsGrid->setVerticalSpacing(12);
    toolsGrid->setContentsMargins(18, 22, 18, 16);
    const QList<QAction*> toolActs = {m_actCheckRes, m_actOpenWorking, m_actOpenLogs, m_actTutorial};
    n = 0;
    for (QAction *act : toolActs) {
        auto *btn = new QPushButton(QString(act->text()).remove('&'), m_setToolsGroup);
        btn->setCursor(Qt::PointingHandCursor);
        connect(btn, &QPushButton::clicked, act, &QAction::trigger);
        connect(act, &QAction::changed, btn, [btn, act]() { btn->setText(QString(act->text()).remove('&')); });
        toolsGrid->addWidget(btn, n / 3, n % 3);
        ++n;
    }
    lay->addWidget(m_setToolsGroup);
    lay->addStretch();

    scroll->setWidget(inner);
    auto *pageLay = new QVBoxLayout(page);
    pageLay->setContentsMargins(0, 0, 0, 0);
    pageLay->addWidget(scroll);
    return page;
}

QWidget* MainWindow::buildAboutTab()
{
    auto *page = new QWidget;
    auto *scroll = new QScrollArea(page);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("QScrollArea { background: transparent; } QScrollArea > QWidget > QWidget { background: transparent; }");

    auto *inner = new QWidget;
    auto *lay = new QVBoxLayout(inner);
    lay->setContentsMargins(24, 22, 24, 22);
    lay->setSpacing(18);

    m_aboutTitle = new QLabel("ROM Customization Toolkit", inner);
    m_aboutTitle->setStyleSheet("font-size: 26px; font-weight: 700;");
    lay->addWidget(m_aboutTitle);

    m_aboutDesc = new QLabel(inner);
    m_aboutDesc->setWordWrap(true);
    lay->addWidget(m_aboutDesc);

    m_aboutGroup = new QGroupBox(tr("About"), inner);
    auto *form = new QFormLayout(m_aboutGroup);
    form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    form->setHorizontalSpacing(24);
    form->setVerticalSpacing(12);
    form->setContentsMargins(18, 22, 18, 16);

    m_aboutVersionCap = new QLabel(m_aboutGroup);
    m_aboutVersion = new QLabel(m_aboutGroup);
    m_aboutLicenseCap = new QLabel(m_aboutGroup);
    m_aboutLicense = new QLabel(m_aboutGroup);
    m_aboutSourceCap = new QLabel(m_aboutGroup);
    m_aboutSource = new QLabel(m_aboutGroup);
    m_aboutSource->setTextFormat(Qt::RichText);
    m_aboutSource->setOpenExternalLinks(true);
    m_aboutSource->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_aboutSource->setCursor(Qt::PointingHandCursor);
    for (QLabel *l : {m_aboutVersion, m_aboutLicense}) {
        l->setTextInteractionFlags(Qt::TextSelectableByMouse);
        l->setStyleSheet("font-weight: 600;");
    }
    m_aboutSource->setStyleSheet("font-weight: 600;");
    form->addRow(m_aboutVersionCap, m_aboutVersion);
    form->addRow(m_aboutLicenseCap, m_aboutLicense);
    form->addRow(m_aboutSourceCap, m_aboutSource);
    lay->addWidget(m_aboutGroup);

    m_aboutLicenseGroup = new QGroupBox(tr("License"), inner);
    auto *licLay = new QVBoxLayout(m_aboutLicenseGroup);
    licLay->setContentsMargins(18, 22, 18, 16);
    m_aboutLicenseText = new QLabel(m_aboutLicenseGroup);
    m_aboutLicenseText->setWordWrap(true);
    m_aboutLicenseText->setTextInteractionFlags(Qt::TextSelectableByMouse);
    licLay->addWidget(m_aboutLicenseText);

    lay->addWidget(m_aboutLicenseGroup);
    lay->addStretch();

    scroll->setWidget(inner);
    auto *pageLay = new QVBoxLayout(page);
    pageLay->setContentsMargins(0, 0, 0, 0);
    pageLay->addWidget(scroll);

    retranslateExtraPages();   // fill in the texts
    return page;
}

void MainWindow::retranslateExtraPages()
{
    if (m_setAppearanceGroup) m_setAppearanceGroup->setTitle(tr("Appearance && Language"));
    if (m_setThemeCap) m_setThemeCap->setText(tr("Color theme:"));
    if (m_setLangCap)  m_setLangCap->setText(tr("Language:"));
    if (m_setDevGroup)   m_setDevGroup->setTitle(tr("Developer Tool Settings"));
    if (m_setToolsGroup) m_setToolsGroup->setTitle(tr("Tools"));

    if (m_aboutGroup) m_aboutGroup->setTitle(tr("About"));
    if (m_aboutDesc) m_aboutDesc->setText(tr("Android firmware customization tool for Amlogic, Rockchip, and Allwinner TV boxes."));
    if (m_aboutVersionCap) m_aboutVersionCap->setText(tr("Version:"));
    if (m_aboutVersion)    m_aboutVersion->setText(tr("%1 (Linux Qt Port)").arg(m_appVersion));
    if (m_aboutLicenseCap) m_aboutLicenseCap->setText(tr("License:"));
    if (m_aboutLicense)    m_aboutLicense->setText(tr("GPL / Open Source"));
    if (m_aboutLicenseGroup) m_aboutLicenseGroup->setTitle(tr("License"));
    if (m_aboutLicenseText)
        m_aboutLicenseText->setText(tr(
            "Unpacking and packing rely on third-party tools (7z, dtc, e2fsprogs, simg2img, lpunpack and others). "
            "Each of those tools is distributed under its own license."));
    if (m_aboutSourceCap) m_aboutSourceCap->setText(tr("Source:"));
    if (m_aboutSource)
        m_aboutSource->setText(QStringLiteral(
            "<a href=\"https://github.com/dismal002/ROMCustomizationToolkit\">"
            "github.com/dismal002/ROMCustomizationToolkit</a>"));
}

void MainWindow::applyAppTheme(int themeNum)
{
    if (themeNum < 0 || themeNum > 5) themeNum = 0;
    m_themeNum = themeNum;
    QSettings("ROM Customization Toolkit", "Settings").setValue("Theme", themeNum);
    MaterialTheme::applyTheme(static_cast<MaterialTheme::ThemeType>(themeNum));
    updateSidebarIcons();
}

void MainWindow::updateSidebarIcons()
{
    const auto p = MaterialTheme::getPalette(static_cast<MaterialTheme::ThemeType>(m_themeNum));
    const char *icons[] = {"home", "apps", "image", "edit", "key", "terminal", "patches", "build", "search", "settings", "info"};
    for (int i = 0; i < m_sidebarList->count() && i < 11; ++i) {
        m_sidebarList->item(i)->setIcon(IconHelper::createIconFromSvg(
            QString(":/icons/%1.svg").arg(icons[i]), p.sidebarText, p.sidebarSelectedText, 22));
    }
}

void MainWindow::updateSidebarTexts()
{
    if (!m_sidebarList || !m_mainTab) return;
    for (int i = 0; i < m_sidebarList->count() && i < m_mainTab->count(); ++i)
        m_sidebarList->item(i)->setText(m_mainTab->tabText(i));
}

void MainWindow::setupMenuBar()
{

    // File menu
    m_menuFile = new QMenu(this);
    m_actOpen = m_menuFile->addAction("");
    connect(m_actOpen, &QAction::triggered, this, &MainWindow::onUnpackClicked);
    m_menuFile->addSeparator();
    m_actExit = m_menuFile->addAction("");
    connect(m_actExit, &QAction::triggered, this, &QWidget::close);

    // Tools menu
    m_menuTools = new QMenu(this);
    m_actCheckRes = m_menuTools->addAction("");
    connect(m_actCheckRes, &QAction::triggered, this, &MainWindow::checkResources);
    m_actOpenWorking = m_menuTools->addAction("");
    connect(m_actOpenWorking, &QAction::triggered, this, &MainWindow::onOpenWorkingClicked);
    m_actOpenLogs = m_menuTools->addAction("");
    connect(m_actOpenLogs, &QAction::triggered, this, &MainWindow::onLogsClicked);

    // Dev settings submenu
    m_menuDev = m_menuTools->addMenu("");
    m_actPythonSuperUtils = m_menuDev->addAction("");
    m_actNewDtbTools      = m_menuDev->addAction("");
    m_actNewLogoTools     = m_menuDev->addAction("");
    m_actPythonImgExtractor = m_menuDev->addAction("");
    m_actArchMakeExt4fs   = m_menuDev->addAction("");
    m_actPythonSimg2img   = m_menuDev->addAction("");
    m_actUseLegacyAWTools = m_menuDev->addAction("");
    m_actUseNewRKTools    = m_menuDev->addAction("");

    QList<QAction*> devActs = {
        m_actPythonSuperUtils, m_actNewDtbTools, m_actNewLogoTools,
        m_actPythonImgExtractor, m_actArchMakeExt4fs, m_actPythonSimg2img,
        m_actUseLegacyAWTools, m_actUseNewRKTools
    };
    for (auto *act : devActs) {
        act->setCheckable(true);
        connect(act, &QAction::toggled, this, &MainWindow::onDevSettingsChanged);
    }

    // Language menu
    m_menuLanguage = new QMenu(this);
    auto *langGroup = new QActionGroup(this);
    langGroup->setExclusive(true);

    m_actLangEn   = m_menuLanguage->addAction("English");
    m_actLangZhCn = m_menuLanguage->addAction("简体中文 (Simplified Chinese)");
    m_actLangZhTw = m_menuLanguage->addAction("繁體中文 (Traditional Chinese)");
    m_actLangJa   = m_menuLanguage->addAction("日本語 (Japanese)");
    m_actLangRu   = m_menuLanguage->addAction("Русский (Russian)");

    QList<QAction*> langActions = {m_actLangEn, m_actLangZhCn, m_actLangZhTw, m_actLangJa, m_actLangRu};
    for (auto *act : langActions) {
        act->setCheckable(true);
        langGroup->addAction(act);
    }

    connect(m_actLangEn,   &QAction::triggered, this, [this]() { setLanguage("en"); });
    connect(m_actLangZhCn, &QAction::triggered, this, [this]() { setLanguage("zh_CN"); });
    connect(m_actLangZhTw, &QAction::triggered, this, [this]() { setLanguage("zh_TW"); });
    connect(m_actLangJa,   &QAction::triggered, this, [this]() { setLanguage("ja_JP"); });
    connect(m_actLangRu,   &QAction::triggered, this, [this]() { setLanguage("ru_RU"); });

    // Help menu
    m_menuHelp = new QMenu(this);
    m_actTutorial = m_menuHelp->addAction("");
    connect(m_actTutorial, &QAction::triggered, this, &MainWindow::onTutorialItemClicked);
}

void MainWindow::setupStatusBar()
{
    m_statusLabel = new QLabel;
    m_statusProgress = new QProgressBar;
    m_statusProgress->setFixedWidth(160);
    m_statusProgress->setFixedHeight(14);
    m_statusProgress->setVisible(false);

    m_versionLabel = new QLabel("v" + m_appVersion + " (GPL/OpenSource)");
    m_versionLabel->setStyleSheet("color: #888888; font-size: 11px;");

    statusBar()->addWidget(m_statusLabel, 1);
    statusBar()->addPermanentWidget(m_statusProgress);
    statusBar()->addPermanentWidget(m_versionLabel);
}

void MainWindow::retranslateUi()
{
    if (!m_uiReady) {
        return;
    }

    setWindowTitle(tr("ROM Customization Toolkit – Android Firmware Customizer (Linux)"));

    if (m_btnUnpackImage) m_btnUnpackImage->setText(tr("Unpack Image"));
    if (m_leUpgradePackage) m_leUpgradePackage->setPlaceholderText(tr("No firmware package loaded..."));

    // Menus
    if (m_menuFile) m_menuFile->setTitle(tr("&File"));
    if (m_actOpen)  m_actOpen->setText(tr("&Open / Unpack Image..."));
    if (m_actExit)  m_actExit->setText(tr("E&xit"));

    if (m_menuTools)     m_menuTools->setTitle(tr("&Tools"));
    if (m_actCheckRes)   m_actCheckRes->setText(tr("Check &Tool Resources..."));
    if (m_actOpenWorking) m_actOpenWorking->setText(tr("Open &Working Directory (tmp)"));
    if (m_actOpenLogs)   m_actOpenLogs->setText(tr("Open Application &Logs"));

    if (m_menuDev) m_menuDev->setTitle(tr("&Developer Tool Settings"));
    if (m_actPythonSuperUtils)   m_actPythonSuperUtils->setText(tr("Use Python Super Image Utils"));
    if (m_actNewDtbTools)        m_actNewDtbTools->setText(tr("Use New DTB Processing Tools"));
    if (m_actNewLogoTools)       m_actNewLogoTools->setText(tr("Use New Logo Processing Tools"));
    if (m_actPythonImgExtractor) m_actPythonImgExtractor->setText(tr("Use Python Image Extractor"));
    if (m_actArchMakeExt4fs)     m_actArchMakeExt4fs->setText(tr("Use System make_ext4fs"));
    if (m_actPythonSimg2img)     m_actPythonSimg2img->setText(tr("Use Python simg2img"));
    if (m_actUseLegacyAWTools)   m_actUseLegacyAWTools->setText(tr("Use Legacy Allwinner Tools"));
    if (m_actUseNewRKTools)      m_actUseNewRKTools->setText(tr("Use New Rockchip Tools"));

    if (m_menuLanguage) m_menuLanguage->setTitle(tr("&Language"));

    if (m_menuHelp)       m_menuHelp->setTitle(tr("&Help"));
    if (m_actTutorial)    m_actTutorial->setText(tr("&Tutorials"));

    // Tabs
    if (m_mainTab) {
        m_mainTab->setTabText(0, tr("ROM Info"));
        m_mainTab->setTabText(1, tr("Apps"));
        m_mainTab->setTabText(2, tr("Boot Media"));
        m_mainTab->setTabText(3, tr("DTB Editor"));
        m_mainTab->setTabText(4, tr("Keys"));
        m_mainTab->setTabText(5, tr("Filesystem / Kernel"));
        m_mainTab->setTabText(6, tr("Patches"));
        m_mainTab->setTabText(7, tr("Pack / Build"));
        m_mainTab->setTabText(8, tr("File Search"));
        m_mainTab->setTabText(9, tr("Settings"));
        m_mainTab->setTabText(10, tr("About"));
        updateSidebarTexts();
        retranslateExtraPages();
    }

    // Info Tab
    if (m_propGroup) m_propGroup->setTitle(tr("Firmware Properties"));
    if (m_lblPropVer) m_lblPropVer->setText(tr("Android Version:"));
    if (m_lblPropModel) m_lblPropModel->setText(tr("Model:"));
    if (m_lblPropVendor) m_lblPropVendor->setText(tr("Vendor / Brand:"));
    if (m_lblPropProduct) m_lblPropProduct->setText(tr("Product / Device:"));
    if (m_lblPropSdk) m_lblPropSdk->setText(tr("SDK Version:"));
    if (m_lblPropPatch) m_lblPropPatch->setText(tr("Security Patch:"));
    if (m_lblPropBuild) m_lblPropBuild->setText(tr("Build ID:"));
    if (m_lblPropDispId) m_lblPropDispId->setText(tr("Display ID:"));

    if (m_statsGroup) m_statsGroup->setTitle(tr("Partition Statistics"));
    if (m_lblStatsSysSize) m_lblStatsSysSize->setText(tr("System Size (MB):"));
    if (m_lblStatsSysFiles) m_lblStatsSysFiles->setText(tr("Files:"));
    if (m_lblStatsSysApps) m_lblStatsSysApps->setText(tr("Apps:"));
    if (m_lblStatsVenSize) m_lblStatsVenSize->setText(tr("Vendor Size (MB):"));
    if (m_lblStatsVenFiles) m_lblStatsVenFiles->setText(tr("Files:"));
    if (m_lblStatsVenApps) m_lblStatsVenApps->setText(tr("Apps:"));

    if (m_flagsGroup) m_flagsGroup->setTitle(tr("Package Attributes && Chipset"));
    if (m_cbRamdiskSystem) m_cbRamdiskSystem->setText(tr("Ramdisk System (system/system)"));
    if (m_cbContainsVendor) m_cbContainsVendor->setText(tr("Contains Vendor Partition"));
    if (m_cbMultiDtb) m_cbMultiDtb->setText(tr("Multi DTB Configuration"));
    if (m_cbBootupGunzipped) m_cbBootupGunzipped->setText(tr("Bootup Logo GZ Compressed"));
    if (m_cbSuperPartition) m_cbSuperPartition->setText(tr("Dynamic / Super Partition"));
    if (m_cbCompressBrotli) m_cbCompressBrotli->setText(tr("Brotli (.br) Compression"));
    if (m_cbAddPlatformConfig) m_cbAddPlatformConfig->setText(tr("Add Platform Config (Amlogic)"));
    if (m_cbSpecifyChipset) m_cbSpecifyChipset->setText(tr("Specify Rockchip Chipset:"));
    if (m_lblPkgTypeTitle) m_lblPkgTypeTitle->setText(tr("Package Type:"));
    if (m_lblLocaleTitle) m_lblLocaleTitle->setText(tr("Locale:"));

    // Apps Tab
    if (m_lblAppsDir) m_lblAppsDir->setText(tr("Show apps inside:"));
    if (m_btnAddApps) m_btnAddApps->setText(tr("Add Apps"));
    if (m_btnRemoveApps) m_btnRemoveApps->setText(tr("Remove App"));
    if (m_apkGroupbox) m_apkGroupbox->setTitle(tr("Application Metadata"));
    if (m_btnUnpackApk) m_btnUnpackApk->setText(tr("Unpack APK (Extract)"));
    if (m_btnRepackApk) m_btnRepackApk->setText(tr("Repack APK"));

    // Boot Media Tab
    if (m_mediaTabs) {
        m_mediaTabs->setTabText(0, tr("Boot Logo"));
        m_mediaTabs->setTabText(1, tr("Boot Animation"));
        m_mediaTabs->setTabText(2, tr("Boot Video"));
    }
    if (m_btnBootLogo) m_btnBootLogo->setText(tr("Change Boot Logo (.bmp / .png)"));
    if (m_btnRefreshLogo) m_btnRefreshLogo->setText(tr("Refresh Logo"));
    if (m_btnOpenLogos) m_btnOpenLogos->setText(tr("Open Logos Folder"));
    if (m_btnPlayAnim) m_btnPlayAnim->setText(tr("Preview Animation"));
    if (m_btnChangeAnim) m_btnChangeAnim->setText(tr("Change bootanimation.zip"));
    if (m_btnPlayVideo) m_btnPlayVideo->setText(tr("Play / Pause Boot Video"));
    if (m_btnChangeVideo) m_btnChangeVideo->setText(tr("Change Boot Video (.ts / .mp4)"));

    // DTB Tab
    if (m_lblDtsFile) m_lblDtsFile->setText(tr("DTS File:"));
    if (m_leSearchBox) m_leSearchBox->setPlaceholderText(tr("Search inside DTS..."));
    if (m_btnDtbBack) m_btnDtbBack->setText(tr("Prev"));
    if (m_btnDtbNext) m_btnDtbNext->setText(tr("Next"));
    if (m_btnDtbSave) m_btnDtbSave->setText(tr("Save DTB Changes"));
    if (m_btnDtbDiscard) m_btnDtbDiscard->setText(tr("Discard Changes"));

    // Keys Tab
    if (m_lblKeysInfo) m_lblKeysInfo->setText(tr("Amlogic Keys Configuration: Select key files to include in burn tool image."));
    if (m_btnWriteKeys) m_btnWriteKeys->setText(tr("Write keys.conf"));
    if (m_btnOpenWorkingKeys) m_btnOpenWorkingKeys->setText(tr("Open Working Dir"));

    // Filesystem Tab
    if (m_systemGroupBox) m_systemGroupBox->setTitle(tr("System Partition Operations"));
    if (m_btnOpenSys) m_btnOpenSys->setText(tr("Open System Directory"));
    if (m_btnSysProp) m_btnSysProp->setText(tr("Open system/build.prop"));
    if (m_btnSysKeymap) m_btnSysKeymap->setText(tr("Open system Keymap (.kl)"));
    if (m_btnSysGenKl) m_btnSysGenKl->setText(tr("Open system Generic.kl"));
    if (m_btnSysRemConf) m_btnSysRemConf->setText(tr("Open system remote.conf"));

    if (m_vendorGroupBox) m_vendorGroupBox->setTitle(tr("Vendor Partition Operations"));
    if (m_btnOpenVen) m_btnOpenVen->setText(tr("Open Vendor Directory"));
    if (m_btnVenProp) m_btnVenProp->setText(tr("Open vendor/build.prop"));
    if (m_btnVenKeymap) m_btnVenKeymap->setText(tr("Open vendor Keymap (.kl)"));
    if (m_btnVenGenKl) m_btnVenGenKl->setText(tr("Open vendor Generic.kl"));
    if (m_btnVenRemConf) m_btnVenRemConf->setText(tr("Open vendor remote.conf"));

    if (m_kernelGroup) m_kernelGroup->setTitle(tr("Kernel / Boot && Recovery"));
    if (m_btnOpenKern) m_btnOpenKern->setText(tr("Open Kernel Directory"));
    if (m_btnOpenRec) m_btnOpenRec->setText(tr("Open Recovery Directory"));
    if (m_btnReplKern) m_btnReplKern->setText(tr("Replace Kernel (boot.img)"));
    if (m_btnReplRec) m_btnReplRec->setText(tr("Replace Recovery (recovery.img)"));
    if (m_btnMagiskBoot) m_btnMagiskBoot->setText(tr("Prepare boot.img for Magisk Rooting"));

    if (m_wifiGroupBox) m_wifiGroupBox->setTitle(tr("Rooting && Hardware Drivers"));
    if (m_btnRoot) m_btnRoot->setText(tr("Root Firmware (SuperSU / Magisk SU)"));
    if (m_btnUnroot) m_btnUnroot->setText(tr("Remove Root"));
    if (m_btnWifi) m_btnWifi->setText(tr("Multi WiFi Drivers Injector"));

    // Patches Tab
    if (m_lblPatches) m_lblPatches->setText(tr("Available Patches:"));
    if (m_btnApplyPatch) m_btnApplyPatch->setText(tr("Apply Patch"));
    if (m_lblAuthorTitle) m_lblAuthorTitle->setText(tr("Author:"));
    if (m_lblVerMetaTitle) m_lblVerMetaTitle->setText(tr("Version:"));
    if (m_lblDescTitle) m_lblDescTitle->setText(tr("Description:"));
    if (m_lblPatchExec) m_lblPatchExec->setText(tr("Execution Log:"));
    if (m_btnOpenPatches) m_btnOpenPatches->setText(tr("Open Patches Directory"));

    // Pack Tab
    if (m_imgGroup) m_imgGroup->setTitle(tr("Repack Firmware to Image (Burn Tool / USB Tool)"));
    if (m_btnRepackImg) m_btnRepackImg->setText(tr("Repack to Image (.img)"));
    if (m_zipGroup) m_zipGroup->setTitle(tr("Repack Firmware to OTA / Upgrade Zip"));
    if (m_lblCompSlider) m_lblCompSlider->setText(tr("Compression Level:"));
    if (m_btnZipRom) m_btnZipRom->setText(tr("Zip ROM (.zip)"));
    if (m_ctpGroup) m_ctpGroup->setTitle(tr("Export ROM Customization Toolkit Project (.ctp)"));
    if (m_lblCtpSlider) m_lblCtpSlider->setText(tr("CTP Compression:"));
    if (m_btnCtp) m_btnCtp->setText(tr("Export Project (.ctp)"));

    // File Search Tab
    if (m_lblSearchFile) m_lblSearchFile->setText(tr("Search Files in ROM:"));
    if (m_leSearchFile) m_leSearchFile->setPlaceholderText(tr("Type filename or pattern to search in level2/..."));

    if (m_statusLabel && m_statusLabel->text().isEmpty()) {
        m_statusLabel->setText(tr("Ready"));
    }
}

// -------------------------------------------------------------
// Tab Builders
// -------------------------------------------------------------
QWidget* MainWindow::buildInfoTab()
{
    // Colours/cards come from MaterialTheme (global stylesheet); only weight is set here.
    const QString valueStyle = "font-weight: 600;";

    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(18);

    // Caption + value pair placed in one grid cell. Both widgets are stored in the
    // member pointers passed in, so loadInfo()/retranslateUi() always have valid objects.
    auto addPair = [&](QWidget *parent, QGridLayout *grid, int row, int col, int colSpan,
                       QLabel *&caption, QLabel *&value, const QString &captionText) {
        auto *cell = new QWidget(parent);
        auto *cl = new QHBoxLayout(cell);
        cl->setContentsMargins(0, 0, 0, 0);
        cl->setSpacing(8);
        caption = new QLabel(captionText, cell);
        value = new QLabel(cell);
        value->setStyleSheet(valueStyle);
        value->setTextInteractionFlags(Qt::TextSelectableByMouse);
        cl->addWidget(caption);
        cl->addWidget(value, 1);
        grid->addWidget(cell, row, col, 1, colSpan);
    };

    auto *content = new QWidget(widget);
    auto *contentLayout = new QHBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(18);

    // ---- Firmware Properties ----
    m_propGroup = new QGroupBox(tr("Firmware Properties"), content);
    auto *propGrid = new QGridLayout(m_propGroup);
    propGrid->setHorizontalSpacing(26);
    propGrid->setVerticalSpacing(14);
    addPair(m_propGroup, propGrid, 0, 0, 1, m_lblPropVer,     m_leVersion,        "Android Version:");
    addPair(m_propGroup, propGrid, 0, 1, 1, m_lblPropModel,   m_leModel,          "Model:");
    addPair(m_propGroup, propGrid, 1, 0, 1, m_lblPropVendor,  m_leVendor,         "Vendor / Brand:");
    addPair(m_propGroup, propGrid, 1, 1, 1, m_lblPropProduct, m_leProduct,        "Product / Device:");
    addPair(m_propGroup, propGrid, 2, 0, 1, m_lblPropSdk,     m_leSDK,            "SDK Version:");
    addPair(m_propGroup, propGrid, 2, 1, 1, m_lblPropPatch,   m_lePatch,          "Security Patch:");
    addPair(m_propGroup, propGrid, 3, 0, 1, m_lblPropBuild,   m_leBuild,          "Build ID:");
    addPair(m_propGroup, propGrid, 4, 0, 2, m_lblPropDispId,  m_leBuildDisplayId, "Display ID:");
    propGrid->setRowStretch(5, 1);

    // ---- Partition Statistics ----
    m_statsGroup = new QGroupBox(tr("Partition Statistics"), content);
    auto *statsGrid = new QGridLayout(m_statsGroup);
    statsGrid->setHorizontalSpacing(18);
    statsGrid->setVerticalSpacing(12);
    addPair(m_statsGroup, statsGrid, 0, 0, 1, m_lblStatsSysSize,  m_leSystemSize,  "System Size (MB):");
    addPair(m_statsGroup, statsGrid, 0, 1, 1, m_lblStatsSysFiles, m_leSystemFiles, "Files:");
    addPair(m_statsGroup, statsGrid, 0, 2, 1, m_lblStatsSysApps,  m_leSystemApps,  "Apps:");
    addPair(m_statsGroup, statsGrid, 1, 0, 1, m_lblStatsVenSize,  m_leVendorSize,  "Vendor Size (MB):");
    addPair(m_statsGroup, statsGrid, 1, 1, 1, m_lblStatsVenFiles, m_leVendorFiles, "Files:");
    addPair(m_statsGroup, statsGrid, 1, 2, 1, m_lblStatsVenApps,  m_leVendorApps,  "Apps:");
    statsGrid->setRowStretch(2, 1);

    contentLayout->addWidget(m_propGroup, 1);
    contentLayout->addWidget(m_statsGroup, 1);
    layout->addWidget(content);

    // ---- Package Attributes & Chipset ----
    m_flagsGroup = new QGroupBox(tr("Package Attributes && Chipset"), widget);
    auto *flagsLayout = new QVBoxLayout(m_flagsGroup);
    flagsLayout->setSpacing(14);

    auto *checkboxGrid = new QGridLayout;
    checkboxGrid->setHorizontalSpacing(22);
    checkboxGrid->setVerticalSpacing(16);

    m_cbRamdiskSystem   = new QCheckBox("Ramdisk System (system/system)", m_flagsGroup);
    m_cbContainsVendor  = new QCheckBox("Contains Vendor Partition", m_flagsGroup);
    m_cbMultiDtb        = new QCheckBox("Multi DTB Configuration", m_flagsGroup);
    m_cbBootupGunzipped = new QCheckBox("Bootup Logo GZ Compressed", m_flagsGroup);
    m_cbSuperPartition  = new QCheckBox("Dynamic / Super Partition", m_flagsGroup);
    m_cbCompressBrotli  = new QCheckBox("Brotli (.br) Compression", m_flagsGroup);
    m_cbAddPlatformConfig = new QCheckBox("Add Platform Config (Amlogic)", m_flagsGroup);
    m_cbSpecifyChipset  = new QCheckBox("Specify Rockchip Chipset:", m_flagsGroup);

    checkboxGrid->addWidget(m_cbRamdiskSystem,     0, 0);
    checkboxGrid->addWidget(m_cbContainsVendor,    0, 1);
    checkboxGrid->addWidget(m_cbMultiDtb,          0, 2);
    checkboxGrid->addWidget(m_cbBootupGunzipped,   1, 0);
    checkboxGrid->addWidget(m_cbSuperPartition,    1, 1);
    checkboxGrid->addWidget(m_cbCompressBrotli,    1, 2);
    checkboxGrid->addWidget(m_cbAddPlatformConfig, 2, 0);
    checkboxGrid->addWidget(m_cbSpecifyChipset,    2, 1);

    m_leRKChipset = new QLineEdit(m_flagsGroup);
    m_leRKChipset->setPlaceholderText("Chipset");
    m_leRKChipset->setMaximumWidth(180);
        checkboxGrid->addWidget(m_leRKChipset, 2, 2);
    flagsLayout->addLayout(checkboxGrid);

    auto *metaRow = new QHBoxLayout;
    metaRow->setSpacing(10);
    m_lblPkgTypeTitle = new QLabel("Package Type:", m_flagsGroup);
    m_lblPackageType = new QLabel("None", m_flagsGroup);
    m_lblPackageType->setStyleSheet(valueStyle);
    m_lblLocaleTitle = new QLabel("Locale:", m_flagsGroup);
    m_cbLocales = new QComboBox(m_flagsGroup);
    m_cbLocales->setMinimumWidth(140);
    metaRow->addWidget(m_lblPkgTypeTitle);
    metaRow->addWidget(m_lblPackageType);
    metaRow->addSpacing(24);
    metaRow->addWidget(m_lblLocaleTitle);
    metaRow->addWidget(m_cbLocales);
    metaRow->addStretch();
    flagsLayout->addLayout(metaRow);

    layout->addWidget(m_flagsGroup);
    layout->addStretch();


    return widget;
}

QWidget* MainWindow::buildAppsTab()
{
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);

    auto *topLayout = new QHBoxLayout;
    m_lblAppsDir = new QLabel;
    topLayout->addWidget(m_lblAppsDir);
    m_appsDirectory = new QComboBox;
    connect(m_appsDirectory, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onAppsDirectoryChanged);
    topLayout->addWidget(m_appsDirectory, 1);

    m_btnAddApps = new QPushButton;
    m_btnRemoveApps = new QPushButton;
    connect(m_btnAddApps, &QPushButton::clicked, this, &MainWindow::onAddAppsClicked);
    connect(m_btnRemoveApps, &QPushButton::clicked, this, &MainWindow::onRemoveAppsClicked);
    topLayout->addWidget(m_btnAddApps);
    topLayout->addWidget(m_btnRemoveApps);

    layout->addLayout(topLayout);

    auto *split = new QSplitter(Qt::Horizontal);
    m_appsViewer = new QListWidget;
    m_appsViewer->setObjectName("appsViewer");
    // Icon grid, like the original tool: one tile per app with its name underneath
    m_appsViewer->setViewMode(QListView::IconMode);
    m_appsViewer->setIconSize(QSize(64, 64));
    m_appsViewer->setGridSize(QSize(132, 120));
    m_appsViewer->setResizeMode(QListView::Adjust);
    m_appsViewer->setMovement(QListView::Static);
    // No wrapping: wrapped labels change the tile height when the row is selected/narrow
    m_appsViewer->setWordWrap(false);
    m_appsViewer->setUniformItemSizes(true);
    m_appsViewer->setSpacing(6);
    m_appsViewer->setMinimumWidth(380);
    m_appsViewer->setSelectionMode(QAbstractItemView::SingleSelection);
    m_appsViewer->setTextElideMode(Qt::ElideRight);
    connect(m_appsViewer, &QListWidget::itemSelectionChanged, this, &MainWindow::onAppSelectionChanged);
    split->addWidget(m_appsViewer);

    // App Info Group
    m_apkGroupbox = new QGroupBox;
    auto *apkLayout = new QVBoxLayout(m_apkGroupbox);

    m_lblAppPackage           = new QLabel("Package: -");
    m_lblAppLauncherApp       = new QLabel("Is Launcher: -");
    m_lblAppLauncherApp->setWordWrap(true);
    m_lblAppVisibleLauncher   = new QLabel("Visible on Standard Launcher: -");
    m_lblAppVisibleTvLauncher = new QLabel("Visible on TV Launcher: -");

    apkLayout->addWidget(m_lblAppPackage);
    apkLayout->addWidget(m_lblAppLauncherApp);
    apkLayout->addWidget(m_lblAppVisibleLauncher);
    apkLayout->addWidget(m_lblAppVisibleTvLauncher);
    apkLayout->addSpacing(16);

    m_btnUnpackApk = new QPushButton;
    m_btnRepackApk = new QPushButton;
    connect(m_btnUnpackApk, &QPushButton::clicked, this, &MainWindow::onUnpackApkClicked);
    connect(m_btnRepackApk, &QPushButton::clicked, this, &MainWindow::onRepackApkClicked);
    apkLayout->addWidget(m_btnUnpackApk);
    apkLayout->addWidget(m_btnRepackApk);
    apkLayout->addStretch();

    m_apkGroupbox->setMaximumWidth(360);
    split->addWidget(m_apkGroupbox);
    split->setSizes({600, 300});
    split->setStretchFactor(0, 1);
    split->setStretchFactor(1, 0);
    layout->addWidget(split);

    return widget;
}

QWidget* MainWindow::buildBootMediaTab()
{
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);

    m_mediaTabs = new QTabWidget;

    // 1. Boot Logo
    m_bootlogoPanel = new QWidget;
    auto *logoLayout = new QVBoxLayout(m_bootlogoPanel);
    m_bootlogoPicture = new QLabel;
    m_bootlogoPicture->setAlignment(Qt::AlignCenter);
    m_bootlogoPicture->setMinimumSize(400, 240);
    m_bootlogoPicture->setStyleSheet("border: 1px solid #444; background: #222;");
    logoLayout->addWidget(m_bootlogoPicture, 1);

    auto *logoBtnLayout = new QHBoxLayout;
    m_btnBootLogo = new QPushButton;
    m_btnRefreshLogo = new QPushButton;
    m_btnOpenLogos = new QPushButton;
    connect(m_btnBootLogo, &QPushButton::clicked, this, &MainWindow::onBootLogoButtonClicked);
    connect(m_btnRefreshLogo, &QPushButton::clicked, this, &MainWindow::onRefreshLogoClicked);
    connect(m_btnOpenLogos, &QPushButton::clicked, this, &MainWindow::onOpenLogosClicked);
    logoBtnLayout->addWidget(m_btnBootLogo);
    logoBtnLayout->addWidget(m_btnRefreshLogo);
    logoBtnLayout->addWidget(m_btnOpenLogos);
    logoLayout->addLayout(logoBtnLayout);

    m_mediaTabs->addTab(m_bootlogoPanel, "");

    // 2. Boot Animation
    m_bootanimPanel = new QWidget;
    auto *animLayout = new QVBoxLayout(m_bootanimPanel);
    m_bootanimPicture = new QLabel;
    m_bootanimPicture->setAlignment(Qt::AlignCenter);
    m_bootanimPicture->setMinimumSize(400, 240);
    m_bootanimPicture->setStyleSheet("border: 1px solid #444; background: #222;");
    animLayout->addWidget(m_bootanimPicture, 1);

    m_lblBootanimFPS = new QLabel("FPS: -");
    animLayout->addWidget(m_lblBootanimFPS);

    auto *animBtnLayout = new QHBoxLayout;
    m_btnPlayAnim = new QPushButton;
    m_btnChangeAnim = new QPushButton;
    connect(m_btnPlayAnim, &QPushButton::clicked, this, &MainWindow::onPlayBootanimationClicked);
    connect(m_btnChangeAnim, &QPushButton::clicked, this, &MainWindow::onBootanimationChangeClicked);
    animBtnLayout->addWidget(m_btnPlayAnim);
    animBtnLayout->addWidget(m_btnChangeAnim);
    animLayout->addLayout(animBtnLayout);

    m_mediaTabs->addTab(m_bootanimPanel, "");

    // 3. Boot Video
    m_bootvideoPanel = new QWidget;
    auto *videoLayout = new QVBoxLayout(m_bootvideoPanel);
    m_videoPlayer->setVideoOutput(m_videoWidget);
    m_videoWidget->setMinimumSize(400, 240);
    videoLayout->addWidget(m_videoWidget, 1);

    auto *videoBtnLayout = new QHBoxLayout;
    m_btnPlayVideo = new QPushButton;
    m_btnChangeVideo = new QPushButton;
    connect(m_btnPlayVideo, &QPushButton::clicked, this, &MainWindow::onPlayBootvideoClicked);
    connect(m_btnChangeVideo, &QPushButton::clicked, this, &MainWindow::onChangeBootvideoClicked);
    videoBtnLayout->addWidget(m_btnPlayVideo);
    videoBtnLayout->addWidget(m_btnChangeVideo);
    videoLayout->addLayout(videoBtnLayout);

    m_mediaTabs->addTab(m_bootvideoPanel, "");

    layout->addWidget(m_mediaTabs);
    return widget;
}

QWidget* MainWindow::buildDtbTab()
{
    m_dtbPanel = new QWidget;
    auto *layout = new QVBoxLayout(m_dtbPanel);

    auto *topLayout = new QHBoxLayout;
    m_lblDtsFile = new QLabel;
    topLayout->addWidget(m_lblDtsFile);
    m_dtbCombo = new QComboBox;
    connect(m_dtbCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onDtbComboChanged);
    topLayout->addWidget(m_dtbCombo, 1);

    m_leSearchBox = new QLineEdit;
    topLayout->addWidget(m_leSearchBox);

    m_btnDtbNext = new QPushButton;
    m_btnDtbBack = new QPushButton;
    connect(m_btnDtbNext, &QPushButton::clicked, this, &MainWindow::onSearchNextClicked);
    connect(m_btnDtbBack, &QPushButton::clicked, this, &MainWindow::onSearchBackClicked);
    topLayout->addWidget(m_btnDtbBack);
    topLayout->addWidget(m_btnDtbNext);

    layout->addLayout(topLayout);

    m_dtbEditor = new QTextEdit;
    m_dtbEditor->setFontFamily("Monospace");
    m_dtbEditor->setStyleSheet("background: #1e1e1e; color: #d4d4d4; font-size: 13px;");
    layout->addWidget(m_dtbEditor, 1);

    auto *btnLayout = new QHBoxLayout;
    m_btnDtbSave = new QPushButton;
    m_btnDtbDiscard = new QPushButton;
    connect(m_btnDtbSave, &QPushButton::clicked, this, &MainWindow::onSaveDtbClicked);
    connect(m_btnDtbDiscard, &QPushButton::clicked, this, &MainWindow::onDiscardDtbClicked);
    btnLayout->addWidget(m_btnDtbSave);
    btnLayout->addWidget(m_btnDtbDiscard);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    return m_dtbPanel;
}

QWidget* MainWindow::buildKeysTab()
{
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);

    m_lblKeysInfo = new QLabel;
    layout->addWidget(m_lblKeysInfo);

    m_keysChecklist = new QListWidget;
    QStringList defaultKeys = {
        "mac", "mac_bt", "mac_wifi", "usid", "deviceid", "oemkey",
        "hdcp", "hdcp2", "widevine", "playready", "attestation", "rsa"
    };
    for (const auto &k : defaultKeys) {
        auto *item = new QListWidgetItem(k, m_keysChecklist);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
    }
    layout->addWidget(m_keysChecklist, 1);

    auto *btnLayout = new QHBoxLayout;
    m_btnWriteKeys = new QPushButton;
    m_btnOpenWorkingKeys = new QPushButton;
    connect(m_btnWriteKeys, &QPushButton::clicked, this, &MainWindow::onWriteKeysClicked);
    connect(m_btnOpenWorkingKeys, &QPushButton::clicked, this, &MainWindow::onOpenWorkingClicked);
    btnLayout->addWidget(m_btnWriteKeys);
    btnLayout->addWidget(m_btnOpenWorkingKeys);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    return widget;
}

QWidget* MainWindow::buildFilesystemTab()
{
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);

    auto *grid = new QGridLayout;

    // System group
    m_systemGroupBox = new QGroupBox;
    auto *sysLayout = new QVBoxLayout(m_systemGroupBox);
    m_btnOpenSys = new QPushButton;
    m_btnSysProp = new QPushButton;
    m_btnSysKeymap = new QPushButton;
    m_btnSysGenKl = new QPushButton;
    m_btnSysRemConf = new QPushButton;

    connect(m_btnOpenSys, &QPushButton::clicked, this, &MainWindow::onOpenSystemClicked);
    connect(m_btnSysProp, &QPushButton::clicked, this, &MainWindow::onOpenSystemBuildPropClicked);
    connect(m_btnSysKeymap, &QPushButton::clicked, this, &MainWindow::onOpenSystemKeymapClicked);
    connect(m_btnSysGenKl, &QPushButton::clicked, this, &MainWindow::onOpenSystemGenericKlClicked);
    connect(m_btnSysRemConf, &QPushButton::clicked, this, &MainWindow::onOpenSystemRemoteConfClicked);

    sysLayout->addWidget(m_btnOpenSys);
    sysLayout->addWidget(m_btnSysProp);
    sysLayout->addWidget(m_btnSysKeymap);
    sysLayout->addWidget(m_btnSysGenKl);
    sysLayout->addWidget(m_btnSysRemConf);
    grid->addWidget(m_systemGroupBox, 0, 0);

    // Vendor group
    m_vendorGroupBox = new QGroupBox;
    auto *venLayout = new QVBoxLayout(m_vendorGroupBox);
    m_btnOpenVen = new QPushButton;
    m_btnVenProp = new QPushButton;
    m_btnVenKeymap = new QPushButton;
    m_btnVenGenKl = new QPushButton;
    m_btnVenRemConf = new QPushButton;

    connect(m_btnOpenVen, &QPushButton::clicked, this, &MainWindow::onOpenVendorClicked);
    connect(m_btnVenProp, &QPushButton::clicked, this, &MainWindow::onOpenVendorBuildPropClicked);
    connect(m_btnVenKeymap, &QPushButton::clicked, this, &MainWindow::onOpenVendorKeymapClicked);
    connect(m_btnVenGenKl, &QPushButton::clicked, this, &MainWindow::onOpenVendorGenericKlClicked);
    connect(m_btnVenRemConf, &QPushButton::clicked, this, &MainWindow::onOpenVendorRemoteConfClicked);

    venLayout->addWidget(m_btnOpenVen);
    venLayout->addWidget(m_btnVenProp);
    venLayout->addWidget(m_btnVenKeymap);
    venLayout->addWidget(m_btnVenGenKl);
    venLayout->addWidget(m_btnVenRemConf);
    grid->addWidget(m_vendorGroupBox, 0, 1);

    // Kernel & Recovery group
    m_kernelGroup = new QGroupBox;
    auto *kernLayout = new QVBoxLayout(m_kernelGroup);
    m_btnOpenKern = new QPushButton;
    m_btnOpenRec = new QPushButton;
    m_btnReplKern = new QPushButton;
    m_btnReplRec = new QPushButton;
    m_btnMagiskBoot = new QPushButton;

    connect(m_btnOpenKern, &QPushButton::clicked, this, &MainWindow::onOpenKernelClicked);
    connect(m_btnOpenRec, &QPushButton::clicked, this, &MainWindow::onOpenRecoveryClicked);
    connect(m_btnReplKern, &QPushButton::clicked, this, &MainWindow::onReplaceKernelClicked);
    connect(m_btnReplRec, &QPushButton::clicked, this, &MainWindow::onReplaceRecoveryClicked);
    connect(m_btnMagiskBoot, &QPushButton::clicked, this, &MainWindow::onBootImgForMagiskClicked);

    kernLayout->addWidget(m_btnOpenKern);
    kernLayout->addWidget(m_btnOpenRec);
    kernLayout->addWidget(m_btnReplKern);
    kernLayout->addWidget(m_btnReplRec);
    kernLayout->addWidget(m_btnMagiskBoot);
    grid->addWidget(m_kernelGroup, 1, 0);

    // Rooting & Hardware mods group
    m_wifiGroupBox = new QGroupBox;
    auto *rootLayout = new QVBoxLayout(m_wifiGroupBox);
    m_btnRoot = new QPushButton;
    m_btnUnroot = new QPushButton;
    m_btnWifi = new QPushButton;

    connect(m_btnRoot, &QPushButton::clicked, this, &MainWindow::onRootFirmwareClicked);
    connect(m_btnUnroot, &QPushButton::clicked, this, &MainWindow::onRemoveRootClicked);
    connect(m_btnWifi, &QPushButton::clicked, this, &MainWindow::onMultiWifiDriversClicked);

    rootLayout->addWidget(m_btnRoot);
    rootLayout->addWidget(m_btnUnroot);
    rootLayout->addWidget(m_btnWifi);
    grid->addWidget(m_wifiGroupBox, 1, 1);

    layout->addLayout(grid);
    layout->addStretch();
    return widget;
}

QWidget* MainWindow::buildPatchesTab()
{
    m_patchesPanel = new QWidget;
    auto *layout = new QVBoxLayout(m_patchesPanel);

    auto *topLayout = new QHBoxLayout;
    m_lblPatches = new QLabel;
    topLayout->addWidget(m_lblPatches);
    m_patchCombo = new QComboBox;
    connect(m_patchCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onPatchComboChanged);
    topLayout->addWidget(m_patchCombo, 1);

    m_btnApplyPatch = new QPushButton;
    m_btnApplyPatch->setStyleSheet("font-weight: bold;");
    connect(m_btnApplyPatch, &QPushButton::clicked, this, &MainWindow::onApplyPatchClicked);
    topLayout->addWidget(m_btnApplyPatch);

    layout->addLayout(topLayout);

    // Metadata
    auto *metaLayout = new QFormLayout;
    m_lblPatchAuthor = new QLabel("-");
    m_lblPatchVersion = new QLabel("-");
    m_lblPatchDescription = new QLabel("-");
    m_lblPatchDescription->setWordWrap(true);

    m_lblAuthorTitle = new QLabel;
    m_lblVerMetaTitle = new QLabel;
    m_lblDescTitle = new QLabel;

    metaLayout->addRow(m_lblAuthorTitle, m_lblPatchAuthor);
    metaLayout->addRow(m_lblVerMetaTitle, m_lblPatchVersion);
    metaLayout->addRow(m_lblDescTitle, m_lblPatchDescription);
    layout->addLayout(metaLayout);

    // Output console
    m_lblPatchExec = new QLabel;
    layout->addWidget(m_lblPatchExec);
    m_patchRTB = new QTextEdit;
    m_patchRTB->setReadOnly(true);
    m_patchRTB->setFontFamily("Monospace");
    m_patchRTB->setStyleSheet("background: #111; color: #0f0;");
    layout->addWidget(m_patchRTB, 1);

    m_btnOpenPatches = new QPushButton;
    connect(m_btnOpenPatches, &QPushButton::clicked, this, &MainWindow::onAddPatchLabelClicked);
    layout->addWidget(m_btnOpenPatches, 0, Qt::AlignLeft);

    return m_patchesPanel;
}

QWidget* MainWindow::buildPackTab()
{
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);
    layout->setSpacing(16);

    // 1. Repack to image
    m_imgGroup = new QGroupBox;
    auto *imgLayout = new QVBoxLayout(m_imgGroup);

    m_btnRepackImg = new QPushButton;
    m_btnRepackImg->setFixedHeight(40);
    m_btnRepackImg->setStyleSheet("font-weight: bold; font-size: 14px;");
    connect(m_btnRepackImg, &QPushButton::clicked, this, &MainWindow::onRepackToImageClicked);
    imgLayout->addWidget(m_btnRepackImg);
    layout->addWidget(m_imgGroup);

    // 2. Repack to Zip
    m_zipGroup = new QGroupBox;
    auto *zipLayout = new QVBoxLayout(m_zipGroup);

    auto *sliderLayout = new QHBoxLayout;
    m_lblCompSlider = new QLabel;
    sliderLayout->addWidget(m_lblCompSlider);
    m_compressionSlider = new QSlider(Qt::Horizontal);
    m_compressionSlider->setRange(0, 9);
    m_compressionSlider->setValue(5);
    sliderLayout->addWidget(m_compressionSlider, 1);
    m_lblCompressionStatus = new QLabel("5 (Normal)");
    sliderLayout->addWidget(m_lblCompressionStatus);
    zipLayout->addLayout(sliderLayout);

    connect(m_compressionSlider, &QSlider::valueChanged, this, [this](int val) {
        m_lblCompressionStatus->setText(QString::number(val) + (val == 0 ? " (Store)" : (val == 9 ? " (Maximum)" : " (Normal)")));
    });

    m_btnZipRom = new QPushButton;
    m_btnZipRom->setFixedHeight(36);
    connect(m_btnZipRom, &QPushButton::clicked, this, &MainWindow::onZipRomClicked);
    zipLayout->addWidget(m_btnZipRom);
    layout->addWidget(m_zipGroup);

    // 3. Export CTP
    m_ctpGroup = new QGroupBox;
    auto *ctpLayout = new QVBoxLayout(m_ctpGroup);

    auto *ctpSliderLayout = new QHBoxLayout;
    m_lblCtpSlider = new QLabel;
    ctpSliderLayout->addWidget(m_lblCtpSlider);
    m_ctpCompressionSlider = new QSlider(Qt::Horizontal);
    m_ctpCompressionSlider->setRange(0, 9);
    m_ctpCompressionSlider->setValue(5);
    ctpSliderLayout->addWidget(m_ctpCompressionSlider, 1);
    m_lblCTPCompressionStatus = new QLabel("5");
    ctpSliderLayout->addWidget(m_lblCTPCompressionStatus);
    ctpLayout->addLayout(ctpSliderLayout);

    connect(m_ctpCompressionSlider, &QSlider::valueChanged, this, [this](int val) {
        m_lblCTPCompressionStatus->setText(QString::number(val));
    });

    m_btnCtp = new QPushButton;
    m_btnCtp->setFixedHeight(36);
    connect(m_btnCtp, &QPushButton::clicked, this, &MainWindow::onExportCTPClicked);
    ctpLayout->addWidget(m_btnCtp);
    layout->addWidget(m_ctpGroup);

    layout->addStretch();
    return widget;
}

QWidget* MainWindow::buildFileSearchTab()
{
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);

    auto *searchLayout = new QHBoxLayout;
    m_lblSearchFile = new QLabel;
    searchLayout->addWidget(m_lblSearchFile);
    m_leSearchFile = new QLineEdit;
    connect(m_leSearchFile, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    searchLayout->addWidget(m_leSearchFile, 1);
    layout->addLayout(searchLayout);

    m_filesList = new QListWidget;
    connect(m_filesList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        if (!item) return;
        QString path = item->text();
        if (QFileInfo(path).isDir()) {
            openFileManager(path);
        } else {
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        }
    });
    layout->addWidget(m_filesList, 1);

    return widget;
}

// -------------------------------------------------------------
// ROM Loading and Metadata Extraction
// -------------------------------------------------------------
void MainWindow::loadRom()
{
    loadInfo();
    updateAppsView();

    // Check patch folder
    m_patchCombo->clear();
    QDir patchDir(Platform::patchesDir());
    if (patchDir.exists()) {
        auto patches = patchDir.entryList(QStringList() << "*.patch", QDir::Files);
        m_patchCombo->addItems(patches);
    }
    m_patchesPanel->setEnabled(m_patchCombo->count() > 0 && QDir(tmpDir() + "/level2").exists());

    // Check DTB files
    m_dtbCombo->clear();
    int dtsCount = 0;
    {
        const QDir dtbDir(tmpDir() + "/level2/dtb");
        const QStringList dtsFiles = dtbDir.exists()
            ? dtbDir.entryList(QStringList() << "*.dts", QDir::Files)
            : QStringList();
        dtsCount = dtsFiles.size();
        m_dtbCombo->addItems(dtsFiles);
        if (dtsCount == 0) {
            // Allwinner firmware (Android 4.4 and older) ships no FDT at all – the
            // hardware description lives in sys_config.fex / config.fex instead.
            m_dtbCombo->addItem(QFile::exists(tmpDir() + "/level1/sys_config.fex")
                                    ? tr("No DTB in this firmware (Allwinner sys_config.fex)")
                                    : tr("No DTB found – unpack Level 2 with the DTB option"));
        }
    }
    m_dtbPanel->setEnabled(dtsCount > 0);

    // Partition groups
    m_systemGroupBox->setEnabled(QDir(tmpDir() + "/level2/system").exists());
    m_vendorGroupBox->setEnabled(QDir(tmpDir() + "/level2/vendor").exists());

    // Keys checklist
    QFile keysConf(tmpDir() + "/level1/keys.conf");
    if (keysConf.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&keysConf);
        in.setCodec("UTF-8");
        QStringList keys = in.readAll().split('\n', Qt::SkipEmptyParts);
        for (int i = 0; i < m_keysChecklist->count(); ++i) {
            auto *item = m_keysChecklist->item(i);
            if (keys.contains(item->text().trimmed())) {
                item->setCheckState(Qt::Checked);
            }
        }
    }
}

void MainWindow::loadInfo()
{
    m_leVersion->clear();
    m_leModel->clear();
    m_leVendor->clear();
    m_leProduct->clear();
    m_leSDK->clear();
    m_lePatch->clear();
    m_leBuild->clear();
    m_leBuildDisplayId->clear();
    m_leSystemSize->clear();
    m_leSystemFiles->clear();
    m_leSystemApps->clear();
    m_leVendorSize->clear();
    m_leVendorFiles->clear();
    m_leVendorApps->clear();

    m_cbRamdiskSystem->setChecked(false);
    m_cbContainsVendor->setChecked(false);
    m_cbMultiDtb->setChecked(false);
    m_cbBootupGunzipped->setChecked(false);
    m_cbSuperPartition->setChecked(false);
    m_cbCompressBrotli->setChecked(false);
    m_lblPackageType->setText("None");
    m_imageType = ImgType::None;

    // Detect package type from level1 contents
    QString l1 = tmpDir() + "/level1";
    // Vendor tools place the metadata either next to the partitions or in an Image/ sub folder
    const bool amlogic = QFile::exists(l1 + "/DDR.USB") ||
                         !findPartitionFile("_aml_dtb").isEmpty();
    const bool allwinner = QFile::exists(l1 + "/image.cfg");
    const bool rockchip = QFile::exists(l1 + "/parameter.txt") ||
                          QFile::exists(l1 + "/Image/parameter.txt") ||
                          QFile::exists(l1 + "/package-file");
    if (amlogic) {
        m_imageType = ImgType::Amlogic;
        m_lblPackageType->setText("Amlogic");
        m_patcher->imageType = "amlogic";
    } else if (rockchip) {
        m_imageType = ImgType::Rockchip;
        m_lblPackageType->setText("Rockchip");
        m_patcher->imageType = "rockchip";
    } else if (allwinner) {
        m_imageType = ImgType::Allwinner;
        m_lblPackageType->setText("Allwinner");
        m_patcher->imageType = "allwinner";
    }

    m_cbSpecifyChipset->setEnabled(m_imageType == ImgType::Rockchip);
    m_cbAddPlatformConfig->setEnabled(m_imageType == ImgType::Amlogic);

    // Detect sysdir
    m_sysDir = checkSysDir();
    if (m_sysDir == "system/system/") {
        m_cbRamdiskSystem->setChecked(true);
    }

    // Check Vendor existence
    if (!findPartitionFile("vendor").isEmpty() || QDir(tmpDir() + "/level2/vendor").exists())
    {
        m_cbContainsVendor->setChecked(true);
    }

    // Check Super Partition
    if (QDir(tmpDir() + "/level2/super").exists()) {
        m_cbSuperPartition->setChecked(true);
    }

    // Check DTB
    QDir dtbDir(tmpDir() + "/level2/dtb");
    if (dtbDir.exists() && dtbDir.entryList(QDir::Files).count() > 1) {
        m_cbMultiDtb->setChecked(true);
    }

    // Check Brotli
    QDir l1Check(l1);
    if (l1Check.exists()) {
        auto brFiles = l1Check.entryList(QStringList() << "*.br", QDir::Files);
        if (!brFiles.isEmpty()) {
            m_cbCompressBrotli->setChecked(true);
        }
    }

    // Check Bootup Gunzipped
    QFile logoTypes(tmpDir() + "/level3/.config/logo.types");
    if (logoTypes.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (logoTypes.readAll().contains("bootup=gz")) {
            m_cbBootupGunzipped->setChecked(true);
        }
    }

    // Properties from build.prop
    m_leVersion->setText(getProp("ro.build.version.release"));
    m_leModel->setText(getProp("ro.product.model"));
    QString brand = getProp("ro.product.brand");
    if (brand.isEmpty()) brand = getProp("ro.product.system.brand");
    m_leVendor->setText(brand);

    QString device = getProp("ro.product.device");
    if (device.isEmpty()) device = getProp("ro.product.system.device");
    m_leProduct->setText(device);

    m_leSDK->setText(getProp("ro.build.version.sdk"));
    m_lePatch->setText(getProp("ro.build.version.security_patch"));
    m_leBuild->setText(getProp("ro.build.id"));
    m_leBuildDisplayId->setText(getProp("ro.build.display.id"));

    // Locales
    m_cbLocales->clear();
    QString locale = getProp("ro.product.locale").trimmed();
    QFile localesFile(Platform::binFile("resources/locales"));
    if (localesFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&localesFile);
        in.setCodec("UTF-8");
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (!line.isEmpty()) {
                m_cbLocales->addItem(line);
                if (!locale.isEmpty() && line.startsWith(locale)) {
                    m_cbLocales->setCurrentText(line);
                }
            }
        }
    }

    // Partition Sizes and file counts
    QString sysPath = tmpDir() + "/level2/" + m_sysDir;
    if (QDir(sysPath).exists()) {
        m_leSystemSize->setText(QString::number(romSize(sysPath) / (1024 * 1024)));
        m_leSystemFiles->setText(QString::number(filesAmount(sysPath)));
    }

    QString venPath = tmpDir() + "/level2/vendor";
    if (QDir(venPath).exists()) {
        m_leVendorSize->setText(QString::number(romSize(venPath) / (1024 * 1024)));
        m_leVendorFiles->setText(QString::number(filesAmount(venPath)));
    }

    // App counts
    int sysApps = 0, venApps = 0;
    QDir level2Dir(tmpDir() + "/level2");
    if (level2Dir.exists()) {
        QDirIterator it(level2Dir.path(), QStringList() << "*.apk", QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString apkPath = it.next();
            if (apkPath.contains("/system/")) {
                sysApps++;
            } else {
                venApps++;
            }
        }
    }
    m_leSystemApps->setText(QString::number(sysApps));
    m_leVendorApps->setText(QString::number(venApps));
}

QString MainWindow::checkSysDir() const
{
    if (QFile::exists(tmpDir() + "/level2/system/system/build.prop")) {
        return "system/system/";
    }
    return "system/";
}

QString MainWindow::getProp(const QString &property) const
{
    QString combinedProps;
    QStringList paths = {
        tmpDir() + "/level2/" + m_sysDir + "build.prop",
        tmpDir() + "/level2/vendor/build.prop",
        tmpDir() + "/level2/product/build.prop",
        tmpDir() + "/level2/odm/build.prop",
        tmpDir() + "/level2/oem/build.prop",
        tmpDir() + "/level2/system_ext/build.prop"
    };

    for (const auto &p : paths) {
        QFile file(p);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            combinedProps += "\n" + QString::fromUtf8(file.readAll());
        }
    }

    const auto lines = combinedProps.split('\n', Qt::SkipEmptyParts);
    for (const auto &line : lines) {
        if (line.trimmed().startsWith(property + "=")) {
            return line.section('=', 1).trimmed();
        }
    }
    return QString();
}

long MainWindow::romSize(const QString &dir)
{
    long total = 0;
    QDirIterator it(dir, QDir::Files | QDir::Hidden, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        total += it.fileInfo().size();
    }
    return total;
}

int MainWindow::filesAmount(const QString &dir)
{
    int count = 0;
    QDirIterator it(dir, QDir::Files | QDir::Hidden, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        count++;
    }
    return count;
}

// -------------------------------------------------------------
// Unpack Operations
// -------------------------------------------------------------
void MainWindow::onUnpackClicked()
{
    QString filter = tr("Upgrade Packages (*.img *.ctp *.zip);;Image Files (*.img);;Zip Files (*.zip);;CTP Projects (*.ctp);;All Files (*)");
    QString input = QFileDialog::getOpenFileName(this, tr("Select Firmware Package to Unpack"), QString(), filter);
    if (input.isEmpty()) return;

    UnpackLevelDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;

    unpack(input, dlg.unpackLevel());
}

void MainWindow::unpack(const QString &inputFile, const QString &unpackItems)
{
    m_leUpgradePackage->setText(inputFile);
    setBusy(true);
    setStatusText(tr("Preparing unpack directory..."));

    QDir tmp(tmpDir());
    if (tmp.exists()) {
        tmp.removeRecursively();
    }
    QDir().mkpath(tmpDir());

    QtConcurrent::run([this, inputFile, unpackItems]() {
        QString outDir = tmpDir();
        bool ok = true;

        // Level 1: Split package
        setStatusText(tr("Unpacking level1 (Splitting package)..."));
        QDir().mkpath(outDir + "/level1");

        if (inputFile.endsWith(".zip", Qt::CaseInsensitive)) {
            ok = m_unpacker->unpackUpgradeZip(inputFile, outDir + "/level1");
        } else if (inputFile.endsWith(".ctp", Qt::CaseInsensitive)) {
            ok = m_unpacker->unpackUpgradeZip(inputFile, outDir);
        } else {
            ok = m_unpacker->unpackVendorImage(inputFile, outDir + "/level1");
        }

        if (!ok) {
            QMetaObject::invokeMethod(this, [this]() {
                setBusy(false);
                setStatusText(tr("Failed to unpack level 1."));
                const QString reason = m_unpacker->lastError();
                QMessageBox::critical(this, tr("Error"),
                    tr("Failed to unpack package. Not a supported firmware image or missing tool.") +
                    (reason.isEmpty() ? QString() : "\n\n" + reason));
            });
            return;
        }

        // Level 2: Partition unpack
        if (unpackItems.contains(";level2;")) {
            setStatusText(tr("Unpacking level2 (Partitions)..."));
            QDir().mkpath(outDir + "/level2");
            QDir().mkpath(outDir + "/level2/.config");

            if (unpackItems.contains(";super;")) {
                setStatusText(tr("Unpacking super partition..."));
                const QString superFile = findPartitionFile("super");
                if (!superFile.isEmpty())
                    m_unpacker->unpackSuper(superFile, outDir + "/level2/super");
            }

            const QStringList partitions = {"system", "vendor", "product", "odm", "oem", "system_ext", "odm_ext"};
            for (const auto &part : partitions) {
                if (unpackItems.contains(";" + part + ";")) {
                    setStatusText(tr("Unpacking %1...").arg(part));
                    ext4PartitionUnpack(part, outDir);
                }
            }

            if (unpackItems.contains(";boot;")) {
                setStatusText(tr("Unpacking kernel / boot..."));
                const QString bootFile = findPartitionFile("boot");
                if (!bootFile.isEmpty())
                    m_unpacker->unpackKernel(bootFile, outDir + "/level2/boot");
            }

            if (unpackItems.contains(";recovery;")) {
                setStatusText(tr("Unpacking recovery..."));
                const QString recFile = findPartitionFile("recovery");
                if (!recFile.isEmpty())
                    m_unpacker->unpackKernel(recFile, outDir + "/level2/recovery");
            }

            if (unpackItems.contains(";dtb;")) {
                setStatusText(tr("Unpacking DTB..."));
                QString dtbFile = findPartitionFile("dtb");
                if (dtbFile.isEmpty()) dtbFile = findPartitionFile("_aml_dtb");
                if (dtbFile.isEmpty()) dtbFile = findPartitionFile("dtb_a");
                if (!dtbFile.isEmpty()) {
                    m_unpacker->unpackDtb(dtbFile, outDir + "/level2/dtb");
                } else {
                    m_logger->log("[UNPACKER] no DTB in this firmware – "
                                  "nothing to unpack for the DTS tab");
                }
            }
        }

        // Level 3: Boot Media
        if (unpackItems.contains(";level3;")) {
            setStatusText(tr("Unpacking level3 (Media assets)..."));
            QDir().mkpath(outDir + "/level3");

            if (unpackItems.contains(";logo;")) {
                QString logoFile = findPartitionFile("logo");
                if (logoFile.isEmpty()) logoFile = findPartitionFile("boot-resource");
                if (!logoFile.isEmpty())
                    m_unpacker->unpackLogo(logoFile, outDir + "/level3/bootlogo");
            }

            if (unpackItems.contains(";bootanimation;")) {
                const QString animZip = findPartitionAsset("media/bootanimation.zip");
                if (!animZip.isEmpty())
                    m_unpacker->unpackBootanimation(animZip, outDir + "/level3/bootanimation");
            }

            if (unpackItems.contains(";bootvideo;")) {
                const QString vidDir = findPartitionAsset("media");
                if (!vidDir.isEmpty())
                    m_unpacker->unpackBootVideo(QFileInfo(vidDir).absolutePath(), outDir + "/level3/bootvideo");
            }
        }

        QMetaObject::invokeMethod(this, [this]() {
            setBusy(false);
            setStatusText(tr("Unpack completed."));
            loadRom();
            onRefreshLogoClicked();
            QMessageBox::information(this, tr("Success"), tr("Firmware unpacked successfully!"));
        });
    });
}

QString MainWindow::findPartitionFile(const QString &name) const
{
    const QString l1 = tmpDir() + "/level1";
    const QStringList candidates = {
        l1 + "/" + name + ".PARTITION",
        l1 + "/Image/" + name + ".PARTITION",
        l1 + "/" + name + ".img",
        l1 + "/Image/" + name + ".img",
        l1 + "/" + name + ".fex",
        // Amlogic A/B images only ship slot _a partitions (boot_a.PARTITION, …)
        l1 + "/" + name + "_a.PARTITION",
        l1 + "/Image/" + name + "_a.PARTITION",
        l1 + "/" + name + "_a.img",
        l1 + "/Image/" + name + "_a.img",
        tmpDir() + "/level2/super/" + name + ".img",
        tmpDir() + "/level2/super/" + name + "_a.img"
    };
    for (const QString &candidate : candidates) {
        if (QFile::exists(candidate)) return candidate;
    }

    // Vendor packages name partitions freely – fall back to a stem match anywhere in level1
    QDirIterator it(l1, {"*.img", "*.PARTITION", "*.fex"}, QDir::Files,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString path = it.next();
        const QString stem = QFileInfo(path).completeBaseName();
        if (stem.compare(name, Qt::CaseInsensitive) == 0 ||
            stem.compare(name + "_a", Qt::CaseInsensitive) == 0)
            return path;
    }
    return QString();
}

void MainWindow::ext4PartitionUnpack(const QString &partition, const QString &outputDir)
{
    const QString partFile = findPartitionFile(partition);
    if (!partFile.isEmpty()) {
        m_unpacker->unpackExt4(partFile, outputDir + "/level2/" + partition);
    }
}

// -------------------------------------------------------------
// Repacking Operations
// -------------------------------------------------------------
void MainWindow::onRepackToImageClicked()
{
    QString outFile = QFileDialog::getSaveFileName(this, tr("Save Burn Tool Image"),
                                                 Platform::defaultSaveDir() + "/repacked_rom.img",
                                                 tr("Image Files (*.img)"));
    if (outFile.isEmpty()) return;

    setBusy(true);
    setStatusText(tr("Repacking firmware image..."));

    bool addPlatform = m_cbAddPlatformConfig->isChecked();
    bool specChip = m_cbSpecifyChipset->isChecked();
    QString chip = m_leRKChipset->text().trimmed();

    QtConcurrent::run([this, outFile, addPlatform, specChip, chip]() {
        // Standard Level 3 repack before packing image
        if (QDir(tmpDir() + "/level2/dtb").exists()) {
            setStatusText(tr("Repacking DTB..."));
            QString dtbOut = tmpDir() + "/level1/_aml_dtb.PARTITION";
            if (!QFile::exists(dtbOut)) dtbOut = tmpDir() + "/level1/dtb.img";
            m_unpacker->repackDtb(tmpDir() + "/level2/dtb", dtbOut);
        }

        if (QDir(tmpDir() + "/level3/bootlogo").exists()) {
            setStatusText(tr("Repacking Logo..."));
            QString logoOut = tmpDir() + "/level1/logo.PARTITION";
            if (!QFile::exists(logoOut)) logoOut = tmpDir() + "/level1/boot-resource.fex";
            m_unpacker->repackLogo(tmpDir() + "/level3/bootlogo", logoOut);
        }

        // Repack partitions
        const QStringList partitions = {"system", "vendor", "product", "odm", "oem", "system_ext"};
        for (const auto &part : partitions) {
            QString partDir = tmpDir() + "/level2/" + part;
            if (QDir(partDir).exists()) {
                setStatusText(tr("Repacking partition: %1...").arg(part));
                QString outPart = tmpDir() + "/level1/" + part + ".PARTITION";
                if (!QFile::exists(outPart)) outPart = tmpDir() + "/level1/" + part + ".img";
                m_unpacker->repackExt4(partDir, outPart, part, m_cbCompressBrotli->isChecked());
            }
        }

        setStatusText(tr("Packing final vendor image..."));
        bool ok = m_unpacker->repackVendorImage(tmpDir() + "/level1", outFile, addPlatform, specChip, chip);

        QMetaObject::invokeMethod(this, [this, ok, outFile]() {
            setBusy(false);
            if (ok) {
                setStatusText(tr("Firmware image packed successfully."));
                QMessageBox::information(this, tr("Success"), tr("Image successfully repacked:\n%1").arg(outFile));
            } else {
                setStatusText(tr("Failed to repack image."));
                const QString reason = m_unpacker->lastError();
                QMessageBox::critical(this, tr("Error"),
                    tr("Failed to repack image. Check logs for details.") +
                    (reason.isEmpty() ? QString() : "\n\n" + reason));
            }
        });
    });
}

void MainWindow::onZipRomClicked()
{
    QString outFile = QFileDialog::getSaveFileName(this, tr("Save Upgrade Zip"),
                                                 Platform::defaultSaveDir() + "/update.zip",
                                                 tr("Zip Files (*.zip)"));
    if (outFile.isEmpty()) return;

    setBusy(true);
    setStatusText(tr("Packaging update zip..."));
    int comp = m_compressionSlider->value();

    QtConcurrent::run([this, outFile, comp]() {
        bool ok = m_unpacker->repackZipFile(tmpDir() + "/level1", outFile, comp);
        QMetaObject::invokeMethod(this, [this, ok, outFile]() {
            setBusy(false);
            if (ok) {
                setStatusText(tr("Zip ROM created successfully."));
                QMessageBox::information(this, tr("Success"), tr("Update zip saved:\n%1").arg(outFile));
            } else {
                setStatusText(tr("Failed to create update zip."));
                QMessageBox::critical(this, tr("Error"), tr("Failed to package update zip."));
            }
        });
    });
}

void MainWindow::onExportCTPClicked()
{
    QString outFile = QFileDialog::getSaveFileName(this, tr("Export Project Archive"),
                                                 Platform::defaultSaveDir() + "/project.ctp",
                                                 tr("ROM Customization Toolkit Project (*.ctp)"));
    if (outFile.isEmpty()) return;

    setBusy(true);
    setStatusText(tr("Exporting CTP project..."));

    int comp = m_ctpCompressionSlider->value();

    QtConcurrent::run([this, outFile, comp]() {
        bool ok = m_unpacker->repackZipFile(tmpDir(), outFile, comp);
        QMetaObject::invokeMethod(this, [this, ok, outFile]() {
            setBusy(false);
            if (ok) {
                setStatusText(tr("Project exported successfully."));
                QMessageBox::information(this, tr("Success"), tr("CTP Project saved:\n%1").arg(outFile));
            } else {
                setStatusText(tr("Failed to export project."));
                QMessageBox::critical(this, tr("Error"), tr("Failed to export CTP project."));
            }
        });
    });
}

// -------------------------------------------------------------
// Boot Media Slots
// -------------------------------------------------------------
void MainWindow::onRefreshLogoClicked()
{
    const QStringList candidates = {
        tmpDir() + "/level3/bootlogo/bootup.bmp",
        tmpDir() + "/level3/bootlogo/bootlogo.bmp",
        tmpDir() + "/level3/bootlogo/logo.bmp",
        tmpDir() + "/level3/bootlogo/bootup.png"
    };

    for (const auto &c : candidates) {
        if (QFile::exists(c)) {
            QPixmap pix(c);
            if (!pix.isNull()) {
                m_bootlogoPicture->setPixmap(pix.scaled(
                    m_bootlogoPicture->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
                return;
            }
        }
    }
    m_bootlogoPicture->setText(tr("No logo found or unloaded"));
}

void MainWindow::onBootLogoButtonClicked()
{
    QString logoFile = QFileDialog::getOpenFileName(this, tr("Select New Boot Logo"), QString(),
                                                   tr("Images (*.bmp *.png);;All Files (*)"));
    if (logoFile.isEmpty()) return;

    QString logoDir = tmpDir() + "/level3/bootlogo";
    QDir().mkpath(logoDir);

    QString dst = logoDir + "/bootup.bmp";
    QFile::remove(dst);
    if (QFile::copy(logoFile, dst)) {
        onRefreshLogoClicked();
        QMessageBox::information(this, tr("Logo Updated"), tr("Boot logo replaced with %1").arg(QFileInfo(logoFile).fileName()));
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Failed to copy logo file."));
    }
}

void MainWindow::onBootanimationButtonClicked()
{
    onBootanimationChangeClicked();
}

void MainWindow::onBootvideoButtonClicked()
{
    onChangeBootvideoClicked();
}

void MainWindow::onPlayBootanimationClicked()
{
    const QString animDir = tmpDir() + "/level3/bootanimation";
    if (!QDir(animDir).exists()) {
        // Not extracted yet – pull it out of the partitions on demand
        const QString zip = findPartitionAsset("media/bootanimation.zip");
        if (zip.isEmpty() || !m_unpacker->unpackBootanimation(zip, animDir)) {
            QMessageBox::information(this, tr("Notice"),
                                     tr("No bootanimation.zip found in the unpacked partitions."));
            return;
        }
    }

    // Animation parts are numbered part0, part1, … (many ROMs have no part0 at all)
    auto firstFrame = [&animDir]() -> QString {
        const auto partDirs = QDir(animDir).entryList({"part*"}, QDir::Dirs | QDir::NoDotAndDotDot,
                                                     QDir::Name);
        for (const QString &part : partDirs) {
            const QDir dir(animDir + "/" + part);
            const auto frames = dir.entryList({"*.png", "*.jpg"}, QDir::Files, QDir::Name);
            if (!frames.isEmpty()) return dir.filePath(frames.first());
        }
        return QString();
    };

    QString frame = firstFrame();
    if (frame.isEmpty()) {
        // Not extracted yet (level 3 may not have been selected) – pull it from the partitions
        const QString zip = findPartitionAsset("media/bootanimation.zip");
        if (!zip.isEmpty() && m_unpacker->unpackBootanimation(zip, animDir))
            frame = firstFrame();
    }

    if (frame.isEmpty()) {
        QMessageBox::information(this, tr("Notice"),
                                 tr("No animation frames found in the unpacked firmware."));
        return;
    }

    // desc.txt starts with "<width> <height> <fps>" – only trust three plain integers
    QFile desc(animDir + "/desc.txt");
    if (desc.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QStringList header = QString(desc.readLine()).trimmed().split(' ', Qt::SkipEmptyParts);
        bool okWidth = false, okHeight = false, okFps = false;
        const int width  = header.value(0).toInt(&okWidth);
        const int height = header.value(1).toInt(&okHeight);
        const int fps    = header.value(2).toInt(&okFps);
        if (okWidth && okHeight && okFps) {
            m_lblBootanimFPS->setText(tr("FPS: %1  (%2 x %3)").arg(fps).arg(width).arg(height));
            m_lblBootanimFPS->setToolTip(QString(desc.fileName()) + ": " + header.join(' '));
        } else {
            m_lblBootanimFPS->setText(tr("FPS: -"));
        }
    }

    // ---- playback: follow desc.txt ("p <loops> <pause> <folder>") ----
    auto framesInPart = [&animDir](const QString &part) {
        const QDir dir(animDir + "/" + part);
        QStringList files;
        for (const QString &f : dir.entryList({"*.png", "*.jpg"}, QDir::Files, QDir::Name))
            files << dir.filePath(f);
        return files;
    };

    QVector<QStringList> partFrames;
    QVector<int> partLoops;
    QVector<int> partPauses;
    int fpsValue = 25;

    QFile parts(animDir + "/desc.txt");
    if (parts.open(QIODevice::ReadOnly | QIODevice::Text)) {
        bool firstLine = true;
        while (!parts.atEnd()) {
            const QStringList tokens = QString(parts.readLine()).trimmed().split(' ', Qt::SkipEmptyParts);
            if (tokens.isEmpty()) continue;
            if (firstLine) {   // "<width> <height> <fps>"
                firstLine = false;
                bool okFps = false;
                const int parsed = tokens.value(2).toInt(&okFps);
                if (okFps) fpsValue = qMax(1, parsed);
                continue;
            }
            if (tokens.value(0) != "p") continue;
            bool okLoops = false, okPause = false;
            const int loops = tokens.value(1).toInt(&okLoops);
            const int pause = tokens.value(2).toInt(&okPause);
            const QStringList files = framesInPart(tokens.value(3));
            if (files.isEmpty()) continue;
            partFrames << files;
            partLoops << (okLoops ? qMax(0, loops) : 1);
            partPauses << (okPause ? qMax(0, pause) : 0);
        }
    }

    // No usable desc.txt – play every part once
    if (partFrames.isEmpty()) {
        const auto partDirs = QDir(animDir).entryList({"part*"}, QDir::Dirs | QDir::NoDotAndDotDot,
                                                      QDir::Name);
        for (const QString &part : partDirs) {
            const QStringList files = framesInPart(part);
            if (files.isEmpty()) continue;
            partFrames << files;
            partLoops << 1;
            partPauses << 0;
        }
    }

    m_bootanimFrames.clear();
    m_bootanimPartStart.clear();
    for (const QStringList &files : partFrames) {
        m_bootanimPartStart << m_bootanimFrames.size();
        m_bootanimFrames << files;
    }
    m_bootanimPartLoops  = partLoops;
    m_bootanimPartPause  = partPauses;
    m_bootanimPart       = 0;
    m_bootanimFrame      = 0;
    m_bootanimFps        = fpsValue;
    m_bootanimPlaying    = !m_bootanimFrames.isEmpty();
    m_bootanimCache.clear();

    if (!m_bootanimTimer) {
        m_bootanimTimer = new QTimer(this);
        connect(m_bootanimTimer, &QTimer::timeout, this, &MainWindow::advanceBootanimFrame);
    }
    m_bootanimTimer->stop();
    if (!m_bootanimPlaying) return;

    showBootanimFrame();
    m_bootanimTimer->start(qMax(1, 1000 / m_bootanimFps));
}

void MainWindow::advanceBootanimFrame()
{
    if (!m_bootanimPlaying || m_bootanimPartStart.isEmpty()) return;

    const int partStart = m_bootanimPartStart.value(m_bootanimPart);
    const int partEnd = (m_bootanimPart + 1 < m_bootanimPartStart.size())
                            ? m_bootanimPartStart.at(m_bootanimPart + 1)
                            : m_bootanimFrames.size();

    ++m_bootanimFrame;
    if (m_bootanimFrame < partEnd) {
        showBootanimFrame();
        return;
    }

    // Part finished: a loop count of 0 means "loop forever", otherwise repeat while passes remain
    const int loops = m_bootanimPartLoops.value(m_bootanimPart);
    if (loops == 0) {
        m_bootanimFrame = partStart;
        showBootanimFrame();
        return;
    }

    const int remaining = loops - 1;
    m_bootanimPartLoops[m_bootanimPart] = remaining;
    if (remaining > 0) {
        m_bootanimFrame = partStart;
        showBootanimFrame();
        return;
    }

    const int pause = m_bootanimPartPause.value(m_bootanimPart);
    ++m_bootanimPart;
    if (m_bootanimPart >= m_bootanimPartStart.size()) {
        stopBootanimPreview();   // animation finished
        return;
    }
    m_bootanimFrame = m_bootanimPartStart.at(m_bootanimPart);

    if (pause > 0) {
        m_bootanimTimer->stop();
        QTimer::singleShot(pause * 1000, this, [this]() {
            if (m_bootanimPlaying && m_bootanimTimer)
                m_bootanimTimer->start(qMax(1, 1000 / m_bootanimFps));
        });
    }
    showBootanimFrame();
}

void MainWindow::showBootanimFrame()
{
    if (m_bootanimFrame < 0 || m_bootanimFrame >= m_bootanimFrames.size()) return;

    const QString path = m_bootanimFrames.at(m_bootanimFrame);
    auto it = m_bootanimCache.constFind(path);
    if (it == m_bootanimCache.constEnd()) {
        const QPixmap pixmap(path);
        if (pixmap.isNull()) return;
        it = m_bootanimCache.insert(path, pixmap);
    }
    m_bootanimPicture->setPixmap(it.value().scaled(m_bootanimPicture->size(),
                                                  Qt::KeepAspectRatio,
                                                  Qt::SmoothTransformation));
}

void MainWindow::stopBootanimPreview()
{
    m_bootanimPlaying = false;
    if (m_bootanimTimer) m_bootanimTimer->stop();
    m_bootanimPart = 0;
    m_bootanimFrame = 0;
}

void MainWindow::onBootanimationChangeClicked()
{
    QString zip = QFileDialog::getOpenFileName(this, tr("Select bootanimation.zip"), QString(), tr("Zip Files (*.zip)"));
    if (zip.isEmpty()) return;

    // Replace the animation where it actually lives (system-as-root aware)
    stopBootanimPreview();
    const QString animZip = findPartitionAsset("media/bootanimation.zip");
    if (!animZip.isEmpty()) {
        QFile::remove(animZip);
        if (QFile::copy(zip, animZip)) {
            m_unpacker->unpackBootanimation(animZip, tmpDir() + "/level3/bootanimation");
            QMessageBox::information(this, tr("Success"),
                                     tr("Replaced bootanimation: %1").arg(animZip));
            return;
        }
    }
    QMessageBox::warning(this, tr("Error"), tr("Could not find media/bootanimation.zip in the unpacked partitions."));
}

void MainWindow::onPlayBootvideoClicked()
{
    if (m_videoPlayer->state() == QMediaPlayer::PlayingState) {
        m_videoPlayer->pause();
    } else {
        QString vidPath;
        const QStringList parts = {"system", "vendor"};
        for (const auto &p : parts) {
            QString cand = tmpDir() + "/level2/" + p + "/media/bootvideo";
            if (QFile::exists(cand)) { vidPath = cand; break; }
            cand = tmpDir() + "/level2/" + p + "/media/bootvideo.ts";
            if (QFile::exists(cand)) { vidPath = cand; break; }
            cand = tmpDir() + "/level2/" + p + "/media/bootvideo.mp4";
            if (QFile::exists(cand)) { vidPath = cand; break; }
        }
        if (!vidPath.isEmpty()) {
            m_videoPlayer->setMedia(QUrl::fromLocalFile(vidPath));
            m_videoPlayer->play();
        } else {
            QMessageBox::information(this, tr("Notice"), tr("No boot video file found in media directories."));
        }
    }
}

void MainWindow::onChangeBootvideoClicked()
{
    QString vid = QFileDialog::getOpenFileName(this, tr("Select Boot Video"), QString(), tr("Video Files (*.ts *.mp4);;All Files (*)"));
    if (vid.isEmpty()) return;

    const QStringList parts = {"system", "vendor"};
    for (const auto &p : parts) {
        QString mediaDir = tmpDir() + "/level2/" + p + "/media";
        if (QDir(mediaDir).exists()) {
            QString dst = mediaDir + "/bootvideo.ts";
            QFile::remove(dst);
            QFile::copy(vid, dst);
            QMessageBox::information(this, tr("Success"), tr("Replaced boot video in /%1/media/").arg(p));
            return;
        }
    }
}

void MainWindow::onBootlogoTimer()
{
}

void MainWindow::onOpenLogosClicked()
{
    openFileManager(tmpDir() + "/level3/bootlogo");
}

// -------------------------------------------------------------
// Apps Tab Slots
// -------------------------------------------------------------
QString MainWindow::findPartitionAsset(const QString &relativePath) const
{
    const QString l2 = tmpDir() + "/level2";
    static const QStringList partitions = {"system", "vendor", "product", "system_ext", "odm", "oem"};

    for (const QString &partition : partitions) {
        const QStringList candidates = {
            l2 + "/" + partition + "/" + relativePath,          // flat partition tree
            l2 + "/" + partition + "/system/" + relativePath    // system-as-root tree
        };
        for (const QString &candidate : candidates) {
            if (QFile::exists(candidate)) return candidate;
        }
    }

    // Unknown layout – match the file name anywhere below level2
    const QString name = QFileInfo(relativePath).fileName();
    QDirIterator it(l2, {name}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString path = it.next();
        if (path.endsWith("/" + relativePath)) return path;
    }
    return QString();
}

QString MainWindow::resolveAppDir(const QString &virtualPath) const
{
    const QString level2 = tmpDir() + "/level2";
    // system-as-root images keep everything below level2/system/system/
    const QString sysPrefix = m_sysDir.isEmpty() ? QStringLiteral("system/") : m_sysDir;
    if (virtualPath.startsWith("/system/"))
        return level2 + "/" + sysPrefix + virtualPath.mid(8);
    return level2 + virtualPath;   // /vendor/…, /product/…, /odm/…
}

void MainWindow::updateAppsView()
{
    m_appsViewer->clear();
    m_appsDirectory->clear();

    if (!QDir(tmpDir() + "/level2").exists()) return;

    // Same list the original tool offers under "Show apps inside:"
    const QStringList virtualDirs = {
        "/system/app", "/system/priv-app", "/system/framework",
        "/vendor/app", "/vendor/priv-app", "/vendor/framework",
        "/product/app"
    };
    for (const QString &virtualDir : virtualDirs)
        m_appsDirectory->addItem(virtualDir, resolveAppDir(virtualDir));

    if (QDir(tmpDir() + "/level3/apps").exists())
        m_appsDirectory->addItem(tr("Unpacked"), tmpDir() + "/level3/apps");

    if (m_appsDirectory->count() > 0)
        onAppsDirectoryChanged(m_appsDirectory->currentIndex());
}

void MainWindow::onAppsDirectoryChanged(int index)
{
    Q_UNUSED(index)
    m_appsViewer->clear();
    resetManifestInfo();

    const QString fullDir = m_appsDirectory->currentData().toString();
    if (fullDir.isEmpty() || !QDir(fullDir).exists()) return;

    // Android robot taken from the original tool; themed glyph only as a fallback
    QPixmap appPixmap(":/icons/app_icon.png");
    if (!appPixmap.isNull())
        appPixmap = appPixmap.scaled(QSize(64, 64), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QIcon appIcon(appPixmap);
    if (appIcon.isNull()) {
        const QColor accent = palette().color(QPalette::Highlight);
        appIcon = IconHelper::createIconFromSvg(":/icons/apps.svg", accent, QColor(), 64);
        if (appIcon.isNull())
            appIcon = IconHelper::createIcon(IconHelper::Apps, accent, QColor(), 64);
    }

    // One tile per app: <dir>/<App>/<App>.apk (bundle) or <dir>/<App>.apk (flat)
    const auto entries = QDir(fullDir).entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot,
                                                     QDir::Name | QDir::IgnoreCase);
    for (const QFileInfo &entry : entries) {
        QString name;
        QString apkPath;
        if (entry.isDir()) {
            const auto apks = QDir(entry.absoluteFilePath()).entryInfoList({"*.apk"}, QDir::Files, QDir::Name);
            if (apks.isEmpty()) continue;
            name = entry.fileName();
            apkPath = apks.first().absoluteFilePath();
        } else if (entry.suffix().compare("apk", Qt::CaseInsensitive) == 0) {
            name = entry.completeBaseName();
            apkPath = entry.absoluteFilePath();
        } else {
            continue;
        }

        auto *item = new QListWidgetItem(appIcon, name, m_appsViewer);
        item->setData(Qt::UserRole, apkPath);
        item->setToolTip(apkPath);
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignTop);
        // Fixed tile size: selecting an item must not resize it
        item->setSizeHint(QSize(120, 100));
    }
}

void MainWindow::onAppSelectionChanged()
{
    auto *item = m_appsViewer->currentItem();
    if (!item) {
        resetManifestInfo();
        return;
    }
    const QString apkPath = item->data(Qt::UserRole).toString();
    m_lblAppPackage->setText(tr("Package: %1").arg(item->text()));
    // Keep the (long) path from widening the metadata panel and squeezing the grid
    const int available = qMax(120, m_apkGroupbox->width() - 40);
    const QString elidedPath = m_lblAppLauncherApp->fontMetrics().elidedText(apkPath, Qt::ElideMiddle, available);
    m_lblAppLauncherApp->setText(tr("Path: %1").arg(elidedPath));
    m_lblAppLauncherApp->setToolTip(apkPath);
}

void MainWindow::resetManifestInfo()
{
    m_lblAppPackage->setText(tr("Package: -"));
    m_lblAppLauncherApp->setText(tr("Is Launcher: -"));
    m_lblAppVisibleLauncher->setText(tr("Visible on Standard Launcher: -"));
    m_lblAppVisibleTvLauncher->setText(tr("Visible on TV Launcher: -"));
}

void MainWindow::onAddAppsClicked()
{
    QStringList files = QFileDialog::getOpenFileNames(this, tr("Select APK(s) to add"), QString(), tr("APK Files (*.apk)"));
    if (files.isEmpty()) return;

    QString targetDir = m_appsDirectory->currentData().toString();
    if (targetDir.isEmpty() || !targetDir.startsWith(tmpDir() + "/level2"))
        targetDir = resolveAppDir("/system/app");
    QDir().mkpath(targetDir);

    for (const auto &f : files) {
        QString dest = targetDir + "/" + QFileInfo(f).fileName();
        QFile::remove(dest);
        QFile::copy(f, dest);
    }
    onAppsDirectoryChanged(m_appsDirectory->currentIndex());
    QMessageBox::information(this, tr("Added"), tr("Added %1 APKs.").arg(files.count()));
}

void MainWindow::onRemoveAppsClicked()
{
    auto *item = m_appsViewer->currentItem();
    if (!item) return;

    const QString apkPath = item->data(Qt::UserRole).toString();
    const QString appDir  = m_appsDirectory->currentData().toString();
    const QFileInfo apk(apkPath);

    // Bundle layout: drop the whole <App> folder, flat apk: just the file
    if (apk.absolutePath() != QDir(appDir).absolutePath())
        QDir(apk.absolutePath()).removeRecursively();
    else
        QFile::remove(apkPath);

    onAppsDirectoryChanged(m_appsDirectory->currentIndex());
}

void MainWindow::onUnpackApkClicked()
{
    auto *item = m_appsViewer->currentItem();
    if (!item) return;

    const QString apkPath = item->data(Qt::UserRole).toString();
    QString outDir = tmpDir() + "/level3/apps/" + QFileInfo(apkPath).completeBaseName();
    QDir().mkpath(outDir);

    ShellRunner runner(m_logger);
    runner.run(Platform::sevenZip(), QStringList() << "x" << "-y" << ("-o" + outDir) << apkPath);
    QMessageBox::information(this, tr("Unpacked"), tr("APK extracted to:\n%1").arg(outDir));
}

void MainWindow::onRepackApkClicked()
{
    auto *item = m_appsViewer->currentItem();
    if (!item) return;

    const QString apkPath = item->data(Qt::UserRole).toString();
    QString base = QFileInfo(apkPath).completeBaseName();
    QString srcDir = tmpDir() + "/level3/apps/" + base;
    if (!QDir(srcDir).exists()) {
        QMessageBox::warning(this, tr("Error"), tr("No unpacked sources found for %1").arg(base));
        return;
    }

    ShellRunner runner(m_logger);
    runner.run(Platform::sevenZip(), QStringList() << "a" << "-tzip" << apkPath << (srcDir + "/*"));
    QMessageBox::information(this, tr("Repacked"), tr("Repacked %1").arg(item->text()));
}

// -------------------------------------------------------------
// DTB Slots
// -------------------------------------------------------------
void MainWindow::onDtbComboChanged(int index)
{
    Q_UNUSED(index);
    QString dtsName = m_dtbCombo->currentText();
    if (dtsName.trimmed().isEmpty()) {
        m_dtbEditor->clear();
        return;
    }

    QFile file(tmpDir() + "/level2/dtb/" + dtsName);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_dtbEditor->setPlainText(QString::fromUtf8(file.readAll()));
    }
}

void MainWindow::onSaveDtbClicked()
{
    QString dtsName = m_dtbCombo->currentText();
    if (dtsName.trimmed().isEmpty()) return;

    QFile file(tmpDir() + "/level2/dtb/" + dtsName);
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream out(&file);
        out.setCodec("UTF-8");
        out << m_dtbEditor->toPlainText();
        QMessageBox::information(this, tr("Saved"), tr("Saved %1").arg(dtsName));
    }
}

void MainWindow::onDiscardDtbClicked()
{
    onDtbComboChanged(m_dtbCombo->currentIndex());
}

void MainWindow::onSearchNextClicked()
{
    QString term = m_leSearchBox->text();
    if (term.isEmpty()) return;
    m_dtbEditor->find(term);
}

void MainWindow::onSearchBackClicked()
{
    QString term = m_leSearchBox->text();
    if (term.isEmpty()) return;
    m_dtbEditor->find(term, QTextDocument::FindBackward);
}

// -------------------------------------------------------------
// Keys Slots
// -------------------------------------------------------------
void MainWindow::onWriteKeysClicked()
{
    QFile file(tmpDir() + "/level1/keys.conf");
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("Error"), tr("Could not open keys.conf for writing."));
        return;
    }

    QTextStream out(&file);

    out.setCodec("UTF-8");
    for (int i = 0; i < m_keysChecklist->count(); ++i) {
        auto *item = m_keysChecklist->item(i);
        if (item->checkState() == Qt::Checked) {
            out << item->text().trimmed() << "\n";
        }
    }
    QMessageBox::information(this, tr("Keys Written"), tr("keys.conf updated successfully."));
}

void MainWindow::onOpenWorkingClicked()
{
    openFileManager(tmpDir());
}

// -------------------------------------------------------------
// Filesystem / Kernel Slots
// -------------------------------------------------------------
void MainWindow::onOpenSystemClicked()
{
    openFileManager(tmpDir() + "/level2/" + m_sysDir);
}

void MainWindow::onOpenVendorClicked()
{
    openFileManager(tmpDir() + "/level2/vendor");
}

void MainWindow::onOpenSystemBuildPropClicked()
{
    openFileManager(tmpDir() + "/level2/" + m_sysDir + "build.prop");
}

void MainWindow::onOpenVendorBuildPropClicked()
{
    openFileManager(tmpDir() + "/level2/vendor/build.prop");
}

void MainWindow::onOpenSystemKeymapClicked()
{
    openFileManager(tmpDir() + "/level2/" + m_sysDir + "usr/keylayout");
}

void MainWindow::onOpenVendorKeymapClicked()
{
    openFileManager(tmpDir() + "/level2/vendor/usr/keylayout");
}

void MainWindow::onOpenSystemGenericKlClicked()
{
    openFileManager(tmpDir() + "/level2/" + m_sysDir + "usr/keylayout/Generic.kl");
}

void MainWindow::onOpenVendorGenericKlClicked()
{
    openFileManager(tmpDir() + "/level2/vendor/usr/keylayout/Generic.kl");
}

void MainWindow::onOpenSystemRemoteConfClicked()
{
    openFileManager(tmpDir() + "/level2/" + m_sysDir + "etc/remote.conf");
}

void MainWindow::onOpenVendorRemoteConfClicked()
{
    openFileManager(tmpDir() + "/level2/vendor/etc/remote.conf");
}

void MainWindow::onOpenKernelClicked()
{
    openFileManager(tmpDir() + "/level2/boot");
}

void MainWindow::onOpenRecoveryClicked()
{
    openFileManager(tmpDir() + "/level2/recovery");
}

void MainWindow::onReplaceKernelClicked()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Select boot.img"), QString(), tr("Boot Image (*.img);;All Files (*)"));
    if (file.isEmpty()) return;

    QString dst = tmpDir() + "/level1/boot.PARTITION";
    if (!QFile::exists(dst)) dst = tmpDir() + "/level1/boot.img";
    QFile::remove(dst);
    QFile::copy(file, dst);
    QMessageBox::information(this, tr("Replaced"), tr("Kernel image replaced."));
}

void MainWindow::onReplaceRecoveryClicked()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Select recovery.img"), QString(), tr("Recovery Image (*.img);;All Files (*)"));
    if (file.isEmpty()) return;

    QString dst = tmpDir() + "/level1/recovery.PARTITION";
    if (!QFile::exists(dst)) dst = tmpDir() + "/level1/recovery.img";
    QFile::remove(dst);
    QFile::copy(file, dst);
    QMessageBox::information(this, tr("Replaced"), tr("Recovery image replaced."));
}

void MainWindow::onRootFirmwareClicked()
{
    // Inject SU binaries into system/xbin
    QString sysPath = tmpDir() + "/level2/" + m_sysDir;
    if (!QDir(sysPath).exists()) {
        QMessageBox::warning(this, tr("Error"), tr("System partition not unpacked."));
        return;
    }

    QDir().mkpath(sysPath + "xbin");
    QString suSrc = Platform::binFile("resources/su");
    if (QFile::exists(suSrc)) {
        QFile::remove(sysPath + "xbin/su");
        QFile::copy(suSrc, sysPath + "xbin/su");
        QFile::setPermissions(sysPath + "xbin/su",
            QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
            QFile::ReadGroup | QFile::ExeGroup |
            QFile::ReadOther | QFile::ExeOther);
        QMessageBox::information(this, tr("Root"), tr("su binary copied to system/xbin/su"));
    } else {
        QMessageBox::information(this, tr("Root"), tr("Rooting template placed. Use Magisk app or copy su binary."));
    }
}

void MainWindow::onRemoveRootClicked()
{
    QString sysPath = tmpDir() + "/level2/" + m_sysDir;
    QFile::remove(sysPath + "xbin/su");
    QFile::remove(sysPath + "bin/su");
    QMessageBox::information(this, tr("Unroot"), tr("Removed su binaries from system partition."));
}

void MainWindow::onBootImgForMagiskClicked()
{
    QString bootSrc = tmpDir() + "/level1/boot.PARTITION";
    if (!QFile::exists(bootSrc)) bootSrc = tmpDir() + "/level1/boot.img";
    if (!QFile::exists(bootSrc)) bootSrc = tmpDir() + "/level2/super/boot.img";

    if (!QFile::exists(bootSrc)) {
        QMessageBox::warning(this, tr("Error"), tr("No boot image found in firmware."));
        return;
    }

    QString out = QFileDialog::getSaveFileName(this, tr("Export boot.img for Magisk"),
                                             QDir::homePath() + "/boot.img", tr("Image Files (*.img)"));
    if (!out.isEmpty()) {
        QFile::remove(out);
        QFile::copy(bootSrc, out);
        QMessageBox::information(this, tr("Exported"),
            tr("boot.img saved for Magisk App patching:\n%1\n\nPatch it with the Magisk Android App, then replace it using 'Replace Kernel'.").arg(out));
    }
}

void MainWindow::onMultiWifiDriversClicked()
{
    QMessageBox::information(this, tr("WiFi Drivers"),
        tr("Multi WiFi Driver Helper:\nPlace additional kernel WiFi modules (.ko) in vendor/lib/modules/\nand register them in init.amlogic.wifi.rc or load_wifi.sh."));
}

// -------------------------------------------------------------
// Patches Slots
// -------------------------------------------------------------
void MainWindow::onPatchComboChanged(int index)
{
    Q_UNUSED(index);
    QString patchName = m_patchCombo->currentText();
    if (patchName.isEmpty()) return;

    QString patchPath = Platform::patchesDir() + "/" + patchName;
    m_lblPatchAuthor->setText(m_patcher->patchAuthor(patchPath));
    m_lblPatchVersion->setText(m_patcher->patchVersion(patchPath));
    m_lblPatchDescription->setText(m_patcher->patchDescription(patchPath));
}

void MainWindow::onApplyPatchClicked()
{
    QString patchName = m_patchCombo->currentText();
    if (patchName.isEmpty()) return;

    QString patchPath = Platform::patchesDir() + "/" + patchName;
    m_patchRTB->clear();
    m_patchRTB->append(tr("Applying patch: %1...\n").arg(patchName));

    bool ok = m_patcher->runPatch(patchPath, tmpDir());
    if (ok) {
        m_patchRTB->append(tr("\nPatch applied successfully!"));
        QMessageBox::information(this, tr("Success"), tr("Patch executed successfully."));
    } else {
        m_patchRTB->append(tr("\nPatch failed or partially failed."));
        QMessageBox::warning(this, tr("Patch"), tr("Patch execution completed with errors. See log for details."));
    }
    loadInfo();
}

void MainWindow::onAddPatchLabelClicked()
{
    openFileManager(Platform::patchesDir());
}

void MainWindow::onPatchOutputAppended(const QString &text)
{
    m_patchRTB->append(text);
}

// -------------------------------------------------------------
// File Search Slots
// -------------------------------------------------------------
void MainWindow::onSearchTextChanged(const QString &text)
{
    m_filesList->clear();
    if (text.trimmed().length() < 2) return;

    QString l2 = tmpDir() + "/level2";
    if (!QDir(l2).exists()) return;

    QDirIterator it(l2, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    int count = 0;
    while (it.hasNext() && count < 200) {
        QString f = it.next();
        if (QFileInfo(f).fileName().contains(text, Qt::CaseInsensitive)) {
            m_filesList->addItem(f);
            count++;
        }
    }
}

// -------------------------------------------------------------
// Locale Slot
// -------------------------------------------------------------
void MainWindow::onLocaleComboChanged(int index)
{
    Q_UNUSED(index);
}

// -------------------------------------------------------------
// Dev Settings & Menu Slots
// -------------------------------------------------------------
void MainWindow::onDevSettingsChanged()
{
    QSettings settings("ROM Customization Toolkit", "Settings");
    settings.setValue("PythonSuperUtils", m_actPythonSuperUtils->isChecked());
    settings.setValue("NewDtbTools", m_actNewDtbTools->isChecked());
    settings.setValue("NewLogoTools", m_actNewLogoTools->isChecked());
    settings.setValue("PythonImgExtractor", m_actPythonImgExtractor->isChecked());
    settings.setValue("ArchMakeExt4fs", m_actArchMakeExt4fs->isChecked());
    settings.setValue("PythonSimg2img", m_actPythonSimg2img->isChecked());
    settings.setValue("UseLegacyAWTools", m_actUseLegacyAWTools->isChecked());
    settings.setValue("UseNewRKTools", m_actUseNewRKTools->isChecked());
}

void MainWindow::applyDevSettings()
{
    QSettings settings("ROM Customization Toolkit", "Settings");
    // Block toggled() while loading, otherwise each setChecked() would re-save the
    // other (not yet loaded) options and overwrite the stored values.
    for (QAction *a : {m_actPythonSuperUtils, m_actNewDtbTools, m_actNewLogoTools, m_actPythonImgExtractor,
                       m_actArchMakeExt4fs, m_actPythonSimg2img, m_actUseLegacyAWTools, m_actUseNewRKTools})
        a->blockSignals(true);
    m_actPythonSuperUtils->setChecked(settings.value("PythonSuperUtils", false).toBool());
    m_actNewDtbTools->setChecked(settings.value("NewDtbTools", true).toBool());
    m_actNewLogoTools->setChecked(settings.value("NewLogoTools", true).toBool());
    m_actPythonImgExtractor->setChecked(settings.value("PythonImgExtractor", false).toBool());
    m_actArchMakeExt4fs->setChecked(settings.value("ArchMakeExt4fs", true).toBool());
    m_actPythonSimg2img->setChecked(settings.value("PythonSimg2img", false).toBool());
    m_actUseLegacyAWTools->setChecked(settings.value("UseLegacyAWTools", false).toBool());
    m_actUseNewRKTools->setChecked(settings.value("UseNewRKTools", true).toBool());
    for (QAction *a : {m_actPythonSuperUtils, m_actNewDtbTools, m_actNewLogoTools, m_actPythonImgExtractor,
                       m_actArchMakeExt4fs, m_actPythonSimg2img, m_actUseLegacyAWTools, m_actUseNewRKTools})
        a->blockSignals(false);
}

void MainWindow::onLogsClicked()
{
    openFileManager(Platform::logsDir() + "/app.log");
}

void MainWindow::onTutorialItemClicked()
{
    TutorialDialog dlg(this);
    dlg.exec();
}

void MainWindow::checkResources()
{
    // Verify system installed tools
    auto toolFound = [](const QString &tool) {
        if (tool == "7z")        return !Platform::findSevenZip().isEmpty();
        if (tool == "python3")   return !Platform::findTool("python3").isEmpty()
                                        || (Platform::isWindows && !Platform::findTool("python").isEmpty());
        if (tool == "mkfs.ext4") return !Platform::findTool("mkfs.ext4").isEmpty()
                                        || !Platform::findTool("mke2fs").isEmpty();
        return !Platform::findTool(tool).isEmpty();
    };

    QStringList systemTools = {"7z", "dtc", "simg2img", "mkfs.ext4", "python3"};
    QString status = tr("Installed System Tools:\n");
    for (const auto &tool : systemTools) {
        bool found = toolFound(tool);
        status += "  - " + tool + ": " + (found ? tr("FOUND") : tr("NOT FOUND (Install via package manager)")) + "\n";
    }

    status += tr("\nBin Tools:\n");
    QStringList binTools = {"aml_image_pack", "rk_tools", "lpunpack"};
    for (const auto &tool : binTools) {
        bool found = !Platform::findTool(tool).isEmpty();
        status += "  - " + tool + ": " + (found ? tr("FOUND") : tr("MISSING (Will be auto-downloaded or compiled)")) + "\n";
    }

    QMessageBox::information(this, tr("Tool Resource Check"), status);
}
