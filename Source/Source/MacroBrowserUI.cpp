#include "macrobrowserui.h"
#include <QNetworkReply>
#include <QNetworkProxyFactory>
#include <QJsonDocument>
#include <QJsonArray>


MacroBrowserUI::MacroBrowserUI(QWidget * parent)
    : QDialog (parent){

    tableWidget = new QTableWidget(4,1);
    tableWidget -> setHorizontalHeaderLabels(QStringList() << "Macro name");
    tableWidget -> horizontalHeader() -> setStretchLastSection(true);

    connect(tableWidget,SIGNAL(itemClicked(QTableWidgetItem *)),SLOT(itemClicked(QTableWidgetItem *)));
    
    auto lblName = new QLabel(tr("Name"));
    lblName -> setAlignment(Qt::AlignRight);

    auto lblDescription = new QLabel(tr("Description"));
    lblDescription -> setAlignment(Qt::AlignRight | Qt::AlignTop);
    
    leName = new QLineEdit();
    leName -> setReadOnly(true);

    teDescription = new QPlainTextEdit();
    teDescription -> setReadOnly(true);
    
    auto gridLay = new QGridLayout();
    gridLay -> setColumnStretch(0,1);
    gridLay -> setColumnStretch(1,0);
    gridLay -> setColumnStretch(2,1);
    gridLay -> addWidget(tableWidget,1,0,5,1);
    gridLay -> addWidget(lblName,1,1);
    gridLay -> addWidget(lblDescription,2,1);
    gridLay -> addWidget(leName,1,2);
    gridLay -> addWidget(teDescription,2,2);

    buttonBox = new QDialogButtonBox();
    buttonBox -> setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    
    connect(buttonBox,& QDialogButtonBox::accepted,this,& QDialog::accept);
    connect(buttonBox,& QDialogButtonBox::rejected,this,& QDialog::reject);
    
    auto layout = new QVBoxLayout();
    layout -> addLayout(gridLay);
    layout -> addWidget(buttonBox);
    
    setLayout(layout);
    setWindowTitle(tr("Browse macros from repository"));

    config = dynamic_cast<ConfigManager *>(ConfigManagerInterface::getInstance());
    networkManager = new QNetworkAccessManager();

    requestMacroList("");
}


MacroBrowserUI::~MacroBrowserUI(){

    networkManager -> deleteLater();
    networkManager = nullptr;

    for(auto items : itemCache)
        for(auto item : items)
            delete item;
}


QList<Macro> MacroBrowserUI::getSelectedMacros(){

    QList<Macro> macros;

    for(auto items : itemCache)
        for(auto item : items)
            if(item -> checkState() == Qt::Checked){

                const auto url = item 
                    -> data(Qt::UserRole)
                    .  toString();
                
                const auto json = cache.value(url);
                
                if(!json.isEmpty()){
                    Macro macro;
                    macro.loadFromText(json);
                    macros << macro;
                }
            }

    return macros;
}


const QNetworkRequest::Attribute AttributeDirectURL = 
    static_cast<QNetworkRequest::Attribute>(QNetworkRequest::User);

const QNetworkRequest::Attribute AttributeURL = 
    static_cast<QNetworkRequest::Attribute>(QNetworkRequest::User + 1);


void MacroBrowserUI::requestMacroList(const QString & path,const bool & directURL){
    
    if(!networkManager)
        return;

    QString url = config -> URLmacroRepository+path;
    
    //QString url="https://api.github.com/repos/sunderme/texstudio-macro/contents/"+path;
    
    if(directURL)
        url = path;
    else
        currentPath = path;

    auto request = QNetworkRequest(QUrl(url));
    request.setRawHeader("User-Agent","TeXstudio Macro Browser");
    request.setAttribute(AttributeDirectURL,directURL);
    request.setAttribute(AttributeURL,url);
    
    auto reply = networkManager -> get(request);
    connect(reply,SIGNAL(finished()),SLOT(onRequestCompleted()));
    connect(reply,SIGNAL(error(QNetworkReply::NetworkError)),SLOT(onRequestError()));
}


void MacroBrowserUI::itemClicked(QTableWidgetItem * item){

    auto url = item 
        -> data(Qt::UserRole)
        .  toString();
    

    // descend into folder

    if(url.isEmpty()){

        if(item -> text() == ".."){
            int c = currentPath.lastIndexOf('/');
            url = currentPath.left(c);
        } else {
            url = currentPath + "/" + item -> text();
        }

        // reuse cached

        if(itemCache.contains(url)){
            
            currentPath = url;
            
            int i = 0;
            
            for(int i = 0;i < tableWidget -> rowCount();i++)
                tableWidget -> takeItem(i,0);

            if(!url.isEmpty()){
                auto item = new QTableWidgetItem(QIcon::fromTheme("file"),"..");
                tableWidget -> setRowCount(i + 1);
                tableWidget -> setItem(i++,0,item);
            }

            for(auto item : itemCache.value(url)){
                tableWidget -> setRowCount(i + 1);
                tableWidget -> setItem(i++,0,item);
            }
        } else { 
            requestMacroList(url);
        }
    } else {
        requestMacroList(url,true);
    }
}


void MacroBrowserUI::onRequestError(){

    auto reply = qobject_cast<QNetworkReply *>(sender());
    
    if(!reply)
        return;

    QMessageBox::warning(this,
        tr("Browse macro repository"),
        tr("Repository not found. Network error:%1").arg(reply -> errorString()),
        QMessageBox::Ok,
        QMessageBox::Ok
    );

    networkManager -> deleteLater();
    networkManager = nullptr;
}


void MacroBrowserUI::onRequestCompleted(){

    auto reply = qobject_cast<QNetworkReply *>(sender());
    
    if(!reply)
        return;
        
    if(reply -> error() != QNetworkReply::NoError)
        return;

    QByteArray bytes = reply -> readAll();

    // download requested

    if(reply -> request().attribute(AttributeDirectURL).toBool()){

        QJsonDocument document = QJsonDocument::fromJson(bytes);
        QJsonObject json = document.object();
        leName -> setText(json["name"].toString());
        QJsonArray array = json["description"].toArray();
        QVariantList vl = array.toVariantList();
        QString text;

        for(auto v : vl){
            
            if(!text.isEmpty())
                text += "\n";

            text += v.toString();
        }

        teDescription -> setPlainText(text);
        
        // cache complete macro
        
        QString url = reply 
            -> request()
            .  attribute(AttributeURL)
            .  toString();
        
        cache.insert(url,QString(bytes));
    } else {

        // folder overview requested
        // tableWidget->clearContents();
        
        for(int i = 0;i < tableWidget -> rowCount();i++)
            tableWidget -> takeItem(i,0);

        auto document = QJsonDocument::fromJson(bytes);
        QJsonArray elements = document.array();
        int i = 0;

        // add .. (up)
        
        if(!currentPath.isEmpty()){
            auto item = new QTableWidgetItem(QIcon::fromTheme("file"),"..");
            tableWidget -> setRowCount(i + 1);
            tableWidget -> setItem(i++,0,item);
        }

        QList<QTableWidgetItem*> listOfItems;
        
        for(auto element : elements){

            QJsonObject json = element.toObject();
            
            if(json["type"].toString() == "file"){
                
                QString name = json["name"].toString();
                
                if(name.endsWith(".txsMacro")){
                    auto item = new QTableWidgetItem(QIcon::fromTheme("file"),name);
                    item -> setData(Qt::UserRole,json["download_url"].toString());
                    item -> setCheckState(Qt::Unchecked);
                    
                    tableWidget -> setRowCount(i + 1);
                    tableWidget -> setItem(i++,0,item);
                    
                    if(i == 1)
                        requestMacroList(item -> data(Qt::UserRole).toString(),true);

                    listOfItems<<item;
                }
            } else {

                // folder
                
                QString name = json["name"].toString();
                
                auto item = new QTableWidgetItem(QIcon::fromTheme("folder"),name);
                tableWidget -> setRowCount(i + 1);
                tableWidget -> setItem(i++,0,item);
                
                listOfItems << item;
            }
        }

        tableWidget -> setCurrentCell(0,0);
        itemCache.insert(currentPath,listOfItems);
    }
}



