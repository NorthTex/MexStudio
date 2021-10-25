 
#include "BibTex/Parser.hpp"


using BibTex::FileInfo;


bool FileInfo::loadIfModified(Info info){

    QDateTime filelastModified = info.lastModified();

    if(lateModified == filelastModified)
        return false;

    lateModified = filelastModified;
    load(info);

    return true;
}

void FileInfo::load(Info info){

    QFile file(info.absolutePath());

    if(!file.open(QFile::ReadOnly))
        return;

    QByteArray data = file.readAll().trimmed();

    parse(data);
}

void FileInfo::parse(QByteArray & data){

    ids.clear();
    linksTo.clear();

    if(data.startsWith("link ")){
        linksTo = QString::fromLatin1(data.constData(),data.count()).mid(5).trimmed();
    } else {

        enum State { Space , Type , Id , DataKey };

        State state = Space;

        const char * comment = "comment\0";
        const char * COMMENT = "COMMENT\0";

        int
            typeLength = 0,
            bracketBalance = 0;

        bool commentPossible = false;

        char
            bracketOpen = 0,
            bracketClose = 0;

        QByteArray currentId;

        for(int i = 0;i < data.count();i++){

            char c = data.at(i);

            switch(state){
            case Space:
                if(c == '@'){
                    state = Type;
                    commentPossible = true;
                    typeLength = 0;
                }
                break;
            case Type:
                if(c == '(' || c == '{'){
                    bracketOpen = c;

                    if(c == '(')
                        bracketClose = ')';

                    if(c == '{')
                        bracketClose = '}';

                    bracketBalance = 1;
                    currentId = "";
                    state = Id;
                } else
                if(commentPossible && (comment[typeLength] == c || COMMENT[typeLength] == c)){
                    if(comment[typeLength + 1] == '\0')
                        state = Space;
                } else {
                    commentPossible = false;
                }

                typeLength++;

                break;
            case Id:
                if(c != ' ' && c != '\t' && c != '\n' && c != '\r'){
                    if(c == ',' || c == bracketClose){
                        if(!currentId.isEmpty()){
                            if(codec)
                                ids.insert(codec -> toUnicode(currentId));
                            else
                                ids.insert(currentId);
                        }

                        state = DataKey;
                    } else
                    if(c == '=' || c == '"'){
                        state = DataKey;
                    } else {
                        currentId += c;
                    }
                }
                break;
            case DataKey:
                if(c == bracketOpen)
                    bracketBalance++;
                else
                if(c == bracketClose){
                    bracketBalance--;

                    if(bracketBalance <= 0)
                        state = Space;
                }
                break;
            }

        }
    }
}

