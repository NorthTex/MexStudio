
#include "updatechecker.h"
#include "smallUsefulFunctions.h"
#include "utilsVersion.h"
#include "configmanager.h"

#include <QNetworkReply>
#include <QNetworkProxyFactory>
#include <QMutex>


UpdateChecker * UpdateChecker::m_Instance = nullptr;

UpdateChecker::UpdateChecker() 
	: QObject(nullptr)
	, silent(true) {

	QNetworkProxyFactory::setUseSystemConfiguration(true);

    // activate networkManager only when directly needed 
	// as it causes network delays on mac (Bug 1717/1738)

    networkManager=nullptr;
}

UpdateChecker::~UpdateChecker(){
    m_Instance = nullptr;
}

QString UpdateChecker::lastCheckAsString(){

	auto lastCheck = ConfigManager::getInstance()
		-> getOption("Update/LastCheck")
		.  toDateTime();
	
	return lastCheck.isValid() 
		? lastCheck.toString("d.M.yyyy hh:mm") 
		: tr("Never", "last update");
}


void UpdateChecker::autoCheck(){

	auto config = ConfigManager::getInstance();
	
	bool autoCheck = config
		-> getOption("Update/AutoCheck")
		.  toBool();
	
	if(autoCheck){

		auto lastCheck = config 
			-> getOption("Update/LastCheck")
			.  toDateTime();
		
		int checkInterval = config
			-> getOption("Update/AutoCheckInvervalDays")
			.  toInt();
		
		if(
			!lastCheck.isValid() || 
			lastCheck.addDays(checkInterval) < QDateTime::currentDateTime()
		)	check();
	}

}


void UpdateChecker::check(bool silent){

	this -> silent = silent;
    networkManager = new QNetworkAccessManager();
    
	auto request = QNetworkRequest(QUrl("https://api.github.com/repos/texstudio-org/texstudio/git/refs/tags"));
	request.setRawHeader("User-Agent", "TeXstudio Update Checker");
	
	auto reply = networkManager -> get(request);

	connect(reply,SIGNAL(finished()),this,SLOT(onRequestCompleted()));
	
	if(silent)
		return;

	connect(reply,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(onRequestError()));
}


void UpdateChecker::onRequestError(){

	auto reply = qobject_cast<QNetworkReply *>(sender());
	
	if(!reply)
		return;

	UtilsUi::txsCritical(tr("Update check failed with error:\n") + reply -> errorString());

    networkManager -> deleteLater();
    networkManager = nullptr;
}


void UpdateChecker::onRequestCompleted(){

	auto reply = qobject_cast<QNetworkReply *>(sender());

	if(!reply)
		return;
		
	if(reply -> error() != QNetworkReply::NoError)
		return;

	auto bytes = reply -> readAll();

	parseData(bytes);
	checkForNewVersion();

    networkManager -> deleteLater();
    networkManager = nullptr;
}


void UpdateChecker::parseData(const QByteArray & data){
    
    // simple,dirty parsing of github api result (tags)

    QString result = QString(data);
    QStringList lines = result.split(",");
    QStringList tags;

    for(const auto & line : lines){

        int pos = line.indexOf("\"ref\"");
        
		if(pos >= 0){
            QString zw = line.mid(pos + 6).simplified();
            zw.remove("\"");
            zw.remove("refs/tags/");
            tags<<zw;
        }
    }

    for(int j = tags.length() - 1;j >= 0;j--){
        
		QString tag = tags.value(j);
        QRegExp rx("^((\\d+\\.)+(\\d+))([a-zA-Z]+)?(\\d*)?$");
        
		if(rx.indexIn(tag) == 0){
            
			QString ver = rx.cap(1);
            QString type = rx.cap(4);
            
			qDebug()
				<< ver
				<< type;
            
			if(type.toLower() == "rc"){
                Version version;
                version.versionNumber = ver;
                version.type = "release candidate";
                version.revision = rx.cap(5).toInt();
                latestReleaseCandidateVersion = version;
            }
            
			if(type.toLower() == "beta"){
                Version version;
                version.versionNumber = ver;
                version.type = "beta";
                version.revision = rx.cap(5).toInt();
                latestDevVersion = version;
            }

            if(type.isEmpty()){

                Version version;
                version.versionNumber = ver;
                version.type = "stable";

				// unused in stable

                version.revision = 0;

                latestStableVersion = version;

                if(!latestDevVersion.isValid())
                    latestDevVersion = version;
                
				if(!latestReleaseCandidateVersion.isValid())
                    latestReleaseCandidateVersion = version;
                
				// all other versions are older, so abort after first release

				break;
            }
        }
    }
}


const auto
	template_stable =
		"A new version of TeXstudio is available.<br>"
		"<table><tr><td>Current version:</td><td>%1</td></tr>"
		"<tr><td>Latest stable version:</td><td>%2</td></tr>"
		"</table><br><br>"
		"You can download it from the <a href='%3'>TeXstudio website</a>."
	,
	
	template_development =
		"A new development version of TeXstudio is available.<br>"
		"<table><tr><td>Current version:</td><td>%1 (%2)</td></tr>"
		"<tr><td>Latest stable version:</td><td>%3 (%4)</td></tr>"
		"<tr><td>Latest development version:</td><td>%5 (beta%6)</td></tr>"
		"</table><br><br>"
		"You can download it from the <a href='%7'>TeXstudio website</a>."
	,
	
	template_candidate =
		"A new release candidate of TeXstudio is available.<br>"
		"<table><tr><td>Current version:</td><td>%1 (%2)</td></tr>"
		"<tr><td>Latest stable version:</td><td>%3 (%4)</td></tr>"
		"<tr><td>Release candidate:</td><td>%5 (rc%6)</td></tr>"
		"</table><br><br>"
		"You can download it from the <a href='%7'>TeXstudio website</a>."
	;

const auto
	warning_update = "Update check failed (invalid update file format)." ,
	warning_candidate = "Update check for release candidate failed (invalid update file format)." ,
	warning_development = "Update check for development version failed (invalid update file format).";


void UpdateChecker::checkForNewVersion(){

	// updateLevel values from comboBoxUpdateLevel indices:
	// 0: stable, 1: release candidate, 2: development
	
	int updateLevel = ConfigManager::getInstance()
		-> getOption("Update/UpdateLevel")
		.  toInt();

	bool 
		checkReleaseCandidate = (updateLevel >= 1),
		checkDevVersions = (updateLevel >= 2);

	Version currentVersion = Version::current();

    QString 
		downloadAddressBeta = "https://github.com/texstudio-org/texstudio/releases",
		downloadAddress = "https://texstudio.org";

	if(!latestStableVersion.isValid()){
		
		if(!silent)
			UtilsUi::txsWarning(warning_update);
		
		return;
	}

	// single control loop, used be able to break the control flow when some new version is detected

	while(true){
		
		if(checkReleaseCandidate && !latestReleaseCandidateVersion.isEmpty()){

			if(!latestReleaseCandidateVersion.isValid() && !silent)
				UtilsUi::txsWarning(tr(warning_candidate));
			
			if(
				latestReleaseCandidateVersion > currentVersion &&
				latestReleaseCandidateVersion > latestStableVersion
			){
				const auto message = tr(template_candidate)
					.arg(currentVersion.versionNumber)
					.arg(currentVersion.revision)
					.arg(latestStableVersion.versionNumber)
					.arg(latestStableVersion.revision)
					.arg(latestReleaseCandidateVersion.versionNumber)
					.arg(latestReleaseCandidateVersion.revision)
					.arg(downloadAddressBeta);

				notify(message);
	
				break;
			}
		}
	
		if(checkDevVersions && !latestDevVersion.isEmpty()){
			
			if(!latestDevVersion.isValid() && !silent)	
				UtilsUi::txsWarning(tr(warning_development));

			if(latestDevVersion > currentVersion && latestDevVersion > latestStableVersion) {
				
				const auto message = tr(template_development)
					.arg(currentVersion.versionNumber)
					.arg(currentVersion.revision)
					.arg(latestStableVersion.versionNumber)
					.arg(latestStableVersion.revision)
					.arg(latestDevVersion.versionNumber)
					.arg(latestDevVersion.revision)
					.arg(downloadAddressBeta);

				notify(message);
				
				break;
			}
		}

		if(latestStableVersion > currentVersion){
			
			const auto message = tr(template_stable)
				.arg(currentVersion.versionNumber)
				.arg(latestStableVersion.versionNumber)
				.arg(downloadAddress);
			
			notify(message);
		} else {
			if(!silent)
				UtilsUi::txsInformation(tr("TeXstudio is up-to-date."));
		}

		break;
	}

	ConfigManager::getInstance()
		-> setOption("Update/LastCheck",QDateTime::currentDateTime());
	
	emit checkCompleted();
}


void UpdateChecker::notify(QString message){

	QMessageBox box;
	box.setWindowTitle(tr("TeXstudio Update"));
	box.setTextFormat(Qt::RichText);
	box.setText(message);
	box.setStandardButtons(QMessageBox::Ok);
	box.exec();
}


UpdateChecker * UpdateChecker::instance(){

	static QMutex mutex;
	mutex.lock();

	if(!m_Instance)
		m_Instance = new UpdateChecker();

	mutex.unlock();

	return m_Instance;
}
