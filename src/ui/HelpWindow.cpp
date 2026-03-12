#include "HelpWindow.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QVBoxLayout>

HelpWindow::HelpWindow(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("DupeCheck v1.0 Reference"));
    resize(520, 440);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(10);
    layout->setContentsMargins(20, 20, 20, 15);

    layout->addWidget(new QLabel(
        QStringLiteral("<b>Management Console Reference</b>")));

    auto* text = new QPlainTextEdit;
    text->setReadOnly(true);
    text->setFont(QFont(QStringLiteral("Menlo"), 10));
    text->setPlainText(QStringLiteral(
        "OPERATIONS\n"
        "----------\n"
        "START SCAN : Recursively walks all configured target paths.\n"
        "             Hidden paths (any component beginning with '.') and\n"
        "             zero-byte files are silently skipped.\n\n"
        "TRASH      : Moves the selected file(s) to the system Trash.\n"
        "             No immediate deletion is performed.\n\n"
        "UNDO       : Reverts the last database metadata change.\n"
        "             Physical files must be restored manually from Trash.\n\n"
        "HASHING\n"
        "-------\n"
        "Full Hash  : SHA-256 of the entire file (1 MB read chunks).\n"
        "Smart Hash : SHA-256 of the first 1 MB + last 1 MB, prefixed\n"
        "             'smart-'. Triggered by extension (see Settings).\n"
        "             Falls back to full hash if file ≤ 2 MB.\n\n"
        "SHORTCUTS\n"
        "---------\n"
        "Cmd+Z      : Undo last trash\n"
        "Cmd+,      : Open Settings\n"
        "Cmd+C      : Copy selected full path to clipboard\n\n"
        "FILTER\n"
        "------\n"
        "Real-time substring match against the full path. No database\n"
        "query is issued — filtering is purely client-side."
    ));

    layout->addWidget(text, 1);

    auto* btns = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::accept);
    layout->addWidget(btns);
}
