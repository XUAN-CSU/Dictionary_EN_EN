#include "mainwindow.h"
#include <QShowEvent>
#include <QRegularExpression>
#include <QEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , historyFile("word_history.txt")
{
    setupUI();
    networkManager = new QNetworkAccessManager(this);
    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::onNetworkReply);

    loadHistory();
}

MainWindow::~MainWindow()
{
}

bool MainWindow::event(QEvent *event)
{
    if (event->type() == QEvent::WindowActivate) {
        // Window activated (Alt+Tab to this app)
        wordInput->setFocus();
        wordInput->selectAll();
        return true;
    }
    else if (event->type() == QEvent::WindowDeactivate) {
        // Window deactivated (Alt+Tab away from this app)
        wordInput->clear();
        return true;
    }

    return QMainWindow::event(event);
}

void MainWindow::setupUI()
{
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // Set window properties
    setWindowTitle("Dictionary Lookup");
    setMinimumSize(900, 600);

    // Create main splitter (left-right)
    mainSplitter = new QSplitter(Qt::Horizontal, this);

    // Left panel - Lookup
    QWidget *leftPanel = new QWidget;
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);

    wordInput = new QLineEdit(this);
    wordInput->setPlaceholderText("Enter English word and press Enter...");
    wordInput->setStyleSheet("QLineEdit { padding: 8px; font-size: 14px; }");

    QLabel *lookupLabel = new QLabel("Lookup Result", this);
    lookupLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 12px; padding: 5px; background-color: #e0e0e0; }");

    resultDisplay = new QTextEdit(this);
    resultDisplay->setReadOnly(true);
    resultDisplay->setStyleSheet("QTextEdit { background-color: #f5f5f5; padding: 10px; font-size: 12px; }");

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    lookupButton = new QPushButton("Lookup", this);
    copyButton = new QPushButton("Copy as Markdown", this);

    buttonLayout->addWidget(lookupButton);
    buttonLayout->addWidget(copyButton);
    buttonLayout->addStretch();

    leftLayout->addWidget(wordInput);
    leftLayout->addWidget(lookupLabel);
    leftLayout->addWidget(resultDisplay);
    leftLayout->addLayout(buttonLayout);

    // Right panel - History
    QWidget *rightPanel = new QWidget;
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);

    QLabel *historyLabel = new QLabel("Search History", this);
    historyLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 14px; padding: 5px; background-color: #e0e0e0; }");

    historyList = new QListWidget(this);
    historyList->setStyleSheet("QListWidget { font-size: 11px; }");

    QLabel *historyDetailLabel = new QLabel("History Detail", this);
    historyDetailLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 12px; padding: 5px; background-color: #e0e0e0; }");

    historyDetailDisplay = new QTextEdit(this);
    historyDetailDisplay->setReadOnly(true);
    historyDetailDisplay->setStyleSheet("QTextEdit { background-color: #f8f8f8; padding: 10px; font-size: 11px; border: 1px solid #ccc; }");

    copyHistoryButton = new QPushButton("Copy as Markdown", this);

    rightLayout->addWidget(historyLabel);
    rightLayout->addWidget(historyList);
    rightLayout->addWidget(historyDetailLabel);
    rightLayout->addWidget(historyDetailDisplay);
    rightLayout->addWidget(copyHistoryButton);

    // Add panels to main splitter
    mainSplitter->addWidget(leftPanel);
    mainSplitter->addWidget(rightPanel);
    mainSplitter->setSizes({600, 300});

    // Main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->addWidget(mainSplitter);

    statusLabel = new QLabel("Ready", this);
    statusLabel->setStyleSheet("QLabel { color: #666; font-size: 10px; padding: 5px; background-color: #f0f0f0; }");
    mainLayout->addWidget(statusLabel);

    // Connect signals and slots
    connect(wordInput, &QLineEdit::returnPressed, this, &MainWindow::onLookupWord);
    connect(lookupButton, &QPushButton::clicked, this, &MainWindow::onLookupWord);
    connect(copyButton, &QPushButton::clicked, this, &MainWindow::copyToClipboard);
    connect(copyHistoryButton, &QPushButton::clicked, this, &MainWindow::copyHistoryToClipboard);
    connect(historyList, &QListWidget::itemClicked, this, &MainWindow::onHistoryItemClicked);

    // Set focus to input field
    wordInput->setFocus();
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    wordInput->setFocus();
    wordInput->selectAll();
}

void MainWindow::onLookupWord()
{
    QString word = wordInput->text().trimmed();
    if (word.isEmpty()) {
        resultDisplay->setText("Please enter a word to lookup.");
        return;
    }

    statusLabel->setText("Looking up...");
    resultDisplay->setText("Searching...");

    // Make API request
    QString url = QString("https://api.dictionaryapi.dev/api/v2/entries/en/%1").arg(word);
    networkManager->get(QNetworkRequest(QUrl(url)));
}

void MainWindow::onNetworkReply(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        parseDictionaryResponse(data);
    } else {
        resultDisplay->setText("Word not found or network error: " + reply->errorString());
        statusLabel->setText("Error");
    }
    reply->deleteLater();
}

void MainWindow::parseDictionaryResponse(const QByteArray &data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray() || doc.array().isEmpty()) {
        resultDisplay->setText("Word not found in dictionary.");
        statusLabel->setText("Not found");
        return;
    }

    QJsonObject firstEntry = doc.array().first().toObject();
    QString word = firstEntry["word"].toString();

    // Format for display (HTML)
    QString result;
    result += QString("<h2 style='color: red;'>%1</h2>").arg(word);

    QJsonArray meanings = firstEntry["meanings"].toArray();

    for (const QJsonValue &meaningValue : meanings) {
        QJsonObject meaning = meaningValue.toObject();
        QString partOfSpeech = meaning["partOfSpeech"].toString();

        QString posColor = "#2E86AB"; // Different color for each part of speech
        if (partOfSpeech == "noun") posColor = "#A23B72";
        else if (partOfSpeech == "verb") posColor = "#F18F01";
        else if (partOfSpeech == "adjective") posColor = "#C73E1D";

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
            result += "</p>";
        }
    }

    // Generate markdown format
    currentMarkdown = formatMarkdown(firstEntry);
    currentDefinition = result;

    resultDisplay->setHtml(result);
    statusLabel->setText("Found - " + QDateTime::currentDateTime().toString("hh:mm:ss"));

    // Save to history
    saveWordToHistory(word, result);
    refreshHistoryList();

    // Auto-copy to clipboard
    copyToClipboard();
}

QString MainWindow::formatMarkdown(const QJsonObject &entry)
{
    QString markdown;
    QString word = entry["word"].toString();

    // Word in red (using HTML color for markdown compatibility)
    markdown += QString("# <font color='red'>%1</font>\n\n").arg(word);

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
        // Convert history display to markdown format
        QString markdown = "# History Lookup\n\n";

        // Get the selected history item text for the word
        QListWidgetItem *currentItem = historyList->currentItem();
        if (currentItem) {
            QString itemText = currentItem->text();
            // Extract word from history item (format: "timestamp - word: definition...")
            QStringList parts = itemText.split(" - ");
            if (parts.size() >= 2) {
                QString wordPart = parts[1];
                QString word = wordPart.split(":")[0].trimmed();
                markdown += QString("## <font color='red'>%1</font>\n\n").arg(word);
            }
        }

        // Add the definition content
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

        // Create a modified copy for the short description instead of modifying the original
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
            lines.prepend(stream.readLine()); // Reverse order (newest first)
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
                item->setData(Qt::UserRole + 1, fullDefinition); // Store full HTML definition
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

    // Display the full definition in history detail area (don't change lookup result)
    historyDetailDisplay->setHtml(fullDefinition);

    // Update status
    statusLabel->setText("History displayed - " + word);
}
