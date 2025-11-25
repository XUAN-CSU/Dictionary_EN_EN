#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QClipboard>
#include <QApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QLabel>
#include <QSplitter>
#include <QListWidget>
#include <QListWidgetItem>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void showEvent(QShowEvent *event) override;
    bool event(QEvent *event) override;  // Add this for window activation events

private slots:
    void onLookupWord();
    void onNetworkReply(QNetworkReply *reply);
    void copyToClipboard();
    void copyHistoryToClipboard();
    void onHistoryItemClicked(QListWidgetItem *item);

private:
    void setupUI();
    void parseDictionaryResponse(const QByteArray &data);
    QString formatMarkdown(const QJsonObject &entry);
    void saveWordToHistory(const QString &word, const QString &definition);
    void loadHistory();
    void refreshHistoryList();

    QLineEdit *wordInput;
    QTextEdit *resultDisplay;
    QTextEdit *historyDetailDisplay;
    QPushButton *lookupButton;
    QPushButton *copyButton;
    QPushButton *copyHistoryButton;
    QNetworkAccessManager *networkManager;
    QLabel *statusLabel;
    QSplitter *mainSplitter;
    QListWidget *historyList;

    QString currentDefinition;
    QString currentMarkdown;
    QString historyFile;
};

#endif // MAINWINDOW_H
