#include "MainWindow.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QColor>
#include <QDesktopServices>
#include <QFileInfo>
#include <QFont>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QProgressBar>
#include <QPushButton>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

#include "app/DupeApp.h"
#include "ConfigDialog.h"
#include "HelpWindow.h"

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

MainWindow::MainWindow(DupeApp* controller, QWidget* parent)
    : QMainWindow(parent)
    , m_controller(controller)
{
    setWindowTitle(QStringLiteral("DupeCheck v1.0"));
    resize(1100, 700);

    buildMenuBar();
    buildToolBar();
    buildTree();
    buildStatusBar();
    buildContextMenu();

    // --- Controller → View connections ------------------------------------
    connect(m_controller, &DupeApp::scanStarted,   this, &MainWindow::onScanStarted);
    connect(m_controller, &DupeApp::scanProgress,  this, &MainWindow::onScanProgress);
    connect(m_controller, &DupeApp::scanFinished,  this, &MainWindow::onScanFinished);
    connect(m_controller, &DupeApp::scanError,     this, &MainWindow::onScanError);
    connect(m_controller, &DupeApp::statusMessage, this, &MainWindow::onStatusMessage);
    connect(m_controller, &DupeApp::trashCompleted,
            this, [this](const QStringList& paths) {
                // Mark trashed items with strikethrough + gray so the user
                // can see what was trashed and undo if needed.
                // The tree is fully rebuilt by scanFinished after an undo.
                for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
                    QTreeWidgetItem* group = m_tree->topLevelItem(i);
                    for (int j = 0; j < group->childCount(); ++j) {
                        QTreeWidgetItem* child = group->child(j);
                        if (!paths.contains(child->data(0, Qt::UserRole).toString()))
                            continue;
                        QFont f = child->font(0);
                        f.setStrikeOut(true);
                        for (int col = 0; col < child->columnCount(); ++col) {
                            child->setFont(col, f);
                            child->setForeground(col, QColor(Qt::gray));
                        }
                        child->setData(0, Qt::UserRole + 1, true); // mark trashed
                    }
                }
            });

    connect(m_tree, &QTreeWidget::itemSelectionChanged, this, [this] {
        const QString path = selectedPath();
        if (!path.isEmpty())
            m_statusLabel->setText(path);
    });
}

// ---------------------------------------------------------------------------
// UI building
// ---------------------------------------------------------------------------

void MainWindow::buildMenuBar()
{
    QMenu* fileMenu = menuBar()->addMenu(tr("File"));
    fileMenu->addAction(tr("Start Scan"), this, [this] { m_controller->initiateScan(); });
    fileMenu->addAction(tr("Settings…"), this, [this] {
        auto* dlg = new ConfigDialog(m_controller, this);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->exec();
    });
    fileMenu->addSeparator();
    fileMenu->addAction(tr("Quit DupeCheck"), qApp, &QApplication::quit,
                        QKeySequence::Quit);

    QMenu* editMenu = menuBar()->addMenu(tr("Edit"));
    editMenu->addAction(tr("Undo Last Trash"), this,
                        [this] { m_controller->undoLastTrash(); },
                        QKeySequence::Undo);
    editMenu->addSeparator();
    editMenu->addAction(tr("Copy Path"), this, &MainWindow::onCopyPath,
                        QKeySequence::Copy);

    QMenu* helpMenu = menuBar()->addMenu(tr("Help"));
    helpMenu->addAction(tr("DupeCheck Reference…"), this, [this] {
        auto* hw = new HelpWindow(this);
        hw->setAttribute(Qt::WA_DeleteOnClose);
        hw->show();
    });
}

void MainWindow::buildToolBar()
{
    QToolBar* tb = addToolBar(tr("Main"));
    tb->setMovable(false);

    m_scanAction  = tb->addAction(tr("▶  Start Scan"), m_controller, &DupeApp::initiateScan);
    m_trashAction = tb->addAction(tr("Move to Trash"), this, &MainWindow::onTrashRequested);
    m_undoAction  = tb->addAction(tr("↩  Undo"),       m_controller, &DupeApp::undoLastTrash);

    // Right-align the filter.
    QWidget* spacer = new QWidget;
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    tb->addWidget(spacer);

    tb->addWidget(new QLabel(tr("Filter: ")));
    m_filterEdit = new QLineEdit;
    m_filterEdit->setFixedWidth(200);
    m_filterEdit->setPlaceholderText(tr("filename or extension…"));
    tb->addWidget(m_filterEdit);

    tb->addAction(tr("⚙  Settings"), this, [this] {
        auto* dlg = new ConfigDialog(m_controller, this);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->exec();
    });

    connect(m_filterEdit, &QLineEdit::textChanged, this, &MainWindow::onFilterChanged);
}

void MainWindow::buildTree()
{
    m_tree = new QTreeWidget(this);
    m_tree->setColumnCount(2);
    m_tree->setHeaderLabels({tr("SHA-256 Cluster / Filename"), tr("Size (MB)")});
    m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    m_tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_tree->setRootIsDecorated(true);
    m_tree->setAlternatingRowColors(true);
    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_tree, &QTreeWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        if (m_tree->itemAt(pos))
            m_contextMenu->popup(m_tree->viewport()->mapToGlobal(pos));
    });

    setCentralWidget(m_tree);
}

void MainWindow::buildStatusBar()
{
    m_statusLabel  = new QLabel(tr("Ready"));
    m_progressBar  = new QProgressBar;
    m_progressBar->setRange(0, 0); // indeterminate by default
    m_progressBar->setVisible(false);
    m_progressBar->setFixedWidth(200);

    QPushButton* copyBtn = new QPushButton(tr("Copy Path"));
    connect(copyBtn, &QPushButton::clicked, this, &MainWindow::onCopyPath);

    statusBar()->addWidget(m_statusLabel, 1);
    statusBar()->addPermanentWidget(m_progressBar);
    statusBar()->addPermanentWidget(copyBtn);
}

void MainWindow::buildContextMenu()
{
    m_contextMenu = new QMenu(this);
    m_contextMenu->addAction(tr("Open File Location"), this, &MainWindow::onOpenLocation);
    m_contextMenu->addSeparator();
    m_contextMenu->addAction(tr("Keep Only This One"), this, &MainWindow::onKeepOnlyThis);
    m_contextMenu->addAction(tr("Move to Trash"),      this, &MainWindow::onTrashRequested);
}

// ---------------------------------------------------------------------------
// Controller → View slots
// ---------------------------------------------------------------------------

void MainWindow::onScanStarted()
{
    m_scanAction->setEnabled(false);
    m_progressBar->setVisible(true);
    m_statusLabel->setText(tr("Scan in progress…"));
}

void MainWindow::onScanProgress(int filesHashed, const QString& currentPath)
{
    m_statusLabel->setText(tr("Scanning %1 — %2 files processed")
                               .arg(currentPath)
                               .arg(filesHashed));
}

void MainWindow::onScanFinished(const QList<FileRecord>& duplicates)
{
    m_scanAction->setEnabled(true);
    m_progressBar->setVisible(false);
    m_allClusters = duplicates;
    renderTree(duplicates);
    m_statusLabel->setText(tr("Analysis complete. %1 duplicate records.")
                               .arg(duplicates.size()));
}

void MainWindow::onScanError(const QString& message)
{
    m_scanAction->setEnabled(true);
    m_progressBar->setVisible(false);
    m_statusLabel->setText(tr("Error: %1").arg(message));
}

void MainWindow::onStatusMessage(const QString& message)
{
    m_statusLabel->setText(message);
}

// ---------------------------------------------------------------------------
// View event slots
// ---------------------------------------------------------------------------

void MainWindow::onTrashRequested()
{
    QStringList paths;
    const QList<QTreeWidgetItem*> selected = m_tree->selectedItems();
    for (QTreeWidgetItem* item : selected) {
        if (item->parent() && !item->data(0, Qt::UserRole + 1).toBool()) {
            const QString p = item->data(0, Qt::UserRole).toString();
            if (!p.isEmpty())
                paths.append(p);
        }
    }
    if (!paths.isEmpty())
        m_controller->trashItems(paths);
}

void MainWindow::onFilterChanged(const QString& text)
{
    const QString term = text.trimmed().toLower();
    if (term.isEmpty()) {
        renderTree(m_allClusters);
        return;
    }
    QList<FileRecord> filtered;
    for (const FileRecord& rec : m_allClusters) {
        if (rec.path.toLower().contains(term))
            filtered.append(rec);
    }
    renderTree(filtered);
}

void MainWindow::onCopyPath()
{
    const QString path = selectedPath();
    if (path.isEmpty())
        return;
    QGuiApplication::clipboard()->setText(path);
    m_statusLabel->setText(tr("Copied: %1").arg(path));
}

void MainWindow::onOpenLocation()
{
    const QString path = selectedPath();
    if (!path.isEmpty())
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path).absolutePath()));
}

void MainWindow::onKeepOnlyThis()
{
    const QList<QTreeWidgetItem*> selected = m_tree->selectedItems();
    if (selected.isEmpty() || !selected.first()->parent())
        return;

    QTreeWidgetItem* keeper = selected.first();
    QTreeWidgetItem* group  = keeper->parent();
    QStringList toTrash;
    for (int i = 0; i < group->childCount(); ++i) {
        QTreeWidgetItem* sib = group->child(i);
        if (sib != keeper && !sib->data(0, Qt::UserRole + 1).toBool())
            toTrash.append(sib->data(0, Qt::UserRole).toString());
    }
    if (!toTrash.isEmpty())
        m_controller->trashItems(toTrash);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void MainWindow::renderTree(const QList<FileRecord>& records)
{
    m_tree->clear();

    // Group by hash.
    QMap<QString, QList<FileRecord>> groups;
    for (const FileRecord& rec : records)
        groups[rec.hash].append(rec);

    for (auto it = groups.cbegin(); it != groups.cend(); ++it) {
        const QList<FileRecord>& items = it.value();
        double totalMb = 0.0;
        for (const FileRecord& r : items)
            totalMb += static_cast<double>(r.size) / 1'048'576.0;

        auto* groupItem = new QTreeWidgetItem(m_tree);
        groupItem->setText(0, QStringLiteral("Group: %1…").arg(it.key().left(12)));
        groupItem->setText(1, QStringLiteral("%1").arg(totalMb, 0, 'f', 2));

        // Tag the newest file in the cluster.
        QString newestPath;
        QDateTime newestMtime;
        for (const FileRecord& r : items) {
            const QDateTime mtime = QFileInfo(r.path).lastModified();
            if (!newestMtime.isValid() || mtime > newestMtime) {
                newestMtime = mtime;
                newestPath  = r.path;
            }
        }

        for (const FileRecord& r : items) {
            double mb = static_cast<double>(r.size) / 1'048'576.0;
            auto* child = new QTreeWidgetItem(groupItem);
            const QString name = r.path == newestPath
                ? QFileInfo(r.path).fileName() + QStringLiteral("  (Newest)")
                : QFileInfo(r.path).fileName();
            child->setText(0, name);
            child->setText(1, QStringLiteral("%1").arg(mb, 0, 'f', 2));
            // Store full path in UserRole — not shown as a column.
            child->setData(0, Qt::UserRole, r.path);
            child->setToolTip(0, r.path);
        }
    }
}

QString MainWindow::selectedPath() const
{
    const QList<QTreeWidgetItem*> sel = m_tree->selectedItems();
    if (sel.isEmpty() || !sel.first()->parent())
        return {};
    return sel.first()->data(0, Qt::UserRole).toString();
}
