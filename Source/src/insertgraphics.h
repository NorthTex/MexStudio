/***************************************************************************
 *   copyright       : (C) 2003-2007 by Pascal Brachet,Jan Sundermeyer     *
 *   http://www.xm1math.net/texmaker/                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef Header_InsertGraphics
#define Header_InsertGraphics


#include "ui_insertgraphics.h"
#include "QFileInfo"
#include "QListWidgetItem"


class InsertGraphicsConfig;


class PlacementValidator : public QRegularExpressionValidator {

	Q_OBJECT

	public:

		explicit PlacementValidator(QObject * parent = nullptr);

		State validate(QString & input,int & pos) const;
		void fixup (QString & input) const;

};


class InsertGraphics : public QDialog {

	Q_OBJECT

	public:

		InsertGraphics(QWidget * parent = nullptr,InsertGraphicsConfig * = nullptr);

		Q_INVOKABLE QString graphicsFile() const;
		QString getLatexText() const;

		static QStringList imageFormats();

	private:

		enum PlacementType {PlaceHere = QListWidgetItem::UserType, PlaceTop, PlaceBottom, PlacePage};

		InsertGraphicsConfig getConfig() const;

		bool parseCode(const QString & code,InsertGraphicsConfig &);
		bool fileNeedsInputCommand(const QString & filename) const;

		void setConfig(const InsertGraphicsConfig &);

		QString getCaptionLabelString(const InsertGraphicsConfig &) const;
		QString getFormattedFilename(const QString filename) const;

		QString generateLabel(QString fname);

		Ui::InsertGraphics ui;

		QFileInfo texFile;
		QFileInfo masterTexFile;

		bool autoLabel;

		static QStringList m_imageFormats;
		static QStringList heightUnits;
		static QStringList widthUnits;

		InsertGraphicsConfig * defaultConfig;

	public slots:

		void setMasterTexFile(const QFileInfo &);
		void setGraphicsFile(const QString & file);
		void setTexFile(const QFileInfo &);
		void setCode(const QString & code);

	signals:

		void fileNameChanged(const QString &);

	private slots:

		void includeOptionChanged();
		void updatePlacement();
		void saveDefault();
		void chooseFile();

		void togglePlacementCheckboxes(bool forceHide = false);
		void leFileChanged(const QString & filename);
		void labelChanged(const QString & label);
		void updateLabel(const QString & fname);

};


#endif
