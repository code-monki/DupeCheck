#pragma once

#include <QDialog>

// ---------------------------------------------------------------------------
// HelpWindow — read-only reference panel.
// ---------------------------------------------------------------------------
class HelpWindow : public QDialog
{
    Q_OBJECT
public:
    explicit HelpWindow(QWidget* parent = nullptr);
};
