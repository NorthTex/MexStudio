#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QJsonDocument>
#include <QJsonArray>
#include <QThread>

#include "grammarcheck.h"
#include "smallUsefulFunctions.h"


using Tool = GrammarCheckLanguageToolJSON;


const QNetworkRequest::Attribute AttributeTicket =
	static_cast<QNetworkRequest::Attribute>(QNetworkRequest::User);

const QNetworkRequest::Attribute AttributeLanguage =
	static_cast<QNetworkRequest::Attribute>(QNetworkRequest::User + 2);

const QNetworkRequest::Attribute AttributeText =
	static_cast<QNetworkRequest::Attribute>(QNetworkRequest::User + 3);

const QNetworkRequest::Attribute AttributeSubTicket =
	static_cast<QNetworkRequest::Attribute>(QNetworkRequest::User + 4);



struct CheckRequestBackend {

    uint ticket;
    uint subticket;
    QString language;
    QString text;

	CheckRequestBackend(uint ti,uint st,const QString & la,const QString & te)
		: ticket(ti)
		, subticket(st)
		, language(la)
		, text(te) {}
};



Tool::GrammarCheckLanguageToolJSON(QObject * parent)
	: GrammarCheckBackend(parent)
	, nam(nullptr)
	, connectionAvailability(Unknown)
	, triedToStart(false)
	, firstRequest(true)
	, startTime(0) {}

Tool::~GrammarCheckLanguageToolJSON(){
    delete nam;
}


/*!
 * \brief GrammarCheckLanguageToolJSON::init
 * Initialize LanguageTool as grammar backend
 * \param config reference to config
 */

void Tool::init(const GrammarCheckerConfig & config){

    QString url = config.languageToolURL;

    if(!url.endsWith("/v2/check")){
    
        if(!config.languageToolURL.endsWith('/'))
            url += '/';

        url += "v2/check";
    }
    
    //protocol missing, set to default

    if(!url.contains("://"))
        url.prepend("http://");

    server = url;

    ltPath = config.languageToolAutorun ? config.languageToolPath : "";
    ltPath.replace("[txs-settings-dir]",config.configDir);
    ltPath.replace("[txs-app-dir]",config.appDir);

    if(!ltPath.endsWith("jar")){
      
        QStringList jars;
        jars 
            << "/LanguageTool.jar" 
            << "/languagetool-server.jar" 
            << "/languagetool-standalone.jar";
      
        for(const auto & j : jars)
            if(QFile::exists(ltPath + j)){
                ltPath = ltPath + j;
                break;
            }
    }

    ltArguments = config.languageToolArguments;
    javaPath = config.languageToolJavaPath;

    ignoredRules.clear();

    for(const auto & rule : config.languageToolIgnoredRules.split(','))
        ignoredRules << rule.trimmed();

    connectionAvailability = Unknown;
    
    if(config.languageToolURL.isEmpty())
        connectionAvailability = Broken;
    
    triedToStart = false;
    firstRequest = true;

    specialRules.clear();

    const auto Ids = {
        & config.specialIds1,
        & config.specialIds2,
        & config.specialIds3,
        & config.specialIds4
    };

    // QList<const QString *> Ids = QList<const QString *>() 
    //     <<  
    //     << & config.specialIds2 
    //     << & config.specialIds3 
    //     << & config.specialIds4;
    
    for(const auto * id : Ids){

        QSet<QString> rules;
        
        for(const auto & rule : id -> split(','))
            rules << rule.trimmed();
        
        specialRules << rules;
    }
}


/*!
 * \brief GrammarCheckLanguageToolJSON::isAvailable
 * \return LanguageTool is available (or possibly so)
 */

bool Tool::isAvailable(){
    switch(connectionAvailability){
    case WorkedAtLeastOnce:
    case Unknown:
        return true;
    default:
        return false;
    }
    // return (connectionAvailability == WorkedAtLeastOnce) || 
        //    (connectionAvailability == Unkown);
}


/*!
 * \brief GrammarCheckLanguageToolJSON::isWorking
 * \return LanguageTool is available and *known* to work
 */

bool Tool::isWorking(){
    return connectionAvailability == WorkedAtLeastOnce;
}

QString Tool::url(){
    return server.toDisplayString();
}


/*!
 * \brief GrammarCheckLanguageToolJSON::tryToStart
 * try to start LanguageTool-Server on local machine
 */

void Tool::tryToStart(){

    if(triedToStart){

        const auto now = QDateTime::currentDateTime().toSecsSinceEpoch();

        if(now - startTime < 60 * 1000 ){
            connectionAvailability = Unknown;
            ThreadBreaker::sleep(1);
        }

        return;
    }

    triedToStart = true;
    startTime = 0;

    if(ltPath == "")
        return;
    
    if(!QFileInfo::exists(ltPath)){
        errorText = QString("LT path \" %1 \" not found !").arg(ltPath);
        emit errorMessage(errorText);
        return;
    }

    javaProcess = new QProcess();

    connect(javaProcess,SIGNAL(finished(int,QProcess::ExitStatus)),javaProcess,SLOT(deleteLater()));
    connect(this,SIGNAL(destroyed()),javaProcess,SLOT(deleteLater()));

    javaProcess -> start(quoteSpaces(javaPath),QStringList()
        << "-cp" 
        << quoteSpaces(ltPath) 
        << ltArguments.split(' ')
    ); // check sdm

    javaProcess -> waitForStarted(500);
    javaProcess -> waitForReadyRead(500);
    
    errorText = javaProcess -> readAllStandardError();
    
    if(!errorText.isEmpty())
        emit errorMessage(errorText);

    connectionAvailability = Unknown;

    //TODO: fix this in year 2106 when hopefully noone uses qt4.6 anymore
    startTime = QDateTime::currentDateTime().toSecsSinceEpoch(); 
}


/*!
 * \brief GrammarCheckLanguageToolJSON::check
 * Place data to be checked on LT-Server
 * \param ticket
 * \param subticket
 * \param language
 * \param text
 */

void Tool::check(uint ticket,uint subticket,const QString & language,const QString & text){
    
    if(!nam){
        nam = new QNetworkAccessManager();
        connect(nam,SIGNAL(finished(QNetworkReply *)),SLOT(finished(QNetworkReply *)));
    }

    REQUIRE(nam);

    QString lang = language;


    // chop language code to country_variant

    if(lang.count('-') > 1)
        lang = lang.left(5);
    
    if(languagesCodesFail.contains(lang) && lang.contains('-'))
        lang = lang.left(lang.indexOf('-'));

    if(connectionAvailability == Unknown){
        if(firstRequest){
            firstRequest = false;
        } else {
            delayedRequests << CheckRequestBackend(ticket,subticket,lang,text);
            return;
        }
    }

    QNetworkRequest request(server);
    request.setHeader(QNetworkRequest::ContentTypeHeader,"text/json");

    QString post;
    post.reserve(text.length() + 50);
    post.append("language=" + lang + "&text=");
    post.append(QUrl::toPercentEncoding(text,QByteArray(),QByteArray(" ")));
    post.append('\n');

    request.setAttribute(AttributeSubTicket,subticket);
    request.setAttribute(AttributeLanguage,lang);
    request.setAttribute(AttributeTicket,ticket);
    request.setAttribute(AttributeText,text);

    nam -> post(request,post.toUtf8());
}


/*!
 * \brief GrammarCheckLanguageToolJSON::shutdown
 * shutdown LT-Server
 */

void Tool::shutdown(){

    if(javaProcess){
        javaProcess -> terminate();
        javaProcess -> deleteLater();
    }

    connectionAvailability = Terminated;
    
    if(nam){
        nam -> deleteLater();
        nam = nullptr;
    }
}


/*!
 * \brief GrammarCheckLanguageToolJSON::getLastErrorMessage
 * gets the latest error message which occured when operating LT or the start of LT with java
 * \return error message
 */

QString Tool::getLastErrorMessage(){
    return errorText;
}


/*!
 * \brief GrammarCheckLanguageToolJSON::finished
 * Slot for postprocessing LT data
 * \param nreply
 */

void Tool::finished(QNetworkReply * response){

    if(connectionAvailability == Terminated)
        return;  // shutting down
    
    if(connectionAvailability == Broken)
        return;  // don't continue if failed before
    
    if(nam != sender())
        return; //safety check, in case nam was deleted and recreated

    uint ticket = response -> request().attribute(AttributeTicket).toUInt();
    int subticket = response -> request().attribute(AttributeSubTicket).toInt();
    QString text = response -> request().attribute(AttributeText).toString();
    int status = response -> attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if(status == 0){

        //no response
        connectionAvailability = Broken; //assume no backend
        int error = response -> error();
        
        if(error==1){
            tryToStart();
        } else {
            errorText = response -> errorString();
            emit errorMessage(errorText);
        }

        if(connectionAvailability == Broken){

            if(delayedRequests.size())
                delayedRequests.clear();
            
            nam -> deleteLater(); // shutdown unnecessary network manager (Bug 1717/1738)
            nam = nullptr;
            return; //confirmed: no backend
        }

        //there might be a backend now, but we still don't have the results
        firstRequest = true; //send this request directly, queue later ones
        check(ticket,subticket,response -> request().attribute(AttributeLanguage).toString(),text);
        response -> deleteLater();
        
        return;
    }

    QByteArray reply = response -> readAll();

    // LT announces error in set-up, probably with the language code
    // put error message into status symbol

    if(status== 400 && reply.startsWith("Error:")){
        errorText = QString(reply);
        emit errorMessage(errorText);
    }

    if(
        status == 500 && 
        reply.contains("language code") && 
        reply.contains("IllegalArgumentException")
    ){
        QString lang = response -> request()
            .attribute(AttributeLanguage)
            .toString();
     
        if(lang.contains('-')){
            languagesCodesFail.insert(lang);
            check(ticket,subticket,lang,text);
        }
     
        return;
    }

    if(connectionAvailability!=WorkedAtLeastOnce){
        connectionAvailability = WorkedAtLeastOnce;
        emit languageToolStatusChanged();
    }

    QJsonDocument jsonDoc = QJsonDocument::fromJson(reply);
    QJsonObject dd = jsonDoc.object();
    QList<GrammarError> results;
    QJsonArray matches = dd["matches"].toArray();
    
    for(
        QJsonArray::const_iterator lterrors = matches.constBegin();
        lterrors != matches.constEnd();
        lterrors++
    ){
        
        QJsonValue v = * lterrors;
        QJsonObject obj = v.toObject();

        QJsonObject rule = obj["rule"].toObject();
        QString id = rule["id"].toString();
        
        if(ignoredRules.contains(id))
            continue;
        
        int len = obj["length"].toInt();
        int from = obj["offset"].toInt();
        
        QJsonObject contextObj = obj["context"].toObject();
        QString context = contextObj["text"].toString();
        
        int contextoffset = contextObj["offset"].toInt();
        int realfrom = text.indexOf(context,qMax(from - 5 - context.length(),0)); //don't trust from

        if(realfrom == -1)
            realfrom = from;
        else 
            realfrom += contextoffset;

        QStringList cors;
        QJsonArray repl = obj["replacements"].toArray();
        
        for(const auto & elem : repl){
            QJsonObject i = elem.toObject();
            cors << i["value"].toString();
        }

        if(cors.size() == 1 && cors.first() == "")
            cors.clear();

        int type = GET_BACKEND;
        
        for(int j = 0;j < specialRules.size();j++)
            if(specialRules[j].contains(id)){
                type += j + 1;
                break;
            }

        // TODO: message is only needed in the tooltip and should be formatted there. We should extend the Grammar error to
        // contain the raw information.

        QStringList message;
        QString title = obj["shortMessage"].toString();
        
        if(title.length() > 0)
            message << "<b>" + title + "</b>";
        
        QString description = obj["message"].toString();
        
        if(description.length() > 0 && description != title)
            message << description;
        
        message << ('(' + id + ')');
        
        results 
            << GrammarError(realfrom,len,static_cast<GrammarErrorType>(type),message.join("<br>"), cors);
    }

    emit checked(ticket,subticket,results);

    response -> deleteLater();

    if(delayedRequests.size()){
        
        auto delayedRequestsCopy = delayedRequests;
        delayedRequests.clear();
        
        for(const auto & request : delayedRequestsCopy)
            check(request.ticket,request.subticket,request.language,request.text);
    }
}
