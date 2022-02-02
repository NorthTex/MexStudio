#include "utilsSystem.h"
#include "unixutils.h"
#include "smallUsefulFunctions.h"

#ifdef Q_OS_MAC
#include <CoreFoundation/CFURL.h>
#include <CoreFoundation/CFBundle.h>
#endif

#ifdef Q_OS_WIN
#include <windows.h>
#endif

	
#ifdef Q_OS_WIN

bool getDiskFreeSpace(const QString & path,quint64 & freeBytes){
	
	wchar_t * d = new wchar_t[path.size() + 1];
	int len = path.toWCharArray(d);
	d[len] = 0;

	ULARGE_INTEGER freeBytesToCaller;
	freeBytesToCaller.QuadPart = 0L;

	if(!GetDiskFreeSpaceEx(d,& freeBytesToCaller,NULL,NULL)){
        delete[] d;
		qDebug() << "ERROR: Call to GetDiskFreeSpaceEx() failed on path" << path;
		return false;
	}

    delete[] d;
	freeBytes = freeBytesToCaller.QuadPart;
	return true;
}

#else

bool getDiskFreeSpace(const QString & path,quint64 & freeBytes){
	Q_UNUSED(path)
	Q_UNUSED(freeBytes)
	return false;
}

#endif


QLocale::Language getKeyboardLanguage(){
	return QGuiApplication::inputMethod() 
		-> locale()
		.  language();
}


/*!
 * Redefine or filter some shortcuts depending on the locale
 * On Windows, AltGr is interpreted as Ctrl+Alt so we shouldn't
 * use a Ctrl+Alt+Key shortcut if AltGr+Key is used for typing
 * characters.
 */

#ifndef Q_OS_WIN32

QKeySequence filterLocaleShortcut(QKeySequence sequence){
	return sequence;
}

#else

QKeySequence filterLocaleShortcut(QKeySequence sequence){

	auto lang = getKeyboardLanguage();
	
	switch(lang){
	case QLocale::Hungarian:

		if(sequence.matches(QKeySequence("Ctrl+Alt+F")))
			return QKeySequence("Ctrl+Alt+Shift+F");
		
		break;
	case QLocale::Polish:
		
		if(sequence.matches(QKeySequence("Ctrl+Alt+S")))
			return QKeySequence();
		
		if(sequence.matches(QKeySequence("Ctrl+Alt+U")))
			return QKeySequence("Ctrl+Alt+Shift+U");

		break;
	case QLocale::Turkish:

		if(sequence.matches(QKeySequence("Ctrl+Alt+F")))
			return QKeySequence("Ctrl+Alt+Shift+F");

		break;
	case QLocale::Czech:
		
		if(sequence.matches(QKeySequence("Ctrl+Alt+S")))
			return QKeySequence();
		
		if(sequence.matches(QKeySequence("Ctrl+Alt+F")))
			return QKeySequence("Ctrl+Alt+Shift+F");
		
		if(sequence.matches(QKeySequence("Ctrl+Alt+L")))
			return QKeySequence("Ctrl+Alt+Shift+L");

		break;
	case QLocale::Croatian:
		
		if(sequence.matches(QKeySequence("Ctrl+Alt+F")))
			return QKeySequence("Ctrl+Alt+Shift+F");

		break;
	}

	return sequence;
}

#endif


QChar getPathListSeparator()
{
#ifdef Q_OS_WIN32
	return QChar(';');
#else
	return QChar(':');
#endif
}


QStringList splitPaths(const QString & paths){
	return (paths.isEmpty())
		? QStringList()
		: paths.split(getPathListSeparator());
}


#ifdef Q_OS_WIN32

	QString getUserName(){
		return QString(qgetenv("USERNAME"));
	}

#else

	QString getUserName(){
		return QString(qgetenv("USER"));
	}

#endif


#ifdef Q_OS_WIN32

	// typically "C:/Documents and Settings/Username/My Documents"
	
	QString getUserDocumentFolder(){
		
		QSettings settings(QSettings::UserScope,"Microsoft","Windows");
		settings.beginGroup("CurrentVersion/Explorer/Shell Folders");
		
		return settings
			.value("Personal")
			.toString();
	}

#else

	QString getUserDocumentFolder(){
		return QDir::homePath();
	}

#endif


QStringList findResourceFiles(
	const QString & dirName,
	const QString & filter,
	QStringList additionalPreferredPaths
){
	QStringList searchFiles;
	QString dn = dirName;

	// remove / at the end

	if(dn.endsWith('/') || dn.endsWith(QDir::separator()))
		dn = dn.left(dn.length() - 1);

	// add / at beginning

	if(!dn.startsWith('/') && !dn.startsWith(QDir::separator()))
		dn = "/" + dn;
	
	//resource fall back

	searchFiles << ":" + dn;

	searchFiles.append(additionalPreferredPaths);

	const auto root = QCoreApplication::applicationDirPath();

	// Appimage relative path

    searchFiles << root + "/../share/texstudio"; 
    
	// Windows ( Old )
	
	searchFiles << root + "/"; 

	// Windows ( New )

	searchFiles << root + dn; 
    searchFiles << root + "/dictionaries/";
    searchFiles << root + "/translations/";
    searchFiles << root + "/help/";
    searchFiles << root + "/utilities/";

	#if !defined(PREFIX)
	#define PREFIX ""
	#endif

	#if defined( Q_WS_X11 ) || defined (Q_OS_LINUX)
		searchFiles << PREFIX"/share/texstudio" + dn; //X_11
	#endif
		
	#ifdef Q_OS_MAC
		
		auto appUrl = CFBundleCopyBundleURL(CFBundleGetMainBundle());
		auto macPath = CFURLCopyFileSystemPath(appUrlRef,kCFURLPOSIXPathStyle);
		const auto path = CFStringGetCStringPtr(macPath,CFStringGetSystemEncoding());
		
		searchFiles << QString(path) + "/Contents/Resources" + dn;

		CFRelease(appUrlRef);
		CFRelease(macPath);
	
	#endif

	QStringList result;

	for(const auto & file : searchFiles){

		QDir dir(file);
		
		if(dir.exists() && dir.isReadable())
			result << dir.entryList(QStringList(filter),QDir::Files,QDir::Name);
	}

	// sort and remove double entries
	
	result.sort();

	QMutableStringListIterator iterator(result);
	QString old = "";
	
	while(iterator.hasNext()){
	
		auto next = iterator.next();
		
		if(next == old)
			iterator.remove();
		else
			old = next;
	}

	return result;
}


QString findResourceFile(
	const QString & fileName,
	bool allowOverride,
	QStringList additionalPreferredPaths,
	QStringList additionalFallbackPaths
){
	QStringList searchFiles;

	// search first in included resources (much faster)

	if(!allowOverride)
		searchFiles << ":/";

	for(const auto & path : additionalPreferredPaths){
	
		auto separator = "/";
		
		if(path.endsWith('/'))
			separator = "";

		if(path.endsWith('\\'))
			separator = "";

		searchFiles << path + separator;
	}

	#if defined Q_WS_X11 || defined Q_OS_LINUX || defined Q_OS_UNIX
	
		// X11

		searchFiles << PREFIX"/share/texstudio/"; 

		// Relative Path For AppImage

		searchFiles << QCoreApplication::applicationDirPath() + "/../share/texstudio/";
	
		// Debian Package
	
		if(fileName.endsWith(".html"))
			searchFiles << PREFIX"/share/doc/texstudio/html/";
	
	#endif

	const auto root = QCoreApplication::applicationDirPath();
	
	#ifdef Q_OS_MAC
	
		searchFiles << root + "/../Resources/";
	
	#endif


	// Windows ( Old ) 

	searchFiles << root + "/";

	// Windows ( New )

	searchFiles << root + "/dictionaries/"; 
	searchFiles << root + "/translations/";
	searchFiles << root + "/help/";
	searchFiles << root + "/utilities/";

	// Resource Fallback

	if(allowOverride)
		searchFiles << ":/";

	for(const auto & path : additionalFallbackPaths){
	
		auto separator = "/";
		
		if(path.endsWith('/'))
			separator = "";

		if(path.endsWith('\\'))
			separator = "";

		searchFiles << path + separator;
	}

	for(const auto & file : searchFiles){

		QFileInfo info(file + fileName);
		
		if(info.exists() && info.isReadable())
			return info.canonicalFilePath();
	}

	auto newFileName = fileName
		.split('/')
		.last();

	if(!newFileName.isEmpty())
		for(const auto & file : searchFiles){

			QFileInfo info(file + newFileName);
			
			if(info.exists() && info.isReadable())
				return info.canonicalFilePath();
		}

	return "";
}


/*!
 * \brief put quotes araound strin when necessary
 *
 * Enclose the given string with quotes if it contains spaces.
 * This function is useful
 * \param s input string
 * \return string with quotes (if s contains spaces)
 */

QString quoteSpaces(const QString & string){

	if(string.contains(' '))
		return '"' + string + '"';
	
	return string;
}


int modernStyle;
bool darkMode;
bool useSystemTheme;

/*!
 * \brief return icon according to settings
 *
 * The icon is looked up in txs resources. It prefers svg over png.
 * In case of active dark mode, icons with name icon_dm is used (if present).
 * \param icon icon name
 * \return found icon as file name
 */

QString getRealIconFile(const QString & icon){

	if(icon.isEmpty())
		return icon;
		
	if(icon.startsWith(":/"))
		return icon;
    
	QStringList suffixList { "" };
    
	if(darkMode)
        suffixList = QStringList { "_dm" , "" };

    QStringList iconNames = QStringList();

    for(const auto & suffix : suffixList){

		const auto name = icon + suffix;

        iconNames
                << ":/images-ng/" + name + ".svg"
                << ":/images-ng/" + name + ".svgz";
        
		if(modernStyle){
            iconNames << ":/images-ng/modern/" + name + ".svg"
                      << ":/images-ng/modern/" + name + ".svgz"
                      << ":/modern/images/modern/" + name + ".png";
        } else {
            iconNames << ":/images-ng/classic/" + name + ".svg"
                      << ":/images-ng/classic/" + name + ".svgz"
                      << ":/classic/images/classic/" + name + ".png";
        }

        iconNames << ":/symbols-ng/icons/" + name + ".svg" ;//voruebergehend
        iconNames << ":/symbols-ng/icons/" + name + ".png"; //voruebergehend
        iconNames << ":/images/" + icon + ".png";
    }

	for(const auto & name : iconNames)
        if(QFileInfo::exists(name))
			return name;

	return icon;
}


/*!
 * \brief search icon
 *
 * Looks up icon by name. If settings allow, tries to use system icons, otherwise tries to find in txs resources.
 * \param icon icon name
 * \return icon
 */

QIcon getRealIcon(const QString & iconPath){

	if(iconPath.isEmpty())
		return QIcon();
	
	if(iconPath.startsWith(":/"))
		return QIcon(iconPath);
	
	if(useSystemTheme && QIcon::hasThemeIcon(iconPath))
		return QIcon::fromTheme(iconPath);
	
	const auto name = getRealIconFile(iconPath);

	#ifdef Q_OS_OSX
		QPixmap map(32,32);
		map.load(name);
		return QIcon(pm);
	#else
		return QIcon(name);
	#endif
}


/*!
 * \brief search icon
 *
 * Looks up icon by name. If settings allow, tries to use system icons, otherwise tries to find in txs resources.
 * The icon is cached for better reactivity.
 * \param icon icon name
 * \param forceReload ignore cache and reload icon from database. This is necessary if light-/dark-mode is changed.
 * \return icon
 */

QIcon getRealIconCached(const QString & path,bool forceReload){

    if(iconCache.contains(path) && !forceReload)
		return * iconCache[path];

	if(path.isEmpty())
		return QIcon();

	if(path.startsWith(":/")){
		auto icon = new QIcon(path);
		iconCache.insert(path,icon);
		return * icon;
	}

	if(useSystemTheme && QIcon::hasThemeIcon(path)){
		auto icon = new QIcon(QIcon::fromTheme(path));
		iconCache.insert(path,icon);
		return * icon;
	}

	const auto icon = new QIcon(getRealIconFile(path));
	iconCache.insert(path,icon);
	return * icon;
}


/*!
 * \brief Tries to determine if the system uses dark mode
 *
 * This function tries to determine if a system uses "dark mode" by looking up general text color and converting it into gray-scale value.
 * A value above 200 (scale is 0 .. 255 ) is considered as light text color on probably dark background, hence a dark mode is detected.
 * This approach is independent on specific on different systems.
 * \return true -> uses dark mode
 */

bool systemUsesDarkMode(const QPalette & palette){

    const auto color = palette
		.color(QPalette::Text)
		.rgb();

    return (qGray(color) > 200);
}


bool isFileRealWritable(const QString & path){

    if(QFileInfo::exists(path))
        return QFileInfo(path).isWritable();

	QFile file(path);
	
	bool success = false;

	if(file.exists()){
		success = file.open(QIODevice::ReadWrite);
	} else {
		success = file.open(QIODevice::WriteOnly);
		file.remove();
	}

	return success;
}


/*!
 * \brief checks if file exists and is writeable
 *
 * Convenience function
 * \param filename
 * \return
 */

bool isExistingFileRealWritable(const QString & path){
    return QFileInfo::exists(path) && 
		isFileRealWritable(path);
}


/*!
 * \brief make sure that the dirPath is terminated with a separator (/ or \)
 *
 * \param path
 * \return dirPath with trailing separator
 */

QString ensureTrailingDirSeparator(const QString & path){

	if(path.isEmpty())
		return path;
		
	if(path.endsWith("/"))
		return path;
	
	if(path.endsWith(QDir::separator()))
		return path;

	#ifdef Q_OS_WIN32

		// you can create a directory named \ on linux
		
		if(path.endsWith("\\"))
			return path;
	
	#endif
	
	return path + "/";
}


/*!
 * \brief join dirname and filename with apropriate separator
 * \param dirname
 * \param filename
 * \return
 */

QString joinPath(const QString & dirname,const QString & filename){
	return ensureTrailingDirSeparator(dirname) + filename;
}


/*!
 * \brief join dirname, dirname2 and filename with apropriate separator
 * \param dirname
 * \param dirname2
 * \param filename
 * \return
 */

QString joinPath(
	const QString & dirname,
	const QString & dirname2,
	const QString & filename
){
	return 
		ensureTrailingDirSeparator(dirname) + 
		ensureTrailingDirSeparator(dirname2) + 
		filename;
}


/// Removes any symbolic link inside the file path.
/// Does nothing on Windows.

#ifdef Q_OS_UNIX

	// < Do not seek for symbolic links deeper than MAX_DIR_DEPTH.
	// For performance issues and if the root directory was not catched (infinite loop).
	// Static array might be also used to prevent heap allocation for a small amont of data. QFileInfo is shared, so the size of the array is size_of(void*)*MAX_DIR_DEPTH
	// QFileInfo stack[MAX_DIR_DEPTH];

	QFileInfo getNonSymbolicFileInfo(const QFileInfo & info){

		const size_t MAX_DIR_DEPTH = 32; 
		
		QStack<QFileInfo> stack;
		stack.reserve(MAX_DIR_DEPTH);
		stack.push(info);
		
		size_t depth = 0;
		int lastChanged = 0;

		QFileInfo pfi;
		
		do {

			QDir parent =  stack.top().dir();
			pfi = QFileInfo(parent.absolutePath());
			
			if(pfi.isSymLink()){
				pfi = QFileInfo(pfi.symLinkTarget());
				lastChanged = depth;
			}

			stack.push(pfi);
			depth++;

		} while(!pfi.isRoot() && depth < MAX_DIR_DEPTH);

		pfi = stack[lastChanged];
		int i = lastChanged -1;

		for(;i >= 0;i--)
			pfi = QFileInfo(QDir(pfi.absoluteFilePath()),stack[i].fileName());
		
		return pfi;
	}

#else

	QFileInfo getNonSymbolicFileInfo(const QFileInfo & info){
		return info;
	}

#endif


/*!
 * \brief replace file extension
 * \param filename
 * \param newExtension
 * \param appendIfNoExt
 * \return
 */

QString replaceFileExtension(
	const QString & filename,
	const QString & newExtension,
	bool appendIfNoExt
){
	QFileInfo info(filename);
	
	auto extension = newExtension.startsWith('.') 
		? newExtension.mid(1) 
		: newExtension;

	if(info.suffix().isEmpty()){

		if(appendIfNoExt)
			return filename + '.' + extension;

		return QString();
	}

	// exchange the suffix explicitly instead of using fi.completeBaseName()
	// so that the filename stays exactly the same

	return filename.left(filename.length() - info.suffix().length()) + extension;
}


QString getRelativeBaseNameToPath(
	const QString & file,
	QString basepath,
	bool baseFile,
	bool keepSuffix
){
	basepath.replace(QDir::separator(),"/");

	if(basepath.endsWith("/"))
		basepath = basepath.left(basepath.length() - 1);

	QFileInfo fi(file);
	QString filename = fi.fileName();
	QString path = fi.path();
	
	if(path.endsWith("/"))
		path = path.left(path.length() - 1);
	
	QStringList basedirs = basepath.split("/");
	
	if(baseFile && !basedirs.isEmpty())
		basedirs.removeLast();

	QStringList dirs = path.split("/");
	int nDirs = dirs.count();

	while(dirs.count() > 0 && basedirs.count() > 0 &&  dirs[0] == basedirs[0]){
		dirs.pop_front();
		basedirs.pop_front();
	}

	if(nDirs != dirs.count()){
	
		path = dirs.join("/");

		if(basedirs.count() > 0)
			for(int j = 0;j < basedirs.count();++j)
				path = "../" + path;
	} else {
		path = fi.path();
	}

	// necessary if basepath isn't given

	if(path.length() > 0 && !path.endsWith("/") && !path.endsWith("\\"))
		path += "/";

	if(keepSuffix)
		return path + filename;
	
	return path + fi.completeBaseName();
}


QString getPathfromFilename(const QString & path){
	
	if(path.isEmpty())
		return "";
	
	auto directory = QFileInfo(path).absolutePath();
	
	if(!directory.endsWith("/") && !directory.endsWith(QDir::separator()))
		directory.append(QDir::separator());
	
	return directory;
}


QString findAbsoluteFilePath(
	const QString & relName,
	const QString & extension,
	const QStringList & searchPaths,
	const QString & fallbackPath
){
	QString s = relName;
	QString ext = extension;

	if(!ext.isEmpty() && !ext.startsWith("."))
		ext = "." + ext;
	
	if(!s.endsWith(ext, Qt::CaseInsensitive))
		s += ext;
	
	QFileInfo fi(s);
	
	if(!fi.isRelative())
		return s;
	
	for(const auto & path : searchPaths){
		
		fi.setFile(QDir(path),s);
		
		if(fi.exists())
			return fi.absoluteFilePath();
	}

	QString fbp = fallbackPath;
	
	if(!fbp.isEmpty() && !fbp.endsWith('/') && !fbp.endsWith(QDir::separator()))
		fbp += QDir::separator();
	
	return fbp + s; // fallback
}


/*!
 * Tries to get a non-existent filename. If guess, does not exist, return it.
 * Otherwise, try find a non-extistent filename by increasing a number at the end
 * of the filesname. If there is already a number, start from there, e.g.
 * test02.txt -> test03.txt. If no free filename could be determined, return fallback.
 */

QString getNonextistentFilename(const QString & guess,const QString & fallback){
	
	QFileInfo info(guess);

	if(!info.exists())
		return guess;
	
	QRegExp reNumberedFilename("(.*[^\\d])(\\d*)\\.(\\w+)");
	
	if(!reNumberedFilename.exactMatch(guess))
		return fallback;

	const auto
		base = reNumberedFilename.cap(1),
		ext = reNumberedFilename.cap(3);
	
	int 
		numLen = reNumberedFilename.cap(2).length(),
		num = reNumberedFilename.cap(2).toInt();

	for(int i = num + 1;i <= 1000000;i++){

		const auto filename = QString("%1%2.%3")
			.arg(base)
			.arg(i,numLen,10,QLatin1Char('0'))
			.arg(ext);
		
		info.setFile(filename);
		
		if(!info.exists())
			return filename;
	}

	return fallback;
}


/*!
 * \brief get environment path
 *
 * Get the content of PATH environment variable.
 * On OSX starts a bash to get a more complete picture as programs get started without access to the bash PATH.
 * \return path
 */

#ifdef Q_OS_MAC

	QString getEnvironmentPath(){
	
		static QString path;

		if(path.isNull()){

			// -n ensures there is no newline at the end

			auto process = new QProcess();
			process -> start("/usr/libexec/path_helper");
			process -> waitForFinished(3000);
			
			if(process -> exitStatus() == QProcess::NormalExit){
				
				auto bytes = process -> readAllStandardOutput();
				
				// bash may have some initial output.
				// path is on the last line

				path = QString(bytes)
					.split('\"')
					.at(1);
			} else {
				path = "";
			}

			delete process;
		}
		
		return path;
	}

#else

	QString getEnvironmentPath(){
		
		static QString path;

		if(path.isNull())
			path = QProcessEnvironment::systemEnvironment().value("PATH");

		return path;
	}

#endif


/*!
 * \brief get environment path list
 * Same as getEnvironmentPath(), but splits the sresult into a stringlist.
 * \return
 */

QStringList getEnvironmentPathList(){
	return getEnvironmentPath().split(getPathListSeparator());
}


/*!
 * \brief update path settings for a process
 * \param proc process
 * \param additionalPaths
 */

void updatePathSettings(QProcess * process,QString additionalPaths){

	auto environment = QProcessEnvironment::systemEnvironment();
	QString path(getEnvironmentPath());

	if(!additionalPaths.isEmpty())
		path += getPathListSeparator() + additionalPaths;

	environment.insert("PATH",path);

	// Note:
	// this modifies the path only for the context of the called program.
	// It does not affect the search path for the program itself.

	process -> setProcessEnvironment(environment);
}




	// Mac, Windows support folder or file.
#if defined(Q_OS_WIN)

	void showInGraphicalShell(QWidget * parent,const QString & pathIn){

		QFileInfo fiExplorer(QProcessEnvironment::systemEnvironment().value("WINDIR"), "explorer.exe");
		if (!fiExplorer.exists()) {
			QMessageBox::warning(parent,
								QApplication::translate("Texstudio",
														"Launching Windows Explorer Failed"),
								QApplication::translate("Texstudio",
														"Could not find explorer.exe in path to launch Windows Explorer."));
			return;
		}
		QStringList param;
		if (!QFileInfo(pathIn).isDir())
			param += QLatin1String("/select,");
		param += QDir::toNativeSeparators(pathIn);
		QProcess::startDetached(fiExplorer.absoluteFilePath(), param);

	}

#elif defined(Q_OS_MAC)
	
	void showInGraphicalShell(QWidget * parent,const QString & pathIn){

		QStringList scriptArgs;
		scriptArgs << QLatin1String("-e")
				<< QString::fromLatin1("tell application \"Finder\" to reveal POSIX file \"%1\"")
										.arg(pathIn);
		QProcess::execute(QLatin1String("/usr/bin/osascript"), scriptArgs);
		scriptArgs.clear();
		scriptArgs << QLatin1String("-e")
				<< QLatin1String("tell application \"Finder\" to activate");
		QProcess::execute(QLatin1String("/usr/bin/osascript"), scriptArgs);

	}

#else

	void showInGraphicalShell(QWidget * parent,const QString & pathIn){

		// we cannot select a file here, because
		// no file browser really supports it...

		using namespace Utils;

		const QFileInfo info(pathIn);

		const auto folder = (info.isDir()) 
			? info.absoluteFilePath() 
			: info.filePath();
		
		QSettings dummySettings;
		
		const auto app = UnixUtils::fileBrowser(& dummySettings);
		
		QProcess browserProc;
		
		const auto browserArg = UnixUtils::substituteFileBrowserParameters(app,folder);
		
		auto args = QProcess::splitCommand(browserArg);
		
		if(args.isEmpty())
			return;
		
		auto command = args.takeFirst();
		
		for(auto & elem : args)
			elem = removeQuote(elem);

		bool success = browserProc.startDetached(command,args);
		const auto error = QString::fromLocal8Bit(browserProc.readAllStandardError());

		success = success && error.isEmpty();

		if(!success)
			QMessageBox::critical(parent,app,error);
	}

#endif



#if defined(Q_OS_WIN)

	QString msgGraphicalShellAction(){
		return QApplication::translate("Texstudio","Show in Explorer");
	}

#elif defined(Q_OS_MAC)
	
	QString msgGraphicalShellAction(){
		return QApplication::translate("Texstudio","Show in Finder");
	}

#else

	QString msgGraphicalShellAction(){
		return QApplication::translate("Texstudio","Show Containing Folder");
	}

#endif


/*!
 * \brief determine which x11 environment is used.
 * This is probably obsolete.
 * \return 0 : no kde ; 3: kde ; 4 : kde4 ; 5 : kde5
 */

int x11desktop_env(){

	const QString version = ::getenv("KDE_SESSION_VERSION");

	if(!version.isEmpty())
		return version.toInt();

	const QString session = ::getenv("KDE_FULL_SESSION");

	return (session.isEmpty())
		? 0 : 3;
}


// detect a retina macbook via the model identifier
// http://support.apple.com/kb/HT4132?viewlocale=en_US&locale=en_US

#ifdef Q_OS_MAC
	
	bool isRetinaMac(){
	
		static bool 
			firstCall = true,
			isRetina = false;

		if(firstCall){

			firstCall = false;
			
			QProcess process;
			process.start("sysctl",QStringList() << "-n" << "hw.model");
			process.waitForFinished(1000);
			
			// is something like "MacBookPro10,1"

			QString model(process.readAllStandardOutput());
			
			QRegExp regex("MacBookPro([0-9]*)");
			regex.indexIn(model);
			
			int num = regex
				.cap(1)
				.toInt();
			
			// compatibility with future MacBookPros.
			// Assume they are also retina.

			if(num >= 10)
				isRetina = true;
		}

		return isRetina;
	}

#else

	bool isRetinaMac(){
		return false;
	}

#endif


bool hasAtLeastQt(int major,int minor){

	auto version = QString(qVersion()).split('.');

	if(version.count() < 2)
		return false;
	
	int 
		ma = version[0].toInt(),
		mi = version[1].toInt();
	
	return (ma > major) || 
		(ma == major && mi >= minor);
}


/// convenience function for unique connections independent of the Qt version

bool connectUnique(
	const QObject * sender,
	const char * signal,
	const QObject * receiver,
	const char * method
){
	return QObject::connect(sender,signal,receiver,method,Qt::UniqueConnection);
}


void ThreadBreaker::sleep(unsigned long duration){
	QThread::sleep(duration);
}


void ThreadBreaker::msleep(unsigned long ms){
	QThread::msleep(ms);
}


void ThreadBreaker::forceTerminate(QThread * thread){
	
	if(!thread)
		thread = QThread::currentThread();

	thread -> setTerminationEnabled(true);
	thread -> terminate();
}


SafeThread::SafeThread()
	: QThread(nullptr)
	, crashed(false) {}

SafeThread::SafeThread(QObject * parent)
	: QThread(parent)
	, crashed(false) {}


void SafeThread::wait(unsigned long duration){
	
	if(crashed)
		return;

	QThread::wait(duration);
}


QSet<QString> convertStringListtoSet(const QStringList & list){
    return QSet<QString>(list.begin(),list.end());
}
