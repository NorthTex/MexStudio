
#include "configmanager.h"
#include "Dialogs/File.hpp"

#include <QFileDialog>


int defaultOptions(){
    
    const bool useNative = dynamic_cast<ConfigManager*>(ConfigManager::getInstance()) -> useNativeFileDialog;
    
    return (useNative) 
        ? 0 
        : QFileDialog::DontUseNativeDialog ;
}


QString FileDialog::getOpenFileName(
    QWidget * parent,
    const QString & caption,
    const QString & dir,
    const QString & filter,
    QString * selectedFilter,
    int options
){
    return QFileDialog::getOpenFileName(
        parent,caption,dir,filter,selectedFilter,
        QFileDialog::Options( options | defaultOptions())
    );
}


QString FileDialog::getSaveFileName(
    QWidget * parent,
    const QString & caption,
    const QString & dir,
    const QString & filter,
    QString * selectedFilter,
    int options
){
    return QFileDialog::getSaveFileName(
        parent,caption,dir,filter,selectedFilter,
        QFileDialog::Options( options | defaultOptions())
    );
}


QStringList FileDialog::getOpenFileNames(
    QWidget * parent,
    const QString & caption,
    const QString & dir,
    const QString & filter,
    QString * selectedFilter,
    int options
){
    return QFileDialog::getOpenFileNames(
        parent,caption,dir,filter,selectedFilter,
        QFileDialog::Options( options | defaultOptions())
    );
}
