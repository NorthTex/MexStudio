#ifndef Header_UtilsSystem
#define Header_UtilsSystem

#include "mostQtHeaders.h"
#include <QCache>

/*!
 * \file utilsSystem.h
 * \brief helper functions for general use
 *
 */


#define REQUIRE(x)  do { Q_ASSERT((x)); if (!(x)) return; } while (0)
#define REQUIRE_RET(x,e) do { Q_ASSERT((x)); if (!(x)) return (e); } while (0)


extern const char * TEXSTUDIO_GIT_REVISION;

bool getDiskFreeSpace(const QString &path, quint64 &freeBytes);

QLocale::Language getKeyboardLanguage();
QKeySequence filterLocaleShortcut(QKeySequence ks);

QString quoteSpaces(const QString &s);

bool systemUsesDarkMode(const QPalette &pal=QApplication::palette());

QChar getPathListSeparator();
QStringList splitPaths(const QString &paths);

QString getUserName();

QString getUserDocumentFolder();

QStringList findResourceFiles(const QString &dirName, const QString &filter, QStringList additionalPreferredPaths = QStringList());
///returns the real name of a resource file
QString findResourceFile(const QString &fileName, bool allowOverride = false, QStringList additionalPreferredPaths = QStringList(), QStringList additionalFallbackPaths = QStringList());

/*!
 * \brief style used
 *
 * Global variable conatisn info which style is used.
 * 0: classic style
 * 1: modern style
 * 2: modern style - dark
 */
extern int modernStyle;
/*!
 * \brief if dark mode is used
 */
extern bool darkMode;
/*!
 * \brief use system icons if available
 */
extern bool useSystemTheme;
extern QCache<QString, QIcon>iconCache;

using Path = const QString &;

QString getRealIconFile(Path);
QIcon getRealIcon(Path);
QIcon getRealIconCached(Path,bool forceReload = false);

///returns if the file is writable (QFileInfo.isWritable works in different ways on Windows and Linux)
bool isFileRealWritable(Path);

//returns if the file exists and is writable
QString ensureTrailingDirSeparator(Path);
bool isExistingFileRealWritable(Path);

QString joinPath(Path,Path);
QString joinPath(Path,Path,Path);

// replaces "somdir/file.ext" to "somedir/file.newext"
QString replaceFileExtension(Path,const QString & newExtension,bool appendIfNoExt = false);
QString getRelativeBaseNameToPath(Path,QString basepath,bool baseFile = false,bool keepSuffix = false);
QString getPathfromFilename(Path);
QString findAbsoluteFilePath(const QString & relName,const QString & extension,const QStringList & searchPaths,const QString & fallbackPath);
QString getNonextistentFilename(const QString & guess,const QString & fallback = QString());
QFileInfo getNonSymbolicFileInfo(const QFileInfo &info);

QString getEnvironmentPath();
QStringList getEnvironmentPathList();
void updatePathSettings(QProcess * proc,QString additionalPaths);

void showInGraphicalShell(QWidget * parent,const QString & pathIn);
QString msgGraphicalShellAction();

//returns kde version 0,3,4
int x11desktop_env();

bool isRetinaMac();

QSet<QString> convertStringListtoSet(const QStringList & list);

///check if the run-time qt version is higher than the given version (e.g. 4,3)
bool hasAtLeastQt(int major,int minor);

bool connectUnique(const QObject * sender,const char * signal,const QObject * receiver, const char * method);


class ThreadBreaker : public QThread {

	public:

		static void sleep(unsigned long seconds);
		static void msleep(unsigned long milliseconds);
		static void forceTerminate(QThread * thread = nullptr);

};


class SafeThread: public QThread {

	Q_OBJECT

	public:

		SafeThread();
		SafeThread(QObject * parent);

		void wait(unsigned long time = 60000);
		bool crashed;

};


#endif
