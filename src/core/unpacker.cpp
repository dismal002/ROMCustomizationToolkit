#include "unpacker.h"
#include "platform.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDirIterator>
#include <QProcess>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QMap>
#include <QTextStream>
#include <QtConcurrent/QtConcurrentRun>

Unpacker::Unpacker(Logger *logger)
    : m_logger(logger), m_shell(logger)
{}

// ---------------------------------------------------------------------------
// Tool resolution helpers
// ---------------------------------------------------------------------------

QString Unpacker::findTool(const QString &toolName) const
{
    // bin/ (with .exe/.bat on Windows), then the system PATH
    const QString found = Platform::findTool(toolName);
    if (!found.isEmpty()) return found;
    return toolName; // fallback – let the OS deal with it
}

void Unpacker::runScript(const QString &script, const QStringList &args, const QString &workDir)
{
    if (script.endsWith(".sh")) {
        const QString bash = Platform::bash();
        if (bash.isEmpty()) {
            m_logger->log(m_logHead + "bash not found – cannot run " + script +
                          " (install Git for Windows or provide a .bat version of the script)");
            return;
        }
        m_shell.run(bash, QStringList{script} + args, workDir);
    } else {
        m_shell.run(script, args, workDir);   // .bat / .cmd / executable
    }
}

QString Unpacker::sevenZip() const
{
    return Platform::sevenZip();
}

QString Unpacker::python3() const
{
    return Platform::python();
}

void Unpacker::fail(const QString &reason)
{
    m_lastError = reason;
    m_logger->log(m_logHead + reason);
}

QString Unpacker::toolPath(const QString &toolName) const
{
    const QString path = Platform::findTool(toolName);
    if (path.isEmpty() || !QFileInfo(path).isFile()) {
        const_cast<Unpacker *>(this)->fail(toolName + " not found – build the bundled tools "
                                          "with scripts/build_tools.sh (output goes to bin/)");
        return QString();
    }
    m_logger->log(m_logHead + "Using " + toolName + ": " + path);
    return path;
}

// ---------------------------------------------------------------------------
// Firmware container detection / bookkeeping
// ---------------------------------------------------------------------------

Unpacker::Container Unpacker::sniffContainer(const QString &file)
{
    QFile f(file);
    if (!f.open(QIODevice::ReadOnly)) return Container::Unknown;
    const QByteArray head = f.read(64);

    if (head.startsWith("RKFW"))    return Container::RockchipRkfw;
    if (head.startsWith("RKAF"))    return Container::RockchipRkaf;
    if (head.startsWith("SSFW"))    return Container::RockchipSsfw;
    if (head.startsWith("IMAGEWTY")) return Container::Allwinner;

    // Amlogic burning image: magic 0x27b51956. The header is
    // {crc, version, magic, image_size, align, item_count, reserve}, so the magic
    // normally sits at offset 8 – older packaging may put it at offset 0.
    auto isAmlogicMagic = [&head](int offset) {
        return head.size() >= offset + 4 &&
               static_cast<uchar>(head[offset])     == 0x56 &&
               static_cast<uchar>(head[offset + 1]) == 0x19 &&
               static_cast<uchar>(head[offset + 2]) == 0xb5 &&
               static_cast<uchar>(head[offset + 3]) == 0x27;
    };
    if (isAmlogicMagic(8) || isAmlogicMagic(0)) return Container::Amlogic;

    return Container::Unknown;
}

Unpacker::Container Unpacker::containerFromName(const QString &name)
{
    if (name == "rockchip-rkaf") return Container::RockchipRkaf;
    if (name == "rockchip-rkfw") return Container::RockchipRkfw;
    if (name == "rockchip-ssfw") return Container::RockchipSsfw;
    if (name == "amlogic")       return Container::Amlogic;
    if (name == "allwinner")     return Container::Allwinner;
    return Container::Unknown;
}

void Unpacker::writeFirmwareInfo(const QString &dir, const QString &type,
                                 const QMap<QString, QString> &extra)
{
    QFile f(dir + "/.firmware-info");
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream out(&f);
    out.setCodec("UTF-8");
    out << "type=" << type << "\n";
    for (auto it = extra.constBegin(); it != extra.constEnd(); ++it)
        out << it.key() << "=" << it.value() << "\n";
}

QString Unpacker::readFirmwareInfo(const QString &dir, const QString &key)
{
    QFile f(dir + "/.firmware-info");
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return QString();
    QTextStream in(&f);
    in.setCodec("UTF-8");
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.startsWith(key + "=")) return line.mid(key.length() + 1);
    }
    return QString();
}

// ---------------------------------------------------------------------------
// Level 1 – split upgrade package
// ---------------------------------------------------------------------------

bool Unpacker::unpackVendorImage(const QString &inputFile, const QString &outputDir)
{
    m_logger->log(m_logHead + "Unpacking vendor image: " + inputFile);
    m_lastError.clear();
    QDir().mkpath(outputDir);

    switch (sniffContainer(inputFile)) {
    case Container::RockchipRkaf:
    case Container::RockchipRkfw:
    case Container::RockchipSsfw:
        return unpackRockchip(sniffContainer(inputFile), inputFile, outputDir);
    case Container::Amlogic:
        return unpackAmlogicImage(inputFile, outputDir);
    case Container::Allwinner:
        return unpackAllwinnerImage(inputFile, outputDir);
    case Container::Unknown:
        break;
    }

    // Unknown container – some vendor images are zip based, so fall back to 7z
    QFile head(inputFile);
    QString magic = "?";
    if (head.open(QIODevice::ReadOnly)) magic = QString::fromLatin1(head.read(8).toHex(' '));
    m_logger->log(m_logHead + "Unrecognised container (magic " + magic +
                              "), falling back to 7z extraction");
    m_shell.run(sevenZip(), QString("x \"%1\" -o\"%2\" -y").arg(inputFile, outputDir));
    if (QDir(outputDir).entryList(QDir::Files | QDir::NoDotAndDotDot).count() > 0) return true;

    fail(QString("unsupported firmware container (magic %1) – no bundled tool handles it").arg(magic));
    return false;
}

// ---------------------------------------------------------------------------
// Container unpacking – bundled open-source tools (New_Tools/)
// ---------------------------------------------------------------------------

bool Unpacker::unpackRockchip(Container kind, const QString &inputFile, const QString &outputDir)
{
    const QString afp = toolPath("afptool-rs");
    if (afp.isEmpty()) return false;

    // RKFW wraps an RKAF update image, so the partitions live one level deeper.
    const bool rkfw = (kind == Container::RockchipRkfw);
    const QString work = rkfw ? outputDir + "/.rkfw-stage" : outputDir;
    if (rkfw) {
        ShellRunner::deleteDirectory(work);
        QDir().mkpath(work);
    }

    if (m_shell.runGetCode(afp, QStringList{"unpack", inputFile, work}) != 0) {
        fail("afptool-rs could not unpack " + QFileInfo(inputFile).fileName() + ": " +
             m_shell.lastOutput().trimmed().section('\n', -1));
        if (rkfw) ShellRunner::deleteDirectory(work);
        return false;
    }

    // The tool prints "manufacturer: X", "model: Y", "family: Z" – keep them for repacking
    QMap<QString, QString> info;
    info.insert("tool", "afptool-rs");
    for (const QString &line : m_shell.lastOutput().split('\n')) {
        const int colon = line.indexOf(':');
        if (colon <= 0) continue;
        const QString key = line.left(colon).trimmed();
        if (key == "model" || key == "manufacturer" || key == "family" || key == "version")
            info.insert(key, line.mid(colon + 1).trimmed());
    }

    if (rkfw) {
        const QString embedded = work + "/embedded-update.img";
        if (!QFile::exists(embedded)) {
            m_logger->log(m_logHead + "RKFW image contains no embedded RKAF update image");
            ShellRunner::deleteDirectory(work);
            return false;
        }
        if (m_shell.runGetCode(afp, QStringList{"unpack", embedded, outputDir}) != 0) {
            fail("afptool-rs could not unpack the embedded RKAF image: " +
                 m_shell.lastOutput().trimmed().section('\n', -1));
            return false;
        }

        // BOOT and the original header rebuild the RKFW wrapper when repacking
        QDir().mkpath(outputDir + "/.rkfw");
        QFile::copy(work + "/BOOT", outputDir + "/.rkfw/BOOT");
        QFile::copy(work + "/rkfw-header.bin", outputDir + "/.rkfw/rkfw-header.bin");
        writeFirmwareInfo(outputDir, "rockchip-rkfw", info);
        ShellRunner::deleteDirectory(work);
        return true;
    }

    if (kind == Container::RockchipSsfw) {
        // Multi-storage container: every embedded RKFW gets its own partition folder
        const auto images = QDir(outputDir).entryInfoList({"firmware-*.img"}, QDir::Files);
        for (const QFileInfo &image : images) {
            const QString sub = outputDir + "/" + image.completeBaseName();
            QDir().mkpath(sub);
            if (m_shell.runGetCode(afp, QStringList{"unpack", image.absoluteFilePath(), sub}) != 0)
                m_logger->log(m_logHead + "afptool-rs could not unpack " + image.fileName());
        }
        writeFirmwareInfo(outputDir, "rockchip-ssfw", info);
        return true;
    }

    writeFirmwareInfo(outputDir, "rockchip-rkaf", info);
    return true;
}

bool Unpacker::unpackAmlogicImage(const QString &inputFile, const QString &outputDir)
{
    const QString amp = toolPath("ampack");
    if (amp.isEmpty()) return false;

    // ampack recreates the output folder and writes <stem>.<type> files (boot.PARTITION, …)
    if (m_shell.runGetCode(amp, QStringList{"unpack", inputFile, outputDir}) != 0) {
        fail("ampack could not unpack " + QFileInfo(inputFile).fileName() + ": " +
             m_shell.lastOutput().trimmed().section('\n', -1));
        return false;
    }

    writeFirmwareInfo(outputDir, "amlogic");
    return QDir(outputDir).entryList(QDir::Files | QDir::NoDotAndDotDot).count() > 0;
}

bool Unpacker::unpackAllwinnerImage(const QString &inputFile, const QString &outputDir)
{
    const QString wty = toolPath("imagewty-tool");
    if (wty.isEmpty()) return false;

    // imagewty-tool always extracts into "<image>.dump" next to the image, so stage a copy
    const QString staging = outputDir + "/.wty-stage";
    ShellRunner::deleteDirectory(staging);
    QDir().mkpath(staging);
    const QString staged = staging + "/" + QFileInfo(inputFile).fileName();
    if (!QFile::copy(inputFile, staged)) {
        m_logger->log(m_logHead + "could not stage " + inputFile + " for imagewty-tool");
        ShellRunner::deleteDirectory(staging);
        return false;
    }

    if (m_shell.runGetCode(wty, QStringList{"extract", staged}, staging) != 0) {
        fail("imagewty-tool could not extract " + QFileInfo(inputFile).fileName() + ": " +
             m_shell.lastOutput().trimmed().section('\n', -1));
        ShellRunner::deleteDirectory(staging);
        return false;
    }

    QString dumpDir;
    const auto dumps = QDir(staging).entryInfoList({"*.dump"}, QDir::Dirs | QDir::NoDotAndDotDot);
    if (!dumps.isEmpty()) dumpDir = dumps.first().absoluteFilePath();
    if (dumpDir.isEmpty()) {
        m_logger->log(m_logHead + "imagewty-tool produced no dump folder for " + inputFile);
        ShellRunner::deleteDirectory(staging);
        return false;
    }

    copyFilesRecursively(dumpDir, outputDir);
    ShellRunner::deleteDirectory(staging);

    writeFirmwareInfo(outputDir, "allwinner");
    return QDir(outputDir).entryList(QDir::Files | QDir::NoDotAndDotDot).count() > 0;
}

bool Unpacker::unpackUpgradeZip(const QString &inputFile, const QString &outputDir)
{
    m_logger->log(m_logHead + "Unpacking upgrade zip: " + inputFile);
    QDir().mkpath(outputDir);

    // Standard zip – use 7z
    m_shell.run(sevenZip(), QString("x \"%1\" -o\"%2\" -y").arg(inputFile, outputDir));

    // Check if it's an OTA-style zip (has payload.bin) → use payload-dumper
    if (QFile::exists(outputDir + "/payload.bin")) {
        QString payloadDumper = findTool("payload-dumper-go");
        if (payloadDumper.isEmpty() || !QFile::exists(payloadDumper))
            payloadDumper = findTool("payload_dumper");
        if (!payloadDumper.isEmpty() && QFile::exists(payloadDumper))
            m_shell.run(payloadDumper, QString("\"%1\"").arg(outputDir + "/payload.bin"));
    }

    return QDir(outputDir).entryList(QDir::Files | QDir::NoDotAndDotDot).count() > 0;
}

// ---------------------------------------------------------------------------
// Level 2 – unpack partitions
// ---------------------------------------------------------------------------

bool Unpacker::unpackExt4(const QString &partitionFile, const QString &outputDir)
{
    m_logger->log(m_logHead + "Unpacking ext4: " + partitionFile);
    QString partName = QFileInfo(partitionFile).completeBaseName()
                         .replace("_a", "").replace(".PARTITION", "");
    // The caller already passes the folder for this partition (level2/<partition>),
    // so extract straight into it – appending partName again buried build.prop
    // one level deeper than loadInfo()/getProp() look for it.
    const QString destDir = outputDir;
    QDir().mkpath(destDir);

    // Convert sparse → raw if needed (simg2img)
    QString workFile = partitionFile;
    QString tmpRaw   = outputDir + "/.tmp_" + partName + ".raw";

    // Detect sparse image by magic bytes
    {
        QFile f(partitionFile);
        if (f.open(QIODevice::ReadOnly)) {
            QByteArray magic = f.read(4);
            f.close();
            // Sparse magic is 3a ff 26 ed (bytes as stored in the file)
            if (magic == QByteArray("\x3a\xff\x26\xed", 4)) {
                // Sparse image – convert
                QString simg2img = findTool("simg2img");
                if (QFile::exists(simg2img)) {
                    m_shell.run(simg2img, QString("\"%1\" \"%2\"").arg(partitionFile, tmpRaw));
                    workFile = tmpRaw;
                }
            }
        }
    }

    // Mount and copy OR use debugfs/e2tools
    // Use debugfs (part of e2fsprogs, GPL) to extract files
    QString debugfs = findTool("debugfs");
    if (QFile::exists(debugfs)) {
        // debugfs rdump command
        m_shell.run(debugfs, QStringList{"-R", QString("rdump / \"%1\"").arg(destDir), workFile});
    } else {
        // Fallback: use 7z (ext4 support limited but works for uncompressed)
        m_shell.run(sevenZip(), QString("x \"%1\" -o\"%2\" -y").arg(workFile, destDir));
    }

    // Cleanup temp raw file
    if (workFile == tmpRaw) QFile::remove(tmpRaw);

    return QDir(destDir).entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot).count() > 0;
}

bool Unpacker::unpackSuper(const QString &partitionFile, const QString &outputDir)
{
    m_logger->log(m_logHead + "Unpacking super partition: " + partitionFile);
    m_lastError.clear();
    QDir().mkpath(outputDir);

    const QString lpunpack = Platform::findTool("lpunpack");
    if (lpunpack.isEmpty() || !QFileInfo(lpunpack).isFile()) {
        fail("lpunpack not found (install android-sdk-libsparse-utils / python-lpunpack)");
        return false;
    }

    // lpunpack only reads raw images – convert an Android sparse super image first
    QString image = partitionFile;
    QString sparseRaw;
    {
        QFile f(partitionFile);
        if (f.open(QIODevice::ReadOnly)) {
            const QByteArray magic = f.read(4);
            if (magic == QByteArray("\x3a\xff\x26\xed", 4)) {
                const QString simg2img = Platform::findTool("simg2img");
                if (!simg2img.isEmpty() && QFileInfo(simg2img).isFile()) {
                    sparseRaw = outputDir + "/.super.raw";
                    if (m_shell.runGetCode(simg2img, QStringList{partitionFile, sparseRaw}) == 0 &&
                        QFile::exists(sparseRaw)) {
                        image = sparseRaw;
                    } else {
                        m_logger->log(m_logHead + "simg2img could not convert " + partitionFile);
                        QFile::remove(sparseRaw);
                        sparseRaw.clear();
                    }
                }
            }
        }
    }

    // Two CLI flavours exist: `lpunpack <image> <outdir>` (AOSP) and
    // `lpunpack -o <outdir> <image>` (python-lpunpack).
    int code = m_shell.runGetCode(lpunpack, QStringList{image, outputDir});
    if (code != 0 ||
        QDir(outputDir).entryList(QDir::Files | QDir::NoDotAndDotDot).isEmpty()) {
        code = m_shell.runGetCode(lpunpack, QStringList{"-o", outputDir, image});
    }

    if (!sparseRaw.isEmpty()) QFile::remove(sparseRaw);

    if (code != 0) {
        fail("lpunpack could not split " + QFileInfo(partitionFile).fileName() + ": " +
             m_shell.lastOutput().trimmed().section('\n', -1));
        return false;
    }
    return QDir(outputDir).entryList(QDir::Files | QDir::NoDotAndDotDot).count() > 0;
}

bool Unpacker::unpackKernel(const QString &bootImg, const QString &outputDir)
{
    m_logger->log(m_logHead + "Unpacking kernel/boot image: " + bootImg);
    QDir().mkpath(outputDir);

    // Use Android Image Kitchen (AIK) or magiskboot
    const QString magiskboot = findTool("magiskboot");
    const QString aikScript  = Platform::findScript("aik/unpackimg");

    if (QFile::exists(magiskboot)) {
        // magiskboot unpack
        QFile::copy(bootImg, outputDir + "/boot.img");
        m_shell.run(magiskboot, "unpack boot.img", outputDir);
    } else if (!aikScript.isEmpty()) {
        runScript(aikScript, {bootImg}, outputDir);
    } else {
        // Fallback: try 7z extraction
        m_shell.run(sevenZip(), QString("x \"%1\" -o\"%2\" -y").arg(bootImg, outputDir));
    }

    return QDir(outputDir).entryList(QDir::Files | QDir::NoDotAndDotDot).count() > 0;
}

bool Unpacker::unpackDtb(const QString &dtbFile, const QString &outputDir)
{
    m_logger->log(m_logHead + "Unpacking DTB: " + dtbFile);
    QDir().mkpath(outputDir);

    QString dtc = findTool("dtc");
    if (!QFile::exists(dtc)) {
        m_logger->log(m_logHead + "dtc not found (install device-tree-compiler)");
        // Copy raw DTB files as fallback
        QFile::copy(dtbFile, outputDir + "/" + QFileInfo(dtbFile).fileName());
        return true;
    }

    QString outDts = outputDir + "/" + QFileInfo(dtbFile).baseName() + ".dts";

    // Try single DTB
    m_shell.run(dtc, QString("-I dtb -O dts -o \"%1\" \"%2\"").arg(outDts, dtbFile));
    if (QFile::exists(outDts)) return true;

    // Multi-DTB: use dtbdump or split first
    QString dtbDump = findTool("dtbdump");
    if (QFile::exists(dtbDump)) {
        m_shell.run(dtbDump, QString("\"%1\" \"%2\"").arg(dtbFile, outputDir));
    } else {
        // Copy raw as last resort
        QFile::copy(dtbFile, outputDir + "/" + QFileInfo(dtbFile).fileName());
    }

    return true;
}

// ---------------------------------------------------------------------------
// Level 3 – UI assets
// ---------------------------------------------------------------------------

bool Unpacker::unpackLogo(const QString &logoFile, const QString &outputDir)
{
    m_logger->log(m_logHead + "Unpacking logo: " + logoFile);
    QDir().mkpath(outputDir);

    // Amlogic logo.img is a custom format – use aml_logo_unpack from bin/
    QString amlLogo = findTool("aml_logo_unpack");

    if (QFile::exists(amlLogo)) {
        m_shell.run(amlLogo, QString("\"%1\" \"%2\"").arg(logoFile, outputDir));
        return true;
    }

    // Fallback: Allwinner boot-resource.fex is a IMGPACK container
    // Try 7z extraction – works for some formats
    m_shell.run(sevenZip(), QString("x \"%1\" -o\"%2\" -y").arg(logoFile, outputDir));
    return QDir(outputDir).entryList(QDir::Files).count() > 0;
}

bool Unpacker::unpackBootanimation(const QString &zipFile, const QString &outputDir)
{
    m_logger->log(m_logHead + "Unpacking bootanimation: " + zipFile);
    // Bootanimation is a standard zip
    QDir().mkpath(outputDir);
    m_shell.run(sevenZip(), QString("x \"%1\" -o\"%2\" -y").arg(zipFile, outputDir));
    return QFile::exists(outputDir + "/desc.txt");
}

bool Unpacker::unpackBootVideo(const QString &videoDir, const QString &outputDir)
{
    m_logger->log(m_logHead + "Copying bootvideo from: " + videoDir);
    QDir().mkpath(outputDir);
    return copyFilesRecursively(videoDir, outputDir);
}

bool Unpacker::unpackLauncher(const QString &level2Dir)
{
    m_logger->log(m_logHead + "Unpacking launcher APKs");
    QString apktool = findTool("apktool.jar");
    if (!QFile::exists(apktool)) apktool = findTool("apktool");
    if (!QFile::exists(apktool)) {
        m_logger->log(m_logHead + "apktool not found – cannot unpack launcher");
        return false;
    }

    // Find launcher APK candidates
    QStringList launcherNames = {"Launcher", "launcher", "TVLauncher",
                                  "Launcher3", "QuickStep"};
    QDirIterator it(level2Dir, {"*.apk"}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString apk = it.next();
        QString name = QFileInfo(apk).baseName();
        for (const QString &ln : launcherNames) {
            if (name.contains(ln, Qt::CaseInsensitive)) {
                QString outDir = QDir(level2Dir).absolutePath() + "/../level3/apps/" +
                                 apk.mid(level2Dir.length() + 1);
                QDir().mkpath(outDir);
                m_shell.run("java", QString("-jar \"%1\" d \"%2\" -o \"%3\" -f")
                                .arg(apktool, apk, outDir));
                break;
            }
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// Repacking
// ---------------------------------------------------------------------------

bool Unpacker::repackExt4(const QString &inputDir, const QString &outputFile,
                          const QString &partitionName, bool useBrotli)
{
    m_logger->log(m_logHead + "Repacking ext4: " + partitionName);

    // Calculate required size (portable replacement for `du -sb`)
    qint64 sizeBytes = Platform::directorySize(inputDir);
    qint64 blockCount = (sizeBytes / 4096 + 512) * 110 / 100; // 10% headroom

    // make_ext4fs (AOSP tool, preferred) -> mkfs.ext4 -> mke2fs (what e2fsprogs ships on Windows)
    QString makeExt4 = findTool("make_ext4fs");
    const bool haveMakeExt4 = QFile::exists(makeExt4);
    QString mkfs = findTool("mkfs.ext4");
    QStringList mkfsExtra;
    if (!QFile::exists(mkfs)) {
        mkfs = findTool("mke2fs");
        mkfsExtra = QStringList{"-t", "ext4"};
    }
    if (!haveMakeExt4 && !QFile::exists(mkfs)) {
        m_logger->log(m_logHead + "make_ext4fs/mkfs.ext4/mke2fs not found");
        return false;
    }

    if (haveMakeExt4) {
        m_shell.run(makeExt4,
            QStringList{"-s", "-l", QString::number(blockCount * 4096), "-a", partitionName,
                        outputFile, inputDir});
    } else {
        QFile::remove(outputFile);
        // Create the empty image file (portable replacement for `dd if=/dev/zero`)
        QFile img(outputFile);
        if (img.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            img.resize(blockCount * 4096);
            img.close();
        }
        m_shell.run(mkfs, mkfsExtra + QStringList{"-L", partitionName, "-d", inputDir, outputFile});
    }

    if (!QFile::exists(outputFile)) return false;

    // Optionally convert to sparse
    QString img2simg = findTool("img2simg");
    if (QFile::exists(img2simg)) {
        QString sparse = outputFile + ".sparse";
        m_shell.run(img2simg, QString("\"%1\" \"%2\"").arg(outputFile, sparse));
        if (QFile::exists(sparse)) {
            QFile::remove(outputFile);
            QFile::rename(sparse, outputFile);
        }
    }

    // Optionally brotli compress
    if (useBrotli) {
        QString brotli = findTool("brotli");
        if (QFile::exists(brotli))
            m_shell.run(brotli, QString("-q 6 \"%1\"").arg(outputFile));
    }

    return QFile::exists(outputFile);
}

bool Unpacker::repackVendorImage(const QString &level1Dir, const QString &outputFile,
                                  bool addPlatformConf, bool specifyChipset,
                                  const QString &chipset)
{
    Q_UNUSED(addPlatformConf)
    m_logger->log(m_logHead + "Repacking vendor image...");
    m_lastError.clear();

    Container kind = containerFromName(readFirmwareInfo(level1Dir, "type"));

    // Projects unpacked by older builds carry no .firmware-info – recognise the layout instead
    if (kind == Container::Unknown) {
        if (QFile::exists(level1Dir + "/DDR.USB"))
            kind = Container::Amlogic;
        else if (QFile::exists(level1Dir + "/image.cfg"))
            kind = Container::Allwinner;
        else if (QFile::exists(level1Dir + "/package-file"))
            kind = Container::RockchipRkaf;
    }

    // Last resort: imported C# projects record the vendor in .config/project.conf
    if (kind == Container::Unknown) {
        QFile conf(level1Dir + "/../.config/project.conf");
        if (conf.open(QIODevice::ReadOnly)) {
            for (const QString &line : QString(conf.readAll()).split('\n')) {
                if (!line.startsWith("FirmwareBase|")) continue;
                const QString vendor = line.split('|').last().trimmed().toUpper();
                if (vendor == "AMLOGIC")        kind = Container::Amlogic;
                else if (vendor == "ALLWINNER")  kind = Container::Allwinner;
                else if (vendor == "ROCKCHIP")   kind = Container::RockchipRkaf;
            }
        }
    }

    bool ok = false;
    switch (kind) {
    case Container::RockchipRkaf:
    case Container::RockchipRkfw:
    case Container::RockchipSsfw:
        ok = repackRockchip(kind, level1Dir, outputFile, specifyChipset, chipset);
        break;
    case Container::Amlogic:
        ok = repackAmlogicImage(level1Dir, outputFile);
        break;
    case Container::Allwinner:
        ok = repackAllwinnerImage(level1Dir, outputFile);
        break;
    case Container::Unknown:
        fail("cannot tell which firmware container " + level1Dir +
             " belongs to – unpack the original image again");
        return false;
    }

    // The bundled tools report failures through their exit code, so a non-empty result
    // is enough – small images are legitimate.
    return ok && QFile::exists(outputFile) && QFileInfo(outputFile).size() > 0;
}

// ---------------------------------------------------------------------------
// Container repacking – bundled open-source tools (New_Tools/)
// ---------------------------------------------------------------------------

bool Unpacker::repackRockchip(Container kind, const QString &level1Dir,
                              const QString &outputFile, bool specifyChipset,
                              const QString &chipset)
{
    const QString afp = toolPath("afptool-rs");
    if (afp.isEmpty()) return false;

    if (kind == Container::RockchipSsfw) {
        fail("afptool-rs cannot build SSFW multi-storage images – repack the individual "
             "firmware-*.img images instead");
        return false;
    }

    QString model        = readFirmwareInfo(level1Dir, "model");
    QString manufacturer = readFirmwareInfo(level1Dir, "manufacturer");
    if (model.isEmpty())        model = QStringLiteral("UNKNOWN");
    if (manufacturer.isEmpty()) manufacturer = model;

    QFile::remove(outputFile);

    if (kind == Container::RockchipRkaf) {
        const QStringList args{"pack-rkaf", level1Dir, outputFile,
                               "--model", model, "--manufacturer", manufacturer};
        if (m_shell.runGetCode(afp, args) != 0) {
            fail("afptool-rs could not pack the RKAF update image: " +
                 m_shell.lastOutput().trimmed().section('\n', -1));
            return false;
        }
        return QFile::exists(outputFile);
    }

    // RKFW: build the inner RKAF, then wrap it together with the saved BOOT and original header
    const QString boot   = level1Dir + "/.rkfw/BOOT";
    const QString header = level1Dir + "/.rkfw/rkfw-header.bin";
    if (!QFile::exists(boot) || !QFile::exists(header)) {
        fail("missing .rkfw/BOOT or .rkfw/rkfw-header.bin – unpack the RKFW again so the "
             "wrapper can be rebuilt");
        return false;
    }

    const QString stage = QFileInfo(level1Dir).absolutePath() + "/.rkfw-repack";
    ShellRunner::deleteDirectory(stage);
    QDir().mkpath(stage);
    QFile::copy(boot, stage + "/BOOT");
    QFile::copy(header, stage + "/rkfw-header.bin");

    const QString embedded = stage + "/embedded-update.img";
    const QStringList rkafArgs{"pack-rkaf", level1Dir, embedded,
                               "--model", model, "--manufacturer", manufacturer};
    if (m_shell.runGetCode(afp, rkafArgs) != 0) {
        fail("afptool-rs could not pack the embedded RKAF image: " +
             m_shell.lastOutput().trimmed().section('\n', -1));
        ShellRunner::deleteDirectory(stage);
        return false;
    }

    QStringList rkfwArgs{"pack-rkfw", stage, outputFile};
    if (specifyChipset && !chipset.isEmpty()) rkfwArgs << "--chip" << chipset;
    const bool packed = (m_shell.runGetCode(afp, rkfwArgs) == 0) && QFile::exists(outputFile);
    ShellRunner::deleteDirectory(stage);
    return packed;
}

bool Unpacker::repackAmlogicImage(const QString &level1Dir, const QString &outputFile)
{
    const QString amp = toolPath("ampack");
    if (amp.isEmpty()) return false;

    QFile::remove(outputFile);
    if (m_shell.runGetCode(amp, QStringList{"pack", level1Dir, outputFile}) != 0) {
        fail("ampack could not pack " + QFileInfo(outputFile).fileName() + ": " +
             m_shell.lastOutput().trimmed().section('\n', -1));
        return false;
    }
    return QFile::exists(outputFile);
}

bool Unpacker::repackAllwinnerImage(const QString &level1Dir, const QString &outputFile)
{
    const QString wty = toolPath("imagewty-tool");
    if (wty.isEmpty()) return false;

    if (!QFile::exists(level1Dir + "/image.cfg")) {
        fail("the unpacked folder has no image.cfg – IMAGEWTY repacking needs it");
        return false;
    }

    QFile::remove(outputFile);
    if (m_shell.runGetCode(wty, QStringList{"repack", level1Dir, outputFile}) != 0) {
        fail("imagewty-tool could not repack " + QFileInfo(outputFile).fileName() + ": " +
             m_shell.lastOutput().trimmed().section('\n', -1));
        return false;
    }
    return QFile::exists(outputFile);
}

bool Unpacker::repackZipFile(const QString &level1Dir, const QString &outputFile,
                              int compressionLevel)
{
    m_logger->log(m_logHead + "Repacking upgrade zip...");

    QString zipTmp = QFileInfo(level1Dir).absolutePath() + "/zip_tmp";
    ShellRunner::deleteDirectory(zipTmp);
    QDir().mkpath(zipTmp);

    // Copy standard partition files to zip_tmp
    QStringList partitions = {"boot", "dtbo", "logo", "recovery", "vbmeta", "super"};
    for (const QString &p : partitions) {
        QStringList candidates = {
            level1Dir + "/" + p + ".PARTITION",
            level1Dir + "/" + p + "_a.PARTITION",
            level1Dir + "/" + p + ".img",
            level1Dir + "/" + p + "_a.img"
        };
        for (const QString &c : candidates) {
            if (QFile::exists(c)) {
                QFile::copy(c, zipTmp + "/" + p + ".img");
                break;
            }
        }
    }

    // Convert ext4 partitions to sdat format using img2sdat.py (GPL)
    QString img2sdat = Platform::binFile("img2sdat.py");
    if (!QFile::exists(img2sdat))
        img2sdat = QCoreApplication::applicationDirPath() + "/scripts/img2sdat.py";
    if (!QFile::exists(img2sdat))
        img2sdat = Platform::rootDir() + "/scripts/img2sdat.py";

    QStringList extPartitions = {"system", "vendor", "product", "odm",
                                  "oem", "system_ext", "odm_ext"};
    for (const QString &p : extPartitions) {
        QStringList candidates = {
            level1Dir + "/" + p + ".PARTITION",
            level1Dir + "/" + p + "_a.PARTITION",
            level1Dir + "/" + p + ".img"
        };
        for (const QString &c : candidates) {
            if (QFile::exists(c) && QFile::exists(img2sdat)) {
                m_shell.run(python3(),
                    QString("\"%1\" -o \"%2\" -v 4 -p %3 \"%4\"")
                        .arg(img2sdat, zipTmp, p, c));
                break;
            }
        }
    }

    // Generate updater-script
    QDir().mkpath(zipTmp + "/META-INF/com/google/android");
    QFile script(zipTmp + "/META-INF/com/google/android/updater-script");
    if (script.open(QIODevice::WriteOnly)) {
        QTextStream s(&script);
        s.setCodec("UTF-8");
        s << "set_bootloader_env(\"upgrade_step\", \"3\");\n";
        s << "show_progress(0.650000, 0);\n";
        for (const QString &p : extPartitions) {
            if (QFile::exists(zipTmp + "/" + p + ".new.dat")) {
                s << "ui_print(\"Patching " << p << " image unconditionally...\");\n";
                s << "block_image_update(\"/dev/block/" << p
                  << "\", package_extract_file(\"" << p << ".transfer.list\"), \""
                  << p << ".new.dat\", \"" << p << ".patch.dat\") ||\n";
                s << "abort(\"E1001: Failed to update " << p << " image.\");\n";
            }
        }
        for (const QString &p : {"super", "logo", "dtbo", "recovery", "boot", "vbmeta"}) {
            if (QFile::exists(zipTmp + "/" + p + ".img")) {
                s << "ui_print(\"Writing raw " << p << ".img...\");\n";
                s << "package_extract_file(\"" << p << ".img\", \"/dev/block/" << p << "\") ||\n";
                s << "abort(\"E1001: Failed to write " << p << " image.\");\n";
            }
        }
        s << "set_bootloader_env(\"upgrade_step\", \"1\");\n";
        s << "set_progress(1.000000);\n";
        script.close();
    }

    // Pack into zip using 7z
    m_shell.run(sevenZip(),
        QString("a \"%1\" \"%2/*\" -mx%3 -tzip")
            .arg(outputFile, zipTmp).arg(compressionLevel));

    ShellRunner::deleteDirectory(zipTmp);
    return QFile::exists(outputFile) && QFileInfo(outputFile).size() > 52400;
}

bool Unpacker::repackKernel(const QString &inputDir, const QString &outputFile)
{
    m_logger->log(m_logHead + "Repacking kernel: " + outputFile);

    const QString aik = Platform::findScript("aik/repackimg");
    if (!aik.isEmpty()) {
        runScript(aik, {}, inputDir);
        QString packed = inputDir + "/image-new.img";
        if (QFile::exists(packed)) return QFile::rename(packed, outputFile);
    }

    QString magiskboot = findTool("magiskboot");
    if (QFile::exists(magiskboot)) {
        m_shell.run(magiskboot, "repack boot.img", inputDir);
        QString packed = inputDir + "/new-boot.img";
        if (QFile::exists(packed)) return QFile::rename(packed, outputFile);
    }

    return false;
}

bool Unpacker::repackDtb(const QString &dtsDir, const QString &outputFile)
{
    m_logger->log(m_logHead + "Repacking DTB: " + outputFile);
    QString dtc = findTool("dtc");
    if (!QFile::exists(dtc)) return false;

    QStringList dtsFiles;
    QDirIterator it(dtsDir, {"*.dts"}, QDir::Files);
    while (it.hasNext()) dtsFiles << it.next();

    if (dtsFiles.isEmpty()) return false;
    if (dtsFiles.size() == 1) {
        m_shell.run(dtc, QString("-I dts -O dtb -o \"%1\" \"%2\"")
                    .arg(outputFile, dtsFiles.first()));
        return QFile::exists(outputFile);
    }

    // Multi-DTB: compile each then merge with dtbdump/mkdtimg
    // For now compile first one (multi-DTB requires mkdtimg from AOSP)
    m_shell.run(dtc, QString("-I dts -O dtb -o \"%1\" \"%2\"")
                .arg(outputFile, dtsFiles.first()));
    return QFile::exists(outputFile);
}

bool Unpacker::repackLogo(const QString &logoDir, const QString &outputFile)
{
    m_logger->log(m_logHead + "Repacking logo: " + outputFile);
    QString amlLogoPack = findTool("aml_logo_pack");
    if (QFile::exists(amlLogoPack)) {
        m_shell.run(amlLogoPack, QString("\"%1\" \"%2\"").arg(outputFile, logoDir));
        return QFile::exists(outputFile);
    }
    return false;
}

// ---------------------------------------------------------------------------
// Utilities
// ---------------------------------------------------------------------------

bool Unpacker::copyFilesRecursively(const QString &src, const QString &dst)
{
    QDir srcDir(src);
    QDir().mkpath(dst);
    for (const QFileInfo &fi : srcDir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (fi.isDir())
            copyFilesRecursively(fi.absoluteFilePath(), dst + "/" + fi.fileName());
        else
            QFile::copy(fi.absoluteFilePath(), dst + "/" + fi.fileName());
    }
    return true;
}
