/***************************************************************************
 *   copyright       : (C) 2009 by Jan Sundermeyer                         *
 *   http://texstudio.sourceforge.net/                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef Header_Template_Selector
#define Header_Template_Selector

#include "mostQtHeaders.h"
#include "Template/Handle.hpp"
#include "Template/AbstractResource.hpp"
#include "ui_templateselector.h"


// a label to show pixmaps. It shinks the pixmap if necessary to fit it into the available space.
// but it does not magnify it if more space is available

class PreviewLabel : public QLabel {

	Q_OBJECT

	public:

		explicit PreviewLabel(QWidget * parent = 0)
			: QLabel(parent) {

			setScaledPixmap(QPixmap());
		}

		void setScaledPixmap(const QPixmap & pm);

	protected:

		virtual void resizeEvent(QResizeEvent * event);

	private:

		void setPixmapWithResizing(const QPixmap & pm);
		QPixmap currentPixmap;

};


class TemplateSelector : public QDialog {

	Q_OBJECT

	public:

		explicit TemplateSelector(QString name = "",QWidget * parent = 0);
		~TemplateSelector();

		TemplateHandle selectedTemplate() const;
		bool createInFolder() const;
		QString creationFolder() const;
		void addResource(AbstractTemplateResource *);
		void hideFolderSelection();

	signals:

		void editTemplateRequest(TemplateHandle);
		void editTemplateInfoRequest(TemplateHandle);

	private slots:

		void showInfo(QTreeWidgetItem * current,QTreeWidgetItem * previous);
		void templatesTreeContextMenu(QPoint);

		void on_templatesTree_doubleClicked(const QModelIndex &);
		void on_btPath_clicked();
		void checkTargetPath();

		void editTemplate();
		void editTemplateInfo();
		void removeTemplate();
		void openTemplateLocation();

	private:

		QString orDefault(const QString & value,const QString & defaultIfValEmpty) const {
			return (value.isEmpty()) ? defaultIfValEmpty : value;
		}

		Ui::templateSelectorDialog ui;
		PreviewLabel * previewLabel;

		static const int TemplateHandleRole;
		static const int ResourceRole;
};


#endif
