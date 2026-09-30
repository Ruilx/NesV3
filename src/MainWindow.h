#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QElapsedTimer>
#include <QFile>
#include <QTimer>
#include <QString>
#include <QTextStream>

#include "Nes.h"
#include "Nes/AudioRingBuffer.h"

class AudioOutputDevice;
class NesScene;
class NesView;
class QAudioSink;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = nullptr);
	~MainWindow() override;

	[[nodiscard]] bool loadRomFromPath(const QString &path, QString &error);
	[[nodiscard]] bool startAutomatedRun(
		int durationMilliseconds,
		const QString &screenshotDirectory,
		int screenshotIntervalMilliseconds,
		bool screenshotOnExit,
		QString &error);

private:
	void savePalette();
	void openPaletteEditor();
	void openPatternTableDialog();
	void openRom();
	void closeRom();
	void openPerformanceLog(const QString &romPath);
	void closePerformanceLog();
	void resetPerformanceWindow();
	void finishAutomatedRun();
	void initializeAudioOutput();
	void startAudioOutput();
	void startAudioSinkIfReady();
	void scheduleSimulationWakeup();
	void stopAudioOutput();
	[[nodiscard]] bool saveViewportScreenshot();
	void runSimulationFrame();
	void updateWindowTitle();
	void setDebugInputMode(bool enabled);

	Nes nes;
	NesScene *scene = nullptr;
	NesView *view = nullptr;
	QTimer simulationTimer;
	QElapsedTimer performanceTimer;
	QElapsedTimer frameTimer;
	QElapsedTimer automatedRunTimer;
	QTimer automatedRunStopTimer;
	QTimer screenshotTimer;
	QFile performanceLogFile;
	QTextStream performanceLogStream;
	QElapsedTimer simulationPacingTimer;
	AudioRingBuffer audioRingBuffer;
	AudioOutputDevice *audioOutputDevice = nullptr;
	QAudioSink *audioSink = nullptr;
	bool audioOutputStarted = false;
	quint64 simulationFrameNumber = 0;
	quint64 performanceFrameCount = 0;
	quint64 performanceCoreNanoseconds = 0;
	quint64 performanceSceneNanoseconds = 0;
	quint64 performanceDirtyTiles = 0;
	quint64 performanceDecodedTiles = 0;
	quint64 performanceUpdatedTiles = 0;
	quint64 performanceNametableSetPos = 0;
	QString currentRomPath;
	QString screenshotDirectory;
	int screenshotSequence = 0;
	bool screenshotOnExit = false;
	bool automatedRunActive = false;
};
#endif // MAINWINDOW_H
