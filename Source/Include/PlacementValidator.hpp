#ifndef Header_PlacementValidator
#define Header_PlacementValidator


#include <QRegularExpressionValidator>


class PlacementValidator : public QRegularExpressionValidator {

	Q_OBJECT

	public:

		explicit PlacementValidator(QObject * parent = nullptr);

	public:

		State validate(QString & input,int & position) const;

		void fixup (QString & input) const;
};


#endif
