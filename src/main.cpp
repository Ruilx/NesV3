#include "MainWindow.h"

#include <QApplication>
#include <QDebug>
#include <QLocale>
#include <QStringList>
#include <QTimer>
#include <QTranslator>

namespace {
bool parseBenchmarkArguments(
	const QStringList &arguments,
	QStringList &romPaths,
	int &perRomMilliseconds) {
	int index = 1;
	while (index < arguments.size()) {
		const QString argument = arguments.at(index);
		if (argument == QStringLiteral("--benchmark")) {
			if (index + 2 >= arguments.size()) {
				return false;
			}
			romPaths = {arguments.at(index + 1), arguments.at(index + 2)};
			index += 3;
			continue;
		}
		if (argument == QStringLiteral("--per-rom-ms")) {
			if (index + 1 >= arguments.size()) {
				return false;
			}
			bool ok = false;
			perRomMilliseconds = arguments.at(index + 1).toInt(&ok);
			if (!ok || perRomMilliseconds <= 0) {
				return false;
			}
			index += 2;
			continue;
		}
		return false;
	}
	return !romPaths.isEmpty();
}
}

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

    QApplication::setApplicationName("NesV3");
    QApplication::setApplicationVersion("1.0.0");
    QApplication::setOrganizationName("GT-Soft Studio");
    QApplication::setOrganizationDomain("https://github.com/Ruilx/NesV3");

	QTranslator translator;
	const QStringList uiLanguages = QLocale::system().uiLanguages();
	for (const QString &locale : uiLanguages) {
		const QString baseName = "NesV3_" + QLocale(locale).name();
		if (translator.load(":/i18n/" + baseName)) {
			QApplication::installTranslator(&translator);
			break;
		}
	}
	MainWindow w;
	w.show();

	QStringList benchmarkRomPaths;
	int perRomMilliseconds = 15000;
	const QStringList arguments = QCoreApplication::arguments();
	const bool benchmarkRequested = arguments.size() > 1;
	if (benchmarkRequested && !parseBenchmarkArguments(
	        arguments, benchmarkRomPaths, perRomMilliseconds)) {
		qCritical().noquote()
			<< "Usage: NesV3 --benchmark <rom1> <rom2>"
			<< "[--per-rom-ms <milliseconds>]";
		return 2;
	}
	if (benchmarkRequested && !w.startBenchmark(
	        benchmarkRomPaths, perRomMilliseconds)) {
		qCritical() << "Unable to start benchmark";
		return 2;
	}
	return QApplication::exec();
}
