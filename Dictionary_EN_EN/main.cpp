#include "mainwindow.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Try multiple methods to set icon
    QIcon appIcon;

    // Method 1: From resource
    if (QFile::exists(":/icons/app_icon.ico")) {
        appIcon = QIcon(":/icons/app_icon.ico");
    }
    // Method 2: From external file
    else if (QFile::exists(":images/app_icon.ico")) {
        appIcon = QIcon(":images/app_icon.ico");
    }
    // Method 3: From PNG
    else if (QFile::exists(":images/app_icon.png")) {
        appIcon = QIcon(":images/app_icon.png");
    }
    // Method 4: Use built-in Qt icon as fallback
    else {
        appIcon = QIcon::fromTheme("help-contents");
        if (appIcon.isNull()) {
            appIcon = QApplication::style()->standardIcon(QStyle::SP_FileIcon);
        }
    }

    app.setWindowIcon(appIcon);

    // Set application properties
    app.setApplicationName("Dictionary Lookup");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("YourCompany");

    // Set modern style
    app.setStyle(QStyleFactory::create("Fusion"));

    // Create and show main window
    MainWindow window;
    window.show();

    return app.exec();
}

/*
┌─────────────────────────────────────────┬────────────────────────┐
│              LEFT PANEL                 │     RIGHT PANEL       │
│                                         │                        │
│  [Word Input]                          │  ┌──────────────────┐  │
│                                         │  │  HISTORY LIST    │  │
│  Lookup Result:                        │  │  - word1 10:30   │  │
│  # <red>word</red>                     │  │  - word2 10:25   │  │
│  [NOUN]                                │  │  - word3 10:20   │  │
│  1. definition...                      │  └──────────────────┘  │
│                                         │                        │
│  [Buttons: Lookup, Copy Markdown]      │  ┌──────────────────┐  │
│                                         │  │ HISTORY DETAIL   │  │
│                                         │  │ Full definition  │  │
│                                         │  │ of selected word │  │
│                                         │  │ from history     │  │
│                                         │  └──────────────────┘  │
│                                         │                        │
│                                         │  [Clear History Button]│
└─────────────────────────────────────────┴────────────────────────┘
*/
