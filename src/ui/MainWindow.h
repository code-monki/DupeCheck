#pragma once

#include <QMainWindow>
#include <QList>
#include <QString>

#include "types.h"

class DupeApp;
class QAction;
class QLabel;
class QLineEdit;
class QProgressBar;
class QTreeWidget;
class QTreeWidgetItem;

// ---------------------------------------------------------------------------
// MainWindow — View (MVC).
//
// Owns no business logic. All user events are forwarded to DupeApp via
// direct method calls or signals; DupeApp drives all updates back through
// slots connected in the constructor.
// ---------------------------------------------------------------------------
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(DupeApp* controller, QWidget* parent = nullptr);

private slots:
    // --- Controller → View ------------------------------------------------
    void onScanStarted();
    void onScanProgress(int filesHashed, const QString& currentPath);
    void onScanFinished(const QList<FileRecord>& duplicates);
    void onScanError(const QString& message);
    void onStatusMessage(const QString& message);

    // --- View events ------------------------------------------------------
    void onTrashRequested();
    void onFilterChanged(const QString& text);
    void onCopyPath();
    void onOpenLocation();
    void onKeepOnlyThis();

private:
    void buildMenuBar();
    void buildToolBar();
    void buildTree();
    void buildStatusBar();
    void buildContextMenu();

    void renderTree(const QList<FileRecord>& records);
    QString selectedPath() const;

    DupeApp*      m_controller;

    // Toolbar widgets
    QAction*      m_scanAction   = nullptr;
    QAction*      m_trashAction  = nullptr;
    QAction*      m_undoAction   = nullptr;

    // Central tree
    QTreeWidget*  m_tree         = nullptr;

    // Filter
    QLineEdit*    m_filterEdit   = nullptr;

    // Status bar
    QLabel*       m_statusLabel  = nullptr;
    QProgressBar* m_progressBar  = nullptr;

    // Context menu
    QMenu*        m_contextMenu  = nullptr;

    // Cache for client-side filtering (no DB query on keystroke).
    QList<FileRecord> m_allClusters;
};
