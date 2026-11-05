#pragma once
#include <QString>
#include <QStringList>
#include <QtGlobal>

/**
 * Platform – everything that differs between Linux and Windows lives here:
 *   - where the app keeps its folders (bin/, tmp/, logs/, patches/, keys/)
 *   - how external tools are located (".exe" suffixes, bundled bin/, PATH, 7-Zip install dir)
 *   - which interpreter / script flavour is used (python3 vs python, .sh vs .bat)
 *
 * Folder layout
 *   ROMTOOLKIT_HOME env var (if set)          -> used on every platform
 *   Windows                                    -> folder of the .exe when writable (portable mode),
 *                                                 otherwise %APPDATA%\ROM Customization Toolkit\...
 *   Linux / macOS                              -> current working directory (unchanged legacy behaviour)
 */
namespace Platform {

#ifdef Q_OS_WIN
constexpr bool isWindows = true;
#else
constexpr bool isWindows = false;
#endif

// --- Folders ---------------------------------------------------------------
QString rootDir();          // base for all of the folders below (writable)
QString binDir();           // <root>/bin
QString tmpDir();           // <root>/tmp  (unpacked ROM + working files)
QString logsDir();          // <root>/logs
QString patchesDir();       // <root>/patches
QString keysDir();          // <root>/keys
QString translationsDir();  // <root>/translations

// Default folder offered by "Save as" dialogs (Linux: current dir as before, Windows: Documents).
QString defaultSaveDir();

// Create the runtime folders and (when root != install dir) seed patches/keys from the install dir.
void ensureLayout();

// Folders searched for bundled tools: <root>/bin, then <install dir>/bin.
QStringList binSearchDirs();

// --- Tool lookup -----------------------------------------------------------
// Absolute path of a tool, or an empty string. Looks in bin/ (adding .exe/.bat/.cmd on
// Windows), then in PATH.
QString findTool(const QString &name);

// A file shipped inside bin/ (e.g. "resources/su"); falls back to <root>/bin/<rel> if absent.
QString binFile(const QString &relPath);

QString findSevenZip();     // bin/7z(a), PATH, "C:\Program Files\7-Zip\7z.exe"; empty if none
QString sevenZip();         // findSevenZip() or plain "7z"
QString python();           // python3 on Linux, python3/python on Windows
QString bash();             // bash for running .sh scripts; empty if none (Windows: Git for Windows only)

// Preferred script for the current OS: "<rel>.bat" / ".cmd" on Windows, "<rel>.sh" elsewhere.
// Falls back to the other flavour if only that one exists. Empty if neither exists.
QString findScript(const QString &relPathNoExt);

// --- Small portable replacements for du / dd -------------------------------
qint64 directorySize(const QString &dir);

} // namespace Platform
