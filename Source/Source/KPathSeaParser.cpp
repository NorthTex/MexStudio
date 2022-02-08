
#include "kpathseaParser.h"
#include "execprogram.h"

PackageScanner::PackageScanner(QObject * parent) 
	: SafeThread(parent) {

	stopped = false;
}


void PackageScanner::savePackageList(
	std::set<QString> packages,
	const QString & path
){
	QFile file(path);

	if(file.open(QFile::WriteOnly | QFile::Text)) {
		
		QTextStream out(& file);
		out << "% detected .sty and .cls filenames\n";

        for(const QString & package : packages)
            out << package << "\n";
	}
}


std::set<QString> PackageScanner::readPackageList(const QString & path){

    QFile file(path);
    
	std::set<QString> packages;
	
	if(file.open(QFile::ReadOnly | QFile::Text)){
		
		QTextStream stream(& file);
		
		while(!stream.atEnd()){
			
			const auto line = stream.readLine();
			
			if(line.isEmpty())
				continue;
				
			if(line.startsWith('%'))
				continue;
			
			packages.insert(line);
		}
	}

	return packages;
}


void PackageScanner::stop(){
	stopped = true;
}


KpathSeaParser::KpathSeaParser(
	QString kpsecmd,
	QObject * parent,
	QString additionalPaths
) : PackageScanner(parent) {
	kpseWhichCmd = kpsecmd;
    m_additionalPaths=additionalPaths;
}


void KpathSeaParser::run(){

    std::set<QString> results;

	const auto location = kpsewhich("--show-path ls-R");

	// find lcoations of ls-R (file database of tex)

	const auto files = location.split(getPathListSeparator()); 
	
	for(auto & path : files){

		if(stopped)
			return;
		
		QFile file(path.trimmed() + "/ls-R");

		if(file.open(QIODevice::ReadOnly | QIODevice::Text)){

			QTextStream stream(& file);
			
			while(!stream.atEnd()) {
				
				auto line = stream.readLine();
				
				if(line.endsWith(".sty") || line.endsWith(".cls")){
					line.chop(4);
					results.insert(line);
				}
			}
		}
	}

	emit scanCompleted(results);
}


QString KpathSeaParser::kpsewhich(const QString & argument){
	ExecProgram program(kpseWhichCmd + " " + argument,m_additionalPaths);
	program.execAndWait();
	return program.m_standardOutput;
}


MiktexPackageScanner::MiktexPackageScanner(
	QString mpmcmd,
	QString settingsDir,
	QObject * parent
) : PackageScanner(parent) 
  , mpmCmd(mpmcmd)
  , settingsDir(settingsDir) {}


QString MiktexPackageScanner::mpm(const QString & argument){
	ExecProgram program(mpmCmd + " " + argument,"");
	program.execAndWait();
	return program.m_standardOutput;
}


void MiktexPackageScanner::savePackageMap(const QHash<QString,QStringList> & packages){

	QFile file(ensureTrailingDirSeparator(settingsDir) + "miktexPackageNames.dat");
	
	if(file.open(QFile::WriteOnly | QFile::Text)){

		QTextStream stream(& file);
		stream << "% This file maps the MiKTeX package names to the .sty and .cls file names.\n";
		stream << "% It's used as cache for a fast lookup. It's automatically created and may be deleted without harm.\n";
		
		for(const auto & name : packages.keys()){

			const auto package = packages
				.value(name)
				.join(',');

			stream << name << ':' << package << '\n';
		}			
	}
}


QHash<QString,QStringList> MiktexPackageScanner::loadPackageMap(){

	QFile file(ensureTrailingDirSeparator(settingsDir) + "miktexPackageNames.dat");
	QHash<QString,QStringList> packages;
	
	if(file.open(QFile::ReadOnly | QFile::Text)) {
		
		QTextStream stream(& file);

		while(!stream.atEnd()){

			const auto line = stream.readLine();
			
			if(line.isEmpty())
				continue;
				
			if(line.startsWith('%'))
				continue;
			
			const auto list = line.split(':');
			
			if(list.length() < 2)
				continue;
			
			packages.insert(list[0],list[1].split(','));
		}
	}

	return packages;
}



QStringList MiktexPackageScanner::stysForPackage(const QString & package){

	bool inRunTimeFilesSection = false;
	QStringList paths;

	const auto lines = mpm("--print-package-info " + package).split("\n");

	for(const auto & line : lines){

		if(inRunTimeFilesSection){

			auto path = line.simplified();
			
			// start of a new section

			if(path.endsWith(":"))
				break; 
			
			path = QFileInfo(path).fileName();

			if(path.endsWith(".sty") || path.endsWith(".cls")){
				path.chop(4);
				paths.append(path);
			}
		} else {
			
			if(line.startsWith("run-time files:"))
				inRunTimeFilesSection = true;
			
			continue;
		}
	}

	return paths;
}


void MiktexPackageScanner::run(){

    std::set<QString> results;

    auto cached = loadPackageMap();

	const auto packages = mpm("--list").split("\n");

	for(auto package : packages){

		if(stopped)
			return;

		// output format of "mpm --list": 
		// installation status, number of files, 
		// size, database name

		QStringList parts = package
			.simplified()
			.split(" "); 
		
		if(parts.count() != 4)
			continue;
		
		// not installed

		if(parts[0] != "i")
			continue;

		package = parts[3];
		QStringList stys;

		if(cached.contains(package)){
			stys = cached.value(package);
		} else {
			stys = stysForPackage(package);
			cached.insert(package,stys);
		}

		for(const auto & sty : stys)
			results.insert(sty);
	}

	savePackageMap(cached);

	emit scanCompleted(results);
}
