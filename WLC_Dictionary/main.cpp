#include "mainwindow.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

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
