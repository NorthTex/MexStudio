#ifndef Header_InsertGraphics
#define Header_InsertGraphics


#include "ui_insertgraphics.h"

#include <QListWidgetItem>
#include <QFileInfo>
#include <QDialog>


class InsertGraphicsConfig;


class InsertGraphics : public QDialog {

	Q_OBJECT

	private:

		enum PlacementType {
			PlaceHere = QListWidgetItem::UserType ,
			PlaceTop ,
			PlaceBottom ,
			PlacePage
		};

	private:

		static QStringList
			m_imageFormats,
			heightUnits,
			widthUnits;

	private:

		InsertGraphicsConfig * defaultConfig;
		Ui::InsertGraphics ui;

		QFileInfo texFile;
		QFileInfo masterTexFile;

		bool autoLabel;

	private:

		InsertGraphicsConfig getConfig() const;

		bool parseCode(const QString & code,InsertGraphicsConfig &);
		bool fileNeedsInputCommand(const QString & filename) const;

		void setConfig(const InsertGraphicsConfig &);

		QString getCaptionLabelString(const InsertGraphicsConfig &) const;
		QString getFormattedFilename(const QString) const;
		QString generateLabel(QString filename);

	private slots:

		void togglePlacementCheckboxes(bool forceHide = false);
		void leFileChanged(const QString & filename);
		void labelChanged(const QString & label);
		void updateLabel(const QString & fname);

		void includeOptionChanged();
		void updatePlacement();
		void saveDefault();
		void chooseFile();

	signals:

		void fileNameChanged(const QString &);

	public:

		InsertGraphics(QWidget * parent = nullptr,InsertGraphicsConfig * = nullptr);

		Q_INVOKABLE QString graphicsFile() const;
		QString getLatexText() const;

		static QStringList imageFormats();

	public slots:

		void setMasterTexFile(const QFileInfo &);
		void setGraphicsFile(const QString &);
		void setTexFile(const QFileInfo &);
		void setCode(const QString &);
};


#endif
