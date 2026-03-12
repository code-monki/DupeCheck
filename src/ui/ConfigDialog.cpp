#include "ConfigDialog.h"

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "app/DupeApp.h"

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

ConfigDialog::ConfigDialog(DupeApp* controller, QWidget* parent)
    : QDialog(parent)
    , m_controller(controller)
{
    setWindowTitle(tr("Scan Configuration"));
    setMinimumSize(500, 560);
    buildUi();
}

void ConfigDialog::buildUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(12);
    layout->setContentsMargins(15, 15, 15, 15);

    // --- Scan Paths -------------------------------------------------------
    auto* pathsGroup = new QGroupBox(tr("Target Scan Paths (Drives / Folders)"));
    auto* pathsLayout = new QVBoxLayout(pathsGroup);

    m_pathList = new QListWidget;
    for (const QString& p : m_controller->scanPaths())
        m_pathList->addItem(p);
    pathsLayout->addWidget(m_pathList);

    auto* pathBtns = new QHBoxLayout;
    auto* addBtn   = new QPushButton(tr("+ Add Path"));
    auto* removeBtn = new QPushButton(tr("– Remove"));
    pathBtns->addWidget(addBtn);
    pathBtns->addWidget(removeBtn);
    pathBtns->addStretch();
    pathsLayout->addLayout(pathBtns);

    layout->addWidget(pathsGroup);

    connect(addBtn,    &QPushButton::clicked, this, &ConfigDialog::onAddPath);
    connect(removeBtn, &QPushButton::clicked, this, &ConfigDialog::onRemovePath);

    // --- Smart Check Extensions ------------------------------------------
    auto* extGroup  = new QGroupBox(tr("Smart Check Extensions (Partial Hashing)"));
    auto* extLayout = new QVBoxLayout(extGroup);

    extLayout->addWidget(new QLabel(
        tr("Comma-separated list (e.g. mkv, mp4, iso). "
           "Only the first 1 MB and last 1 MB are hashed for matching extensions.")));

    m_extEdit = new QLineEdit;
    QStringList extList;
    for (const QString& e : m_controller->smartExtensions())
        extList.append(e.mid(1)); // strip leading dot
    extList.sort();
    m_extEdit->setText(extList.join(QStringLiteral(", ")));
    extLayout->addWidget(m_extEdit);

    layout->addWidget(extGroup);

    // --- Database Maintenance --------------------------------------------
    auto* dbGroup  = new QGroupBox(tr("Database Maintenance"));
    auto* dbLayout = new QHBoxLayout(dbGroup);

    auto* clearBtn = new QPushButton(tr("Clear Scan Cache (Wipe Database)"));
    dbLayout->addWidget(clearBtn);
    dbLayout->addStretch();

    layout->addWidget(dbGroup);
    connect(clearBtn, &QPushButton::clicked, this, &ConfigDialog::onClearCache);

    // --- Dialog Buttons --------------------------------------------------
    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &ConfigDialog::onSave);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void ConfigDialog::onAddPath()
{
    const QString dir = QFileDialog::getExistingDirectory(
        this, tr("Select Scan Directory"), QDir::homePath());
    if (!dir.isEmpty())
        m_pathList->addItem(dir);
}

void ConfigDialog::onRemovePath()
{
    const QList<QListWidgetItem*> selected = m_pathList->selectedItems();
    for (QListWidgetItem* item : selected)
        delete item;
}

void ConfigDialog::onClearCache()
{
    const int ret = QMessageBox::question(
        this,
        tr("Confirm Clear"),
        tr("Wipe the duplicate cache? A full re-hash will be required on the next scan."),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if (ret == QMessageBox::Yes) {
        m_controller->clearCache();
        QMessageBox::information(this, tr("DupeCheck"), tr("Database cleared."));
    }
}

void ConfigDialog::onSave()
{
    // Collect paths from the list widget.
    QStringList paths;
    for (int i = 0; i < m_pathList->count(); ++i)
        paths.append(m_pathList->item(i)->text());
    m_controller->setScanPaths(paths);

    // Parse and normalise the extensions string.
    QSet<QString> exts;
    const QStringList raw = m_extEdit->text().split(QLatin1Char(','));
    for (const QString& e : raw) {
        const QString trimmed = e.trimmed().toLower();
        if (!trimmed.isEmpty())
            exts.insert(QLatin1Char('.') + trimmed);
    }
    m_controller->setSmartExtensions(exts);

    accept();
}
