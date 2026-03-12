#pragma once

#include <QDialog>
#include <QSet>
#include <QStringList>

class DupeApp;
class QListWidget;
class QLineEdit;

// ---------------------------------------------------------------------------
// ConfigDialog — modal settings panel.
// Reads state from DupeApp on open, writes back on Save.
// ---------------------------------------------------------------------------
class ConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConfigDialog(DupeApp* controller, QWidget* parent = nullptr);

private slots:
    void onAddPath();
    void onRemovePath();
    void onClearCache();
    void onSave();

private:
    void buildUi();

    DupeApp*     m_controller;
    QListWidget* m_pathList  = nullptr;
    QLineEdit*   m_extEdit   = nullptr;
};
