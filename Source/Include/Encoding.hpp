#ifndef Header_Encoding
#define Header_Encoding


#include <QTextCodec>


namespace Encoding {

    enum {
        MIB_WINDOWS1252 = 2252,
        MIB_UTF16BE = 1013,
        MIB_UTF16LE = 1014,
        MIB_LATIN1 = 4,
        MIB_UTF8 = 106
    };

    using Codec = QTextCodec;
    using Bytes = QByteArray;

    Codec * QTextCodecForLatexName(QString);
    QStringList latexNamesForTextCodec(const Codec *);

    Codec * guessEncodingBasic(const Bytes & data,int * outSure);
    void guessEncoding(const Bytes & data,Codec * & guess, int & sure);


    namespace Internal {

        int lineStart(const Bytes & data,int index);
        int lineEnd(const Bytes & data,int index);

        Codec * QTextCodecForTeXShopName(const Bytes &);
        QString getEncodingFromPackage(const Bytes & data,int headerSize,const QString & package);

    }
}

#endif
