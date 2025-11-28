#include "mainwindow.h"
#include <QShowEvent>
#include <QRegularExpression>
#include <QEvent>
#include <QKeyEvent>
#include <QNetworkRequest>
#include <QDir>
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QListWidgetItem>
#include <QTextStream>
#include <QUrl>
#include <QProcess>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , historyFile("english_word_history.txt")
    , networkManager(new QNetworkAccessManager(this))
    , ttsNetworkManager(new QNetworkAccessManager(this))
{
    setupUI();

    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::onNetworkReply);
    connect(ttsNetworkManager, &QNetworkAccessManager::finished, this, &MainWindow::onTtsReply);

    // Setup media player for audio playback
    mediaPlayer = new QMediaPlayer(this);

    // Qt 6 style
    #if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    audioOutput = new QAudioOutput(this);
    mediaPlayer->setAudioOutput(audioOutput);
    audioOutput->setVolume(0.7);

    // Connect Qt 6 signals
    connect(mediaPlayer, &QMediaPlayer::mediaStatusChanged, this, &MainWindow::onMediaStatusChanged);
    connect(mediaPlayer, &QMediaPlayer::errorOccurred, this, &MainWindow::onPlayerError);
    #else
    // Qt 5 style
    mediaPlayer->setVolume(70);

    // Connect Qt 5 signals
    connect(mediaPlayer, &QMediaPlayer::stateChanged, this, &MainWindow::onMediaStateChanged);
    connect(mediaPlayer, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), this, &MainWindow::onPlayerError);
    #endif

    // Create word_audio directory if it doesn't exist
    QDir audioDir("word_audio");
    if (!audioDir.exists()) {
        audioDir.mkpath(".");
    }

    loadHistory();
}

MainWindow::~MainWindow()
{
    // QObjects are automatically deleted
}

void MainWindow::setupUI()
{
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    setWindowTitle("English-English Dictionary with Pronunciation");
    setMinimumSize(900, 600);

    mainSplitter = new QSplitter(Qt::Horizontal, centralWidget);

    // Left panel - Lookup
    QWidget *leftPanel = new QWidget(mainSplitter);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);

    // Input layout with pronunciation button
    QHBoxLayout *inputLayout = new QHBoxLayout();
    wordInput = new QLineEdit(leftPanel);
    wordInput->setPlaceholderText("Enter English word and press Enter...");
    wordInput->setStyleSheet("QLineEdit { padding: 8px; font-size: 14px; }");

    pronounceButton = new QPushButton("🔊", leftPanel);
    pronounceButton->setToolTip("Play pronunciation");
    pronounceButton->setStyleSheet("QPushButton { padding: 8px; font-size: 16px; }");
    pronounceButton->setEnabled(false);

    inputLayout->addWidget(wordInput);
    inputLayout->addWidget(pronounceButton);

    // Audio playback checkbox
    autoPlayCheckbox = new QCheckBox("Auto-play pronunciation after lookup", leftPanel);

    // Progress bars
    lookupProgressBar = new QProgressBar(leftPanel);
    lookupProgressBar->setVisible(false);
    lookupProgressBar->setRange(0, 0);

    audioProgressBar = new QProgressBar(leftPanel);
    audioProgressBar->setVisible(false);
    audioProgressBar->setRange(0, 0);

    QLabel *lookupLabel = new QLabel("Lookup Result - English Dictionary", leftPanel);
    lookupLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 12px; padding: 5px; background-color: #e0e0e0; }");

    resultDisplay = new QTextEdit(leftPanel);
    resultDisplay->setReadOnly(true);
    resultDisplay->setStyleSheet("QTextEdit { background-color: #f5f5f5; padding: 10px; font-size: 12px; }");

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    lookupButton = new QPushButton("Lookup", leftPanel);
    copyButton = new QPushButton("Copy as Markdown", leftPanel);

    buttonLayout->addWidget(lookupButton);
    buttonLayout->addWidget(copyButton);
    buttonLayout->addStretch();

    leftLayout->addLayout(inputLayout);
    leftLayout->addWidget(autoPlayCheckbox);
    leftLayout->addWidget(lookupProgressBar);
    leftLayout->addWidget(audioProgressBar);
    leftLayout->addWidget(lookupLabel);
    leftLayout->addWidget(resultDisplay);
    leftLayout->addLayout(buttonLayout);

    // Right panel - History
    QWidget *rightPanel = new QWidget(mainSplitter);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);

    QLabel *historyLabel = new QLabel("Search History", rightPanel);
    historyLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 14px; padding: 5px; background-color: #e0e0e0; }");

    historyList = new QListWidget(rightPanel);
    historyList->setStyleSheet("QListWidget { font-size: 11px; }");

    QLabel *historyDetailLabel = new QLabel("History Detail", rightPanel);
    historyDetailLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 12px; padding: 5px; background-color: #e0e0e0; }");

    historyDetailDisplay = new QTextEdit(rightPanel);
    historyDetailDisplay->setReadOnly(true);
    historyDetailDisplay->setStyleSheet("QTextEdit { background-color: #f8f8f8; padding: 10px; font-size: 11px; border: 1px solid #ccc; }");

    copyHistoryButton = new QPushButton("Copy as Markdown", rightPanel);

    rightLayout->addWidget(historyLabel);
    rightLayout->addWidget(historyList);
    rightLayout->addWidget(historyDetailLabel);
    rightLayout->addWidget(historyDetailDisplay);
    rightLayout->addWidget(copyHistoryButton);

    mainSplitter->addWidget(leftPanel);
    mainSplitter->addWidget(rightPanel);
    mainSplitter->setSizes({600, 300});

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->addWidget(mainSplitter);

    statusLabel = new QLabel("Ready - Enter English word to lookup", centralWidget);
    statusLabel->setStyleSheet("QLabel { color: #666; font-size: 10px; padding: 5px; background-color: #f0f0f0; }");
    mainLayout->addWidget(statusLabel);

    // Connect signals and slots
    connect(wordInput, &QLineEdit::returnPressed, this, &MainWindow::onLookupWord);
    connect(lookupButton, &QPushButton::clicked, this, &MainWindow::onLookupWord);
    connect(pronounceButton, &QPushButton::clicked, this, &MainWindow::onPlayPronunciation);
    connect(copyButton, &QPushButton::clicked, this, &MainWindow::copyToClipboard);
    connect(copyHistoryButton, &QPushButton::clicked, this, &MainWindow::copyHistoryToClipboard);
    connect(historyList, &QListWidget::itemClicked, this, &MainWindow::onHistoryItemClicked);

    wordInput->setFocus();
}

void MainWindow::onLookupWord()
{
    QString word = wordInput->text().trimmed();
    if (word.isEmpty()) {
        resultDisplay->setText("Please enter an English word to lookup.");
        return;
    }

    currentWord = word;

    // Show lookup progress
    lookupProgressBar->setVisible(true);
    statusLabel->setText("Looking up English word: " + word);
    resultDisplay->setText("Searching dictionary API...");
    pronounceButton->setEnabled(false);
    currentAudioUrl.clear();

    // Use dictionaryapi.dev for English-English definitions
    QString url = QString("https://api.dictionaryapi.dev/api/v2/entries/en/%1").arg(word);
    networkManager->get(QNetworkRequest(QUrl(url)));
}

void MainWindow::onNetworkReply(QNetworkReply *reply)
{
    lookupProgressBar->setVisible(false);

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        parseDictionaryResponse(data);

        // Auto-play audio if checkbox is checked
        if (autoPlayCheckbox->isChecked() && !currentWord.isEmpty()) {
            downloadAndPlayAudio(currentWord, "en");
        }
    } else {
        resultDisplay->setText("Word not found or network error: " + reply->errorString());
        statusLabel->setText("Error");
        pronounceButton->setEnabled(false);
    }
    reply->deleteLater();
}

void MainWindow::parseDictionaryResponse(const QByteArray &data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray() || doc.array().isEmpty()) {
        resultDisplay->setText("Word not found in dictionary.");
        statusLabel->setText("Not found");
        pronounceButton->setEnabled(false);
        return;
    }

    QJsonObject firstEntry = doc.array().first().toObject();
    QString word = firstEntry["word"].toString();

    // Get phonetic information
    QString phoneticText = getPhoneticText(firstEntry);
    currentAudioUrl = getAudioUrl(firstEntry);

    // Format for display (HTML)
    QString result;
    result += QString("<h2 style='color: red;'>%1</h2>").arg(word);

    // Add pronunciation
//    if (!phoneticText.isEmpty()) {
//        result += QString("<p style='color: #666; font-size: 14px; margin-bottom: 10px;'>");
//        result += QString("<b>Pronunciation:</b> %1").arg(phoneticText);
//        result += " 🔊";
//        result += "</p>";
//    }

    QJsonArray meanings = firstEntry["meanings"].toArray();

    for (const QJsonValue &meaningValue : meanings) {
        QJsonObject meaning = meaningValue.toObject();
        QString partOfSpeech = meaning["partOfSpeech"].toString();

        QString posColor = "#2E86AB"; // Different color for each part of speech
        if (partOfSpeech == "noun") posColor = "#A23B72";
        else if (partOfSpeech == "verb") posColor = "#F18F01";
        else if (partOfSpeech == "adjective") posColor = "#C73E1D";
        else if (partOfSpeech == "adverb") posColor = "#3E8914";
        else if (partOfSpeech == "preposition") posColor = "#8B1E3F";

        result += QString("<h3 style='color: %1; background-color: #f0f0f0; padding: 5px;'>[%2]</h3>")
                     .arg(posColor, partOfSpeech.toUpper());

        QJsonArray definitions = meaning["definitions"].toArray();
        for (int i = 0; i < definitions.size() && i < 5; ++i) {
            QJsonObject definition = definitions[i].toObject();
            QString def = definition["definition"].toString();
            result += QString("<p><b>%1.</b> %2").arg(i + 1).arg(def);

            if (definition.contains("example")) {
                QString example = definition["example"].toString();
                result += QString("<br><i>Example: %1</i>").arg(example);
            }

            // Add synonyms if available
            if (definition.contains("synonyms")) {
                QJsonArray synonyms = definition["synonyms"].toArray();
                if (!synonyms.isEmpty()) {
                    QStringList synonymList;
                    for (const QJsonValue &synonym : synonyms) {
                        synonymList.append(synonym.toString());
                    }
                    result += QString("<br><span style='color: #666;'><b>Synonyms:</b> %1</span>")
                                 .arg(synonymList.join(", "));
                }
            }

            result += "</p>";
        }
    }

    // Generate markdown format
    currentMarkdown = formatMarkdown(firstEntry);
    currentDefinition = result;

    resultDisplay->setHtml(result);

    // Enable pronounce button
    pronounceButton->setEnabled(true);
    statusLabel->setText("Found - " + QDateTime::currentDateTime().toString("hh:mm:ss"));

    // Save to history
    saveWordToHistory(word, result);
    refreshHistoryList();

    // Auto-copy to clipboard
    copyToClipboard();
}

QString MainWindow::getPhoneticText(const QJsonObject &entry)
{
    // Try to get phonetic text
    if (entry.contains("phonetic")) {
        QString phonetic = entry["phonetic"].toString();
        if (!phonetic.isEmpty()) {
            return phonetic;
        }
    }

    // If no main phonetic, try to get from phonetics array
    if (entry.contains("phonetics")) {
        QJsonArray phonetics = entry["phonetics"].toArray();
        for (const QJsonValue &phoneticValue : phonetics) {
            QJsonObject phonetic = phoneticValue.toObject();
            if (phonetic.contains("text")) {
                QString text = phonetic["text"].toString();
                if (!text.isEmpty()) {
                    return text;
                }
            }
        }
    }

    return "";
}

QString MainWindow::getAudioUrl(const QJsonObject &entry)
{
    if (entry.contains("phonetics")) {
        QJsonArray phonetics = entry["phonetics"].toArray();
        for (const QJsonValue &phoneticValue : phonetics) {
            QJsonObject phonetic = phoneticValue.toObject();
            if (phonetic.contains("audio") && !phonetic["audio"].toString().isEmpty()) {
                QString audioUrl = phonetic["audio"].toString();
                if (audioUrl.startsWith("http")) {
                    return audioUrl;
                }
            }
        }
    }
    return "";
}

void MainWindow::onPlayPronunciation()
{
    if (!currentWord.isEmpty()) {
        downloadAndPlayAudio(currentWord, "en");
    } else {
        statusLabel->setText("No word to pronounce");
    }
}

void MainWindow::downloadAndPlayAudio(const QString &text, const QString &language)
{
    if (text.isEmpty()) return;

    // Check if audio file already exists locally
    QString safeWord = text;
    safeWord.replace(QRegularExpression("[^a-zA-Z0-9]"), "_");
    QString localAudioFile = QString("word_audio/%1_%2.mp3").arg(safeWord).arg(language);

    QFile file(localAudioFile);
    if (file.exists()) {
        // Play local audio file using Qt Multimedia
        playAudioFile(localAudioFile);
        return;
    }

    statusLabel->setText("Downloading audio pronunciation...");
    audioProgressBar->setVisible(true);

    // Encode text for URL
    QString encodedText = QUrl::toPercentEncoding(text);

    // Construct Google TTS URL
    QString url = QString("https://translate.google.com/translate_tts?ie=UTF-8&tl=%1&client=tw-ob&q=%2")
                     .arg(language).arg(encodedText);

    // Set headers to mimic a real browser
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36");
    request.setRawHeader("Referer", "https://translate.google.com/");

    // Download the audio
    ttsNetworkManager->get(request);
}

void MainWindow::onTtsReply(QNetworkReply *reply)
{
    audioProgressBar->setVisible(false);

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray audioData = reply->readAll();

        // Save to word_audio folder with filename based on the word
        QString safeWord = currentWord;
        safeWord.replace(QRegularExpression("[^a-zA-Z0-9]"), "_");
        QString localAudioFile = QString("word_audio/%1_en.mp3").arg(safeWord);

        QFile file(localAudioFile);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(audioData);
            file.close();

            // Play the audio using Qt Multimedia
            playAudioFile(localAudioFile);
        } else {
            statusLabel->setText("Error saving audio file");
        }
    } else {
        statusLabel->setText("Audio download failed: " + reply->errorString());
    }
    reply->deleteLater();
}

void MainWindow::playAudioFile(const QString &filePath)
{
    statusLabel->setText("Playing pronunciation...");

    #if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // Qt 6
    mediaPlayer->setSource(QUrl::fromLocalFile(filePath));
    #else
    // Qt 5
    mediaPlayer->setMedia(QUrl::fromLocalFile(filePath));
    #endif

    mediaPlayer->play();
}

// Qt 5 signal handlers
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
void MainWindow::onMediaStateChanged(QMediaPlayer::State state)
{
    switch (state) {
    case QMediaPlayer::PlayingState:
        statusLabel->setText("Playing pronunciation...");
        break;
    case QMediaPlayer::StoppedState:
        statusLabel->setText("Audio finished playing");
        break;
    case QMediaPlayer::PausedState:
        statusLabel->setText("Audio paused");
        break;
    }
}

void MainWindow::onPlayerError(QMediaPlayer::Error error)
{
    statusLabel->setText("Audio playback error: " + mediaPlayer->errorString());

    // Fallback to system playback if Qt Multimedia fails
    QString filePath = mediaPlayer->currentMedia().canonicalUrl().toLocalFile();
    if (!filePath.isEmpty()) {
        #ifdef Q_OS_WIN
        QString nativePath = QDir::toNativeSeparators(filePath);
        QString command = QString("cmd /c start \"\" \"%1\"").arg(nativePath);
        QProcess::startDetached(command);
        statusLabel->setText("Using system player as fallback...");
        #endif
    }
}
#endif

// Qt 6 signal handlers
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void MainWindow::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    switch (status) {
    case QMediaPlayer::LoadedMedia:
        statusLabel->setText("Audio loaded, playing...");
        break;
    case QMediaPlayer::EndOfMedia:
        statusLabel->setText("Audio finished playing");
        break;
    case QMediaPlayer::InvalidMedia:
        statusLabel->setText("Invalid audio file");
        break;
    default:
        break;
    }
}

void MainWindow::onPlayerError(QMediaPlayer::Error error, const QString &errorString)
{
    statusLabel->setText("Audio playback error: " + errorString);

    // Fallback to system playback if Qt Multimedia fails
    QString filePath = mediaPlayer->source().toLocalFile();
    if (!filePath.isEmpty()) {
        #ifdef Q_OS_WIN
        QString nativePath = QDir::toNativeSeparators(filePath);
        QString command = QString("cmd /c start \"\" \"%1\"").arg(nativePath);
        QProcess::startDetached(command);
        statusLabel->setText("Using system player as fallback...");
        #endif
    }
}
#endif

QString MainWindow::formatMarkdown(const QJsonObject &entry)
{
    QString markdown;
    QString word = entry["word"].toString();
    QString phoneticText = getPhoneticText(entry);

    // Word in red (using HTML color for markdown compatibility)
    markdown += QString("# <font color='red'>%1</font>\n\n").arg(word);

    // Add pronunciation
//    if (!phoneticText.isEmpty()) {
//        markdown += QString("**Pronunciation:** %1").arg(phoneticText);
//        markdown += " 🔊";
//        markdown += "\n\n";
//    }

    QJsonArray meanings = entry["meanings"].toArray();

    for (const QJsonValue &meaningValue : meanings) {
        QJsonObject meaning = meaningValue.toObject();
        QString partOfSpeech = meaning["partOfSpeech"].toString();

        // Part of speech in brackets
        markdown += QString("**[%1]**\n\n").arg(partOfSpeech.toUpper());

        QJsonArray definitions = meaning["definitions"].toArray();
        for (int i = 0; i < definitions.size() && i < 5; ++i) {
            QJsonObject definition = definitions[i].toObject();
            QString def = definition["definition"].toString();

            // Numbered definitions on new lines
            markdown += QString("%1. %2").arg(i + 1).arg(def);

            if (definition.contains("example")) {
                QString example = definition["example"].toString();
                markdown += QString("\n   *Example: %1*").arg(example);
            }

            // Add synonyms if available
            if (definition.contains("synonyms")) {
                QJsonArray synonyms = definition["synonyms"].toArray();
                if (!synonyms.isEmpty()) {
                    QStringList synonymList;
                    for (const QJsonValue &synonym : synonyms) {
                        synonymList.append(synonym.toString());
                    }
                    markdown += QString("\n   *Synonyms: %1*").arg(synonymList.join(", "));
                }
            }

            markdown += "\n\n";
        }
    }

    return markdown;
}

void MainWindow::copyToClipboard()
{
    if (!currentMarkdown.isEmpty()) {
        QApplication::clipboard()->setText(currentMarkdown);
        statusLabel->setText("Markdown copied to clipboard - " + QDateTime::currentDateTime().toString("hh:mm:ss"));
    }
}

void MainWindow::copyHistoryToClipboard()
{
    QString historyText = historyDetailDisplay->toPlainText();
    if (!historyText.isEmpty()) {
        QString markdown = "# History Lookup\n\n";

        QListWidgetItem *currentItem = historyList->currentItem();
        if (currentItem) {
            QString itemText = currentItem->text();
            QStringList parts = itemText.split(" - ");
            if (parts.size() >= 2) {
                QString wordPart = parts[1];
                QString word = wordPart.split(":")[0].trimmed();
                markdown += QString("## <font color='red'>%1</font>\n\n").arg(word);
            }
        }

        QString plainText = historyText;
        plainText.replace(QRegularExpression("\\[[A-Z]+\\]"), "**$0**"); // Bold the [NOUN], [VERB] etc.
        plainText.replace(QRegularExpression("^(\\d+\\.)"), "**$1**"); // Bold the numbers
        markdown += plainText;

        QApplication::clipboard()->setText(markdown);
        statusLabel->setText("History markdown copied to clipboard - " + QDateTime::currentDateTime().toString("hh:mm:ss"));
    }
}

void MainWindow::saveWordToHistory(const QString &word, const QString &definition)
{
    QFile file(historyFile);
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(&file);
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

        QString shortDefinition = definition;
        shortDefinition.remove(QRegularExpression("<[^>]*>"));
        shortDefinition.replace("&nbsp;", " ");
        shortDefinition = shortDefinition.left(100);

        stream << timestamp << "|" << word << "|" << definition << "|" << shortDefinition << "\n";
        file.close();
    }
}

void MainWindow::loadHistory()
{
    refreshHistoryList();
}

void MainWindow::refreshHistoryList()
{
    historyList->clear();
    historyDetailDisplay->clear();

    QFile file(historyFile);
    if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        QStringList lines;

        while (!stream.atEnd()) {
            lines.prepend(stream.readLine());
        }

        for (const QString &line : lines) {
            QStringList parts = line.split("|");
            if (parts.size() >= 4) {
                QString timestamp = parts[0];
                QString word = parts[1];
                QString fullDefinition = parts[2];
                QString shortDefinition = parts[3];

                QString displayText = QString("%1 - %2: %3").arg(timestamp, word, shortDefinition);

                QListWidgetItem *item = new QListWidgetItem(displayText);
                item->setData(Qt::UserRole, word);
                item->setData(Qt::UserRole + 1, fullDefinition);
                historyList->addItem(item);
            }
        }
        file.close();
    }
}

void MainWindow::onHistoryItemClicked(QListWidgetItem *item)
{
    if (!item) return;

    QString word = item->data(Qt::UserRole).toString();
    QString fullDefinition = item->data(Qt::UserRole + 1).toString();

    qDebug() << fullDefinition;
    historyDetailDisplay->setHtml(fullDefinition);

    statusLabel->setText("History displayed - " + word);

    // Play audio if checkbox is checked
    if (autoPlayCheckbox->isChecked() && !word.isEmpty()) {
        playAudioForWord(word);
    }
}

void MainWindow::playAudioForWord(const QString &word)
{
    // Check if audio file exists locally in word_audio folder
    QString safeWord = word;
    safeWord.replace(QRegularExpression("[^a-zA-Z0-9]"), "_");
    QString localAudioFile = QString("word_audio/%1_en.mp3").arg(safeWord);

    QFile file(localAudioFile);
    if (file.exists()) {
        // Play local audio file
        playAudioFile(localAudioFile);
        statusLabel->setText("Playing pronunciation for: " + word);
    } else {
        // Download and play audio using TTS
        currentWord = word; // Set current word for the download
        downloadAndPlayAudio(word, "en");
    }
}

bool MainWindow::event(QEvent *event)
{
    if (event->type() == QEvent::WindowActivate) {
        wordInput->setFocus();
        wordInput->selectAll();
        return true;
    }
    else if (event->type() == QEvent::WindowDeactivate) {
        wordInput->clear();
        return true;
    }

    return QMainWindow::event(event);
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    wordInput->setFocus();
    wordInput->selectAll();
}
