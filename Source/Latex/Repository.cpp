
#include "Latex/Repository.hpp"

#include <QMutex>


using Source = LatexRepository::DataSource;


LatexRepository * LatexRepository::m_Instance = nullptr;


LatexRepository::LatexRepository()
    : QObject(nullptr)
    , m_dataSource(None) {

	loadStaticPackageList(":/utilities/packageList");
}


LatexRepository * LatexRepository::instance(){

	static QMutex mutex;
	mutex.lock();

	if(!m_Instance)
		m_Instance = new LatexRepository();

	mutex.unlock();

	return m_Instance;
}


Source LatexRepository::dataSource(){
	return m_dataSource;
}


bool LatexRepository::loadStaticPackageList(const QString & path){

	if(path.isEmpty())
		return false;

	packages.reserve(3000);

	QFile file(path);

	if(!file.open(QFile::ReadOnly))
		return false;

	while(!file.atEnd()){

		QString line = file.readLine().trimmed();

		if(line.startsWith('#'))
			continue;

		int sep = line.indexOf(':');

		if(sep < 0){
			packages.insert(line, LatexPackageInfo(line));
		} else {
			QString name = line.left(sep);
			packages.insert(name, LatexPackageInfo(name,line.mid(sep + 1)));
		}
	}

	m_dataSource = Static;

	return true;
}


bool LatexRepository::packageExists(const QString & name){
    return packages.contains(name);
}


QString LatexRepository::shortDescription(const QString & name){
	return packages[name].shortDescription;
}
