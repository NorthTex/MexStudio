 
#include "BibTex/Reader.hpp"
#include "configmanagerinterface.h"


using BibTex::Reader;


Reader::Reader(QObject * parent)
    : SafeThread(parent) {}


QString truncateLine(const QString & line,int limit){

    if(line.length() < limit)
        return line;

    int position = line.lastIndexOf(' ',limit - 3);

    if(position < 0)
        position = limit - 3;

    return line.left(position + 1) + '[' + QChar(8230) + ']';
}


void Reader::searchSection(QString path,QString id,int limit){

    QFile file(path);

    if(!file.open(QFile::ReadOnly))
        return;

    QTextStream stream(& file);
    QString encoding = ConfigManagerInterface::getInstance() -> getOption("Bibliography/BibFileEncoding").toString();

    std::optional<QStringConverter::Encoding> codec = QStringConverter::encodingForName(encoding.toLocal8Bit());

    if(!codec.has_value()){

        qDebug()
            << "bibtexReader::searchSection text codec not recognized in QT6:"
            << encoding;

        return;
    }

    stream.setEncoding(codec.value());

    QString line , result;
    int found = -1;

    do {

        line = stream.readLine();

        if(line.startsWith('%'))
            continue;

        if(found == -1 && line.contains(id)){
            int index = line.indexOf('{');

            if(index < 0)
                continue;

            QString lid = line.mid(index + 1).trimmed();

            if(lid.endsWith(','))
                lid.remove(lid.length() - 1,1);

            if(lid != id)
                continue;

            found = 10;
            result = truncateLine(line,limit);
            continue;
        }

        if(found == 0)
            break;

        if(found > 0 && line.startsWith('@'))
            break;

        if(line.isEmpty())
            break;

        if(found < 0)
            continue;

        result += '\n' + truncateLine(line,limit);
        found--;
    } while(!line.isNull());

    if(!result.isEmpty())
        emit sectionFound(result);
}
