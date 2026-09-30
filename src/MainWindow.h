#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QElapsedTimer>
#include <QFile>
#include <QStringList>
#include <QTimer>
#include <QString>
#include <QTextStream>
#include <QVector>

#include "Nes.h"

class NesScene;
class NesView;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = nullptr);
	~MainWindow() override;
	bool startBenchmark(
		const QStringList &romPaths, const QVector<int> &durationsMilliseconds);

private:
	void savePalette();
	void openPaletteEditor();
	void openPatternTableDialog();
	void openRom();
	void closeRom();
	bool loadRomFromPath(
		const QString &path,
		QString *errorMessage = nullptr,
		const QString &testTarget = QStringLiteral("manual"),
		int testDurationMilliseconds = 0);
	void runNextBenchmarkRom();
	void openPerformanceLog(
		const QString &romPath,
		const QString &testTarget,
		int testDurationMilliseconds);
	void closePerformanceLog();
	void runSimulationFrame();
	void updateWindowTitle();
	void setDebugInputMode(bool enabled);

	Nes nes;
	NesScene *scene = nullptr;
	NesView *view = nullptr;
	QTimer simulationTimer;
	QElapsedTimer performanceTimer;
	QFile performanceLogFile;
	QTextStream performanceLogStream;
	quint64 performanceFrameCount = 0;
	quint64 performanceCoreNanoseconds = 0;
	quint64 performanceSceneNanoseconds = 0;
	quint64 performanceDirtyTiles = 0;
	quint64 performanceDecodedTiles = 0;
	quint64 performanceUpdatedTiles = 0;
	quint64 performanceNametableSetPos = 0;
	QString currentRomPath;
	QString currentTestTarget;
	QStringList benchmarkRomPaths;
	QStringList benchmarkTargets;
	QVector<int> benchmarkDurationsMilliseconds;
	int benchmarkRomIndex = 0;
	QElapsedTimer benchmarkRomTimer;
};
#endif // MAINWINDOW_H
