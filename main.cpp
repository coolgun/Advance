#include "Advance.h"
#include <QtWidgets/QApplication>
#include <QSettings>
#include <QStandardPaths>
#include <QInputDialog>
QString user_name;

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	QCoreApplication::setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents);
	QCoreApplication::setAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents);
	
	const auto filename = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/adv_user.ini";
	QSettings settings(filename, QSettings::IniFormat);
	
	const auto val = settings.value("user");

	if (val.isNull())
	{

		while (true)
		{
			bool ok{};
			user_name = QInputDialog::getText({}, "User Name", "name:", QLineEdit::Normal, "New User", &ok);
			if (!ok) return 0;
			if (!user_name.isEmpty())
			{
				break;
			}
		}
		settings.setValue("user", user_name);
	}
	else
		user_name = val.toString();

	Advance w;
	w.setWindowTitle(QString("Advance -%1").arg(user_name));
	w.show();
	return a.exec();
}
