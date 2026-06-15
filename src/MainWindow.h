#pragma once

#include "GCodeGenerator.h"
#include "PresetManager.h"

#include <QMainWindow>

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;
class PreviewWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void applySelectedTool();
    void applySelectedMaterial();
    void refreshPreview();
    void exportGCode();
    void addToolPreset();
    void editToolPreset();
    void removeToolPreset();
    void addMaterialPreset();
    void editMaterialPreset();
    void removeMaterialPreset();
    void showAboutDialog();

private:
    void buildUi();
    void connectSignals();
    void reloadPresetCombos(int toolIndex = 0, int materialIndex = 0);
    void updateMaterialDetails();
    void refreshPreviewSilently();
    void updateSummary(const JobSummary &summary);
    void clearSummary();
    bool savePresets();
    [[nodiscard]] SurfacingJob currentJob() const;
    [[nodiscard]] bool editToolDialog(ToolPreset &preset, const QString &title);
    [[nodiscard]] bool editMaterialDialog(MaterialPreset &preset, const QString &title);
    [[nodiscard]] QString summaryText(const JobSummary &summary) const;
    [[nodiscard]] static QString presetFilePath();
    [[nodiscard]] static QString defaultOutputDirectory();

    PresetManager m_presetManager;
    GCodeGenerator m_generator;

    QComboBox *m_toolCombo = nullptr;
    QLabel *m_toolDetailsLabel = nullptr;
    QComboBox *m_materialCombo = nullptr;
    QLabel *m_materialDetailsLabel = nullptr;

    QDoubleSpinBox *m_lengthXSpin = nullptr;
    QDoubleSpinBox *m_widthYSpin = nullptr;
    QDoubleSpinBox *m_totalDepthSpin = nullptr;
    QDoubleSpinBox *m_depthPerPassSpin = nullptr;
    QDoubleSpinBox *m_marginSpin = nullptr;
    QDoubleSpinBox *m_safeZSpin = nullptr;
    QDoubleSpinBox *m_approachZSpin = nullptr;
    QComboBox *m_originCombo = nullptr;
    QComboBox *m_sweepCombo = nullptr;
    QLineEdit *m_outputNameEdit = nullptr;

    QLabel *m_passCountValue = nullptr;
    QLabel *m_scanLineCountValue = nullptr;
    QLabel *m_finalDepthValue = nullptr;
    QLabel *m_feedXYValue = nullptr;
    QLabel *m_plungeFeedValue = nullptr;
    QLabel *m_spindleValue = nullptr;
    QLabel *m_pathLengthValue = nullptr;

    PreviewWidget *m_preview = nullptr;
    QPushButton *m_previewButton = nullptr;
    QPushButton *m_exportButton = nullptr;
};
