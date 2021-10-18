#ifndef Header_Encoding
#define Header_Encoding

#include "mostQtHeaders.h"

namespace Encoding {

    enum {
        MIB_WINDOWS1252 = 2252,
        MIB_UTF16BE = 1013,
        MIB_UTF16LE = 1014,
        MIB_LATIN1 = 4,
        MIB_UTF8 = 106
    };

    QTextCodec * QTextCodecForLatexName(QString latexName);
    QStringList latexNamesForTextCodec(const QTextCodec * textCodec);

    QTextCodec * guessEncodingBasic(const QByteArray & data,int * outSure);
    void guessEncoding(const QByteArray & data,QTextCodec * & guess, int & sure);


    namespace Internal {

        int lineStart(const QByteArray & data,int index);
        int lineEnd(const QByteArray & data,int index);

        QTextCodec * QTextCodecForTeXShopName(const QByteArray & texShopName);
        QString getEncodingFromPackage(const QByteArray & data,int headerSize,const QString & package);

    }
}

#endif
