#pragma once
#include <QMap>
#include <QString>
#include <QStringList>
#include "logger.h"
#include "shellrunner.h"

/**
 * Unpacker – ports UnpackerV4.cs to Linux/Qt.
 *
 * Firmware containers (level 1 unpack / pack) are handled by the bundled open-source tools
 * built from New_Tools/ by scripts/build_tools.sh. They exist for both Linux and Windows and
 * are resolved through Platform::findTool(): bin/ first (".exe" is added on Windows), then PATH.
 *   Rockchip RKFW/RKAF/SSFW  → afptool-rs unpack | pack-rkaf | pack-rkfw   (Apache-2.0)
 *   Amlogic burning image    → ampack unpack | pack                        (AGPL-3.0)
 *   Allwinner IMAGEWTY       → imagewty-tool extract | repack              (GPL-3.0)
 *
 * The remaining level 2/3 helpers still use system tools:
 *   7za.exe               → 7z (system) or bin/7za
 *   e2fs tools            → e2fsck, resize2fs, ext4fuse (system/downloaded)
 *   android-tools / AIK   → bin/aik (downloaded)
 *   lpunpack              → bin/lpunpack (downloaded)
 *   dtc                   → dtc (system package device-tree-compiler)
 *   img2sdat.py           → bin/img2sdat.py (GPL Python script)
 *   mkfs.ext4             → mkfs.ext4 (system: e2fsprogs)
 *   simg2img/img2simg     → simg2img/img2simg (system: android-sdk-platform-tools or bin/)
 */
class Unpacker {
public:
    enum class ImgType { Amlogic, Rockchip, Allwinner, None };

    explicit Unpacker(Logger *logger);

    // --- Level 1: split upgrade package into partitions ---
    bool unpackVendorImage(const QString &inputFile, const QString &outputDir);
    bool unpackUpgradeZip(const QString &inputFile, const QString &outputDir);

    // --- Level 2: unpack individual partitions ---
    bool unpackExt4(const QString &partitionFile, const QString &outputDir);
    bool unpackSuper(const QString &partitionFile, const QString &outputDir);
    bool unpackKernel(const QString &bootImg, const QString &outputDir);
    bool unpackDtb(const QString &dtbFile, const QString &outputDir);

    // --- Level 3: UI asset unpacking ---
    bool unpackLogo(const QString &logoFile, const QString &outputDir);
    bool unpackBootanimation(const QString &zipFile, const QString &outputDir);
    bool unpackBootVideo(const QString &videoDir, const QString &outputDir);
    bool unpackLauncher(const QString &level2Dir);

    // --- Repacking ---
    bool repackExt4(const QString &inputDir, const QString &outputFile,
                    const QString &partitionName, bool useBrotli = false);
    bool repackVendorImage(const QString &level1Dir, const QString &outputFile,
                           bool addPlatformConf = false, bool specifyChipset = false,
                           const QString &chipset = QString());
    bool repackZipFile(const QString &level1Dir, const QString &outputFile,
                       int compressionLevel = 5);
    bool repackKernel(const QString &inputDir, const QString &outputFile);
    bool repackDtb(const QString &dtsDir, const QString &outputFile);
    bool repackLogo(const QString &logoDir, const QString &outputFile);

    // --- Utility ---
    bool copyFilesRecursively(const QString &src, const QString &dst);
    QString shellOutput() const { return m_shell.lastOutput(); }

    // Human readable reason for the last failed unpack/repack (shown in the UI)
    QString lastError() const { return m_lastError; }

    // Path to the app's bin/ directory (set by MainWindow)
    QString binDir;

private:
    Logger      *m_logger;
    ShellRunner  m_shell;
    QString      m_logHead = "[UNPACKER] : ";
    QString      m_lastError;

    // Record and log a failure reason, so the UI can tell the user what went wrong
    void fail(const QString &reason);

    // Firmware container formats recognised by the bundled tools in New_Tools/
    enum class Container {
        Unknown,
        RockchipRkaf,
        RockchipRkfw,
        RockchipSsfw,
        Amlogic,
        Allwinner
    };

    // Helper: find tool – first check bin/, then system PATH
    QString findTool(const QString &toolName) const;
    QString sevenZip() const;
    QString python3() const;

    // Resolve a bundled tool and log a build hint when it is missing
    QString toolPath(const QString &toolName) const;

    // Container sniffing / bookkeeping
    static Container sniffContainer(const QString &file);
    static Container containerFromName(const QString &name);
    static void writeFirmwareInfo(const QString &dir, const QString &type,
                                  const QMap<QString, QString> &extra = {});
    static QString readFirmwareInfo(const QString &dir, const QString &key);

    // Bundled open-source tools
    bool unpackRockchip(Container kind, const QString &inputFile, const QString &outputDir);
    bool unpackAmlogicImage(const QString &inputFile, const QString &outputDir);
    bool unpackAllwinnerImage(const QString &inputFile, const QString &outputDir);
    bool repackRockchip(Container kind, const QString &level1Dir, const QString &outputFile,
                        bool specifyChipset, const QString &chipset);
    bool repackAmlogicImage(const QString &level1Dir, const QString &outputFile);
    bool repackAllwinnerImage(const QString &level1Dir, const QString &outputFile);

    // Run an AIK-style script: .sh via bash, .bat/.cmd/.exe directly
    void runScript(const QString &script, const QStringList &args, const QString &workDir);
};
