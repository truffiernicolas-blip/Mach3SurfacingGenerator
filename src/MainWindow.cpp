#include "MainWindow.h"

#include "PreviewWidget.h"

#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QPixmap>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QStandardPaths>
#include <QTextStream>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>

namespace {

void configureMillimeterSpin(QDoubleSpinBox *spinBox,
                             double minimum,
                             double maximum,
                             double value)
{
    spinBox->setRange(minimum, maximum);
    spinBox->setDecimals(3);
    spinBox->setSingleStep(0.1);
    spinBox->setSuffix(QStringLiteral(" mm"));
    spinBox->setValue(value);
}

QPushButton *compactButton(const QString &text, QWidget *parent)
{
    auto *button = new QPushButton(text, parent);
    button->setFixedWidth(32);
    return button;
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_presetManager(presetFilePath())
{
    buildUi();

    QString presetError;
    if (!m_presetManager.load(&presetError)) {
        QMessageBox::critical(this, tr("Erreur de presets"), presetError);
    }

    reloadPresetCombos();
    connectSignals();
    applySelectedTool();
    applySelectedMaterial();
    refreshPreviewSilently();
}

void MainWindow::buildUi()
{
    setWindowTitle(tr("Générateur de surfaçage Mach3"));
    setWindowIcon(QIcon(QStringLiteral(":/branding/logo.png")));
    resize(1220, 760);
    setMinimumSize(980, 650);

    auto *centralWidget = new QWidget(this);
    auto *centralLayout = new QHBoxLayout(centralWidget);
    centralLayout->setContentsMargins(10, 10, 10, 10);

    auto *splitter = new QSplitter(Qt::Horizontal, centralWidget);
    centralLayout->addWidget(splitter);

    auto *formContainer = new QWidget(splitter);
    formContainer->setMinimumWidth(390);
    formContainer->setMaximumWidth(520);
    auto *formLayout = new QVBoxLayout(formContainer);
    formLayout->setContentsMargins(0, 0, 8, 0);

    auto *presetGroup = new QGroupBox(tr("Presets"), formContainer);
    auto *presetLayout = new QGridLayout(presetGroup);

    m_toolCombo = new QComboBox(presetGroup);
    auto *addToolButton = compactButton(QStringLiteral("+"), presetGroup);
    auto *editToolButton = compactButton(QStringLiteral("…"), presetGroup);
    auto *removeToolButton = compactButton(QStringLiteral("−"), presetGroup);
    presetLayout->addWidget(new QLabel(tr("Outil"), presetGroup), 0, 0);
    presetLayout->addWidget(m_toolCombo, 0, 1);
    presetLayout->addWidget(addToolButton, 0, 2);
    presetLayout->addWidget(editToolButton, 0, 3);
    presetLayout->addWidget(removeToolButton, 0, 4);

    m_toolDetailsLabel = new QLabel(presetGroup);
    m_toolDetailsLabel->setWordWrap(true);
    m_toolDetailsLabel->setStyleSheet(QStringLiteral("color: #666;"));
    presetLayout->addWidget(m_toolDetailsLabel, 1, 1, 1, 4);

    m_materialCombo = new QComboBox(presetGroup);
    auto *addMaterialButton = compactButton(QStringLiteral("+"), presetGroup);
    auto *editMaterialButton = compactButton(QStringLiteral("…"), presetGroup);
    auto *removeMaterialButton = compactButton(QStringLiteral("−"), presetGroup);
    presetLayout->addWidget(new QLabel(tr("Matière"), presetGroup), 2, 0);
    presetLayout->addWidget(m_materialCombo, 2, 1);
    presetLayout->addWidget(addMaterialButton, 2, 2);
    presetLayout->addWidget(editMaterialButton, 2, 3);
    presetLayout->addWidget(removeMaterialButton, 2, 4);

    m_materialDetailsLabel = new QLabel(presetGroup);
    m_materialDetailsLabel->setWordWrap(true);
    m_materialDetailsLabel->setStyleSheet(QStringLiteral("color: #666;"));
    presetLayout->addWidget(m_materialDetailsLabel, 3, 1, 1, 4);
    presetLayout->setColumnStretch(1, 1);
    formLayout->addWidget(presetGroup);

    auto *jobGroup = new QGroupBox(tr("Opération de surfaçage"), formContainer);
    auto *jobForm = new QFormLayout(jobGroup);

    m_lengthXSpin = new QDoubleSpinBox(jobGroup);
    configureMillimeterSpin(m_lengthXSpin, 0.001, 100000.0, 100.0);
    jobForm->addRow(tr("Longueur X"), m_lengthXSpin);

    m_widthYSpin = new QDoubleSpinBox(jobGroup);
    configureMillimeterSpin(m_widthYSpin, 0.001, 100000.0, 50.0);
    jobForm->addRow(tr("Largeur Y"), m_widthYSpin);

    m_totalDepthSpin = new QDoubleSpinBox(jobGroup);
    configureMillimeterSpin(m_totalDepthSpin, 0.001, 1000.0, 1.0);
    jobForm->addRow(tr("Profondeur totale"), m_totalDepthSpin);

    m_depthPerPassSpin = new QDoubleSpinBox(jobGroup);
    configureMillimeterSpin(m_depthPerPassSpin, 0.001, 1000.0, 0.5);
    jobForm->addRow(tr("Profondeur par passe"), m_depthPerPassSpin);

    m_marginSpin = new QDoubleSpinBox(jobGroup);
    configureMillimeterSpin(m_marginSpin, 0.0, 10000.0, 0.0);
    jobForm->addRow(tr("Marge extérieure"), m_marginSpin);

    m_safeZSpin = new QDoubleSpinBox(jobGroup);
    configureMillimeterSpin(m_safeZSpin, -1000.0, 10000.0, 5.0);
    jobForm->addRow(tr("Z sécurité"), m_safeZSpin);

    m_approachZSpin = new QDoubleSpinBox(jobGroup);
    configureMillimeterSpin(m_approachZSpin, 0.0, 10000.0, 1.0);
    jobForm->addRow(tr("Z approche"), m_approachZSpin);

    m_originCombo = new QComboBox(jobGroup);
    m_originCombo->addItem(tr("Coin bas gauche"), static_cast<int>(JobOrigin::BottomLeft));
    m_originCombo->addItem(tr("Coin bas droit"), static_cast<int>(JobOrigin::BottomRight));
    jobForm->addRow(tr("Origine"), m_originCombo);

    m_sweepCombo = new QComboBox(jobGroup);
    m_sweepCombo->addItem(tr("Balayage suivant X"),
                          static_cast<int>(SweepDirection::AlongX));
    m_sweepCombo->addItem(tr("Balayage suivant Y"),
                          static_cast<int>(SweepDirection::AlongY));
    jobForm->addRow(tr("Sens de balayage"), m_sweepCombo);

    m_outputNameEdit = new QLineEdit(QStringLiteral("surfacage.nc"), jobGroup);
    jobForm->addRow(tr("Fichier de sortie"), m_outputNameEdit);
    formLayout->addWidget(jobGroup);

    auto *summaryGroup = new QGroupBox(tr("Résumé"), formContainer);
    auto *summaryLayout = new QFormLayout(summaryGroup);
    m_passCountValue = new QLabel(QStringLiteral("—"), summaryGroup);
    m_scanLineCountValue = new QLabel(QStringLiteral("—"), summaryGroup);
    m_finalDepthValue = new QLabel(QStringLiteral("—"), summaryGroup);
    m_feedXYValue = new QLabel(QStringLiteral("—"), summaryGroup);
    m_plungeFeedValue = new QLabel(QStringLiteral("—"), summaryGroup);
    m_spindleValue = new QLabel(QStringLiteral("—"), summaryGroup);
    m_pathLengthValue = new QLabel(QStringLiteral("—"), summaryGroup);
    summaryLayout->addRow(tr("Passes Z"), m_passCountValue);
    summaryLayout->addRow(tr("Lignes de balayage"), m_scanLineCountValue);
    summaryLayout->addRow(tr("Profondeur finale"), m_finalDepthValue);
    summaryLayout->addRow(tr("Avance XY"), m_feedXYValue);
    summaryLayout->addRow(tr("Avance Z"), m_plungeFeedValue);
    summaryLayout->addRow(tr("Vitesse broche"), m_spindleValue);
    summaryLayout->addRow(tr("Parcours estimé"), m_pathLengthValue);
    formLayout->addWidget(summaryGroup);

    auto *actionLayout = new QHBoxLayout;
    m_previewButton = new QPushButton(tr("Actualiser"), formContainer);
    m_exportButton = new QPushButton(tr("Exporter le G-code…"), formContainer);
    m_exportButton->setDefault(true);
    actionLayout->addWidget(m_previewButton);
    actionLayout->addWidget(m_exportButton, 1);
    formLayout->addLayout(actionLayout);
    formLayout->addStretch();

    auto *scrollArea = new QScrollArea(splitter);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidget(formContainer);

    auto *previewContainer = new QWidget(splitter);
    auto *previewLayout = new QVBoxLayout(previewContainer);
    previewLayout->setContentsMargins(8, 0, 0, 0);
    auto *previewTitle = new QLabel(tr("Prévisualisation du parcours XY"), previewContainer);
    QFont titleFont = previewTitle->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 1);
    previewTitle->setFont(titleFont);
    previewLayout->addWidget(previewTitle);

    auto *legend = new QLabel(
        tr("Bleu : zone à surfacer  •  Orange : centre outil  •  Rouge : origine"),
        previewContainer);
    legend->setStyleSheet(QStringLiteral("color: #666;"));
    previewLayout->addWidget(legend);

    m_preview = new PreviewWidget(previewContainer);
    previewLayout->addWidget(m_preview, 1);

    splitter->addWidget(scrollArea);
    splitter->addWidget(previewContainer);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({430, 780});

    setCentralWidget(centralWidget);

    auto *helpMenu = menuBar()->addMenu(tr("&Aide"));
    auto *aboutAction = helpMenu->addAction(tr("À &propos…"));
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAboutDialog);

    statusBar()->showMessage(tr("Prêt"));

    connect(addToolButton, &QPushButton::clicked, this, &MainWindow::addToolPreset);
    connect(editToolButton, &QPushButton::clicked, this, &MainWindow::editToolPreset);
    connect(removeToolButton, &QPushButton::clicked, this, &MainWindow::removeToolPreset);
    connect(addMaterialButton, &QPushButton::clicked,
            this, &MainWindow::addMaterialPreset);
    connect(editMaterialButton, &QPushButton::clicked,
            this, &MainWindow::editMaterialPreset);
    connect(removeMaterialButton, &QPushButton::clicked,
            this, &MainWindow::removeMaterialPreset);
}

void MainWindow::connectSignals()
{
    connect(m_toolCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &MainWindow::applySelectedTool);
    connect(m_materialCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &MainWindow::applySelectedMaterial);
    connect(m_previewButton, &QPushButton::clicked, this, &MainWindow::refreshPreview);
    connect(m_exportButton, &QPushButton::clicked, this, &MainWindow::exportGCode);

    const auto connectSpin = [this](QDoubleSpinBox *spinBox) {
        connect(spinBox, qOverload<double>(&QDoubleSpinBox::valueChanged),
                this, &MainWindow::refreshPreviewSilently);
    };
    connectSpin(m_lengthXSpin);
    connectSpin(m_widthYSpin);
    connectSpin(m_totalDepthSpin);
    connectSpin(m_depthPerPassSpin);
    connectSpin(m_marginSpin);
    connectSpin(m_safeZSpin);
    connectSpin(m_approachZSpin);

    connect(m_originCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &MainWindow::refreshPreviewSilently);
    connect(m_sweepCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &MainWindow::refreshPreviewSilently);
    connect(m_outputNameEdit, &QLineEdit::textChanged,
            this, &MainWindow::refreshPreviewSilently);
}

void MainWindow::reloadPresetCombos(int toolIndex, int materialIndex)
{
    const QSignalBlocker toolBlocker(m_toolCombo);
    const QSignalBlocker materialBlocker(m_materialCombo);

    m_toolCombo->clear();
    for (const ToolPreset &preset : m_presetManager.tools()) {
        m_toolCombo->addItem(preset.name);
    }

    m_materialCombo->clear();
    for (const MaterialPreset &preset : m_presetManager.materials()) {
        m_materialCombo->addItem(preset.name);
    }

    if (m_toolCombo->count() > 0) {
        m_toolCombo->setCurrentIndex(std::clamp(toolIndex, 0, m_toolCombo->count() - 1));
    }
    if (m_materialCombo->count() > 0) {
        m_materialCombo->setCurrentIndex(
            std::clamp(materialIndex, 0, m_materialCombo->count() - 1));
    }
}

void MainWindow::applySelectedTool()
{
    const int index = m_toolCombo->currentIndex();
    if (index < 0 || index >= m_presetManager.tools().size()) {
        return;
    }

    const ToolPreset &tool = m_presetManager.tools().at(index);
    m_safeZSpin->setValue(tool.recommendedSafeZ);
    m_toolDetailsLabel->setText(
        tr("T%1  •  Ø %2 mm  •  %3 dents  •  Recouvrement %4 %  •  Stepover %5 mm")
            .arg(tool.toolNumber)
            .arg(tool.diameter, 0, 'f', 2)
            .arg(tool.fluteCount)
            .arg(tool.overlapPercent, 0, 'f', 1)
            .arg(tool.stepover(), 0, 'f', 2));
    updateMaterialDetails();
    refreshPreviewSilently();
}

void MainWindow::applySelectedMaterial()
{
    const int index = m_materialCombo->currentIndex();
    if (index < 0 || index >= m_presetManager.materials().size()) {
        return;
    }

    const MaterialPreset &material = m_presetManager.materials().at(index);
    m_depthPerPassSpin->setValue(material.recommendedDepthPerPass);
    updateMaterialDetails();
    refreshPreviewSilently();
}

void MainWindow::updateMaterialDetails()
{
    const int toolIndex = m_toolCombo->currentIndex();
    const int materialIndex = m_materialCombo->currentIndex();
    if (toolIndex < 0 || toolIndex >= m_presetManager.tools().size()
        || materialIndex < 0 || materialIndex >= m_presetManager.materials().size()) {
        m_materialDetailsLabel->clear();
        return;
    }

    const ToolPreset &tool = m_presetManager.tools().at(toolIndex);
    const MaterialPreset &material = m_presetManager.materials().at(materialIndex);
    const double cuttingSpeed =
        3.14159265358979323846 * tool.diameter * material.spindleSpeed / 1000.0;
    const double feedPerTooth =
        material.feedXY / (material.spindleSpeed * tool.fluteCount);

    m_materialDetailsLabel->setText(
        tr("S %1 tr/min  •  Vc %2 m/min  •  F XY %3  •  F Z %4 mm/min  •  fz %5 mm")
            .arg(material.spindleSpeed)
            .arg(cuttingSpeed, 0, 'f', 2)
            .arg(material.feedXY, 0, 'f', 1)
            .arg(material.plungeFeed, 0, 'f', 1)
            .arg(feedPerTooth, 0, 'f', 5));
}

SurfacingJob MainWindow::currentJob() const
{
    SurfacingJob job;
    job.lengthX = m_lengthXSpin->value();
    job.widthY = m_widthYSpin->value();
    job.totalDepth = m_totalDepthSpin->value();
    job.depthPerPass = m_depthPerPassSpin->value();
    job.outsideMargin = m_marginSpin->value();
    job.safeZ = m_safeZSpin->value();
    job.approachZ = m_approachZSpin->value();
    job.origin = static_cast<JobOrigin>(m_originCombo->currentData().toInt());
    job.sweepDirection =
        static_cast<SweepDirection>(m_sweepCombo->currentData().toInt());
    job.outputFileName = m_outputNameEdit->text().trimmed();

    const int toolIndex = m_toolCombo->currentIndex();
    if (toolIndex >= 0 && toolIndex < m_presetManager.tools().size()) {
        job.tool = m_presetManager.tools().at(toolIndex);
    }

    const int materialIndex = m_materialCombo->currentIndex();
    if (materialIndex >= 0 && materialIndex < m_presetManager.materials().size()) {
        job.material = m_presetManager.materials().at(materialIndex);
    }

    return job;
}

void MainWindow::refreshPreview()
{
    const GenerationResult result = m_generator.generate(currentJob());
    if (!result.success) {
        m_preview->clearToolpath();
        clearSummary();
        QMessageBox::warning(this, tr("Paramètres invalides"), result.error);
        return;
    }

    m_preview->setToolpath(result.geometry, currentJob().tool.diameter);
    updateSummary(result.summary);
    statusBar()->showMessage(tr("Prévisualisation actualisée"), 2500);
}

void MainWindow::refreshPreviewSilently()
{
    const SurfacingJob job = currentJob();
    const GenerationResult result = m_generator.generate(job);
    if (!result.success) {
        m_preview->clearToolpath();
        clearSummary();
        statusBar()->showMessage(result.error.section(QLatin1Char('\n'), 0, 0));
        return;
    }

    m_preview->setToolpath(result.geometry, job.tool.diameter);
    updateSummary(result.summary);
    statusBar()->showMessage(tr("Paramètres valides"));
}

void MainWindow::exportGCode()
{
    const SurfacingJob job = currentJob();
    const GenerationResult result = m_generator.generate(job);
    if (!result.success) {
        QMessageBox::warning(this, tr("Paramètres invalides"), result.error);
        return;
    }

    const QString confirmation =
        summaryText(result.summary)
        + tr("\n\nLe fichier doit être vérifié dans Mach3 avant usinage.\n"
             "Continuer l'export ?");
    if (QMessageBox::question(this, tr("Résumé avant export"), confirmation,
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No)
        != QMessageBox::Yes) {
        return;
    }

    const QString outputDirectory = defaultOutputDirectory();
    QDir().mkpath(outputDirectory);
    const QString suggestedPath = QDir(outputDirectory).filePath(job.outputFileName);
    QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("Exporter le programme Mach3"),
        suggestedPath,
        tr("Programme CNC (*.nc *.tap);;Tous les fichiers (*.*)"));
    if (filePath.isEmpty()) {
        return;
    }
    if (QFileInfo(filePath).suffix().isEmpty()) {
        filePath += QStringLiteral(".nc");
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QMessageBox::critical(
            this,
            tr("Erreur d'export"),
            tr("Impossible d'écrire le fichier :\n%1").arg(file.errorString()));
        return;
    }

    QTextStream stream(&file);
    stream << result.gcode;
    file.close();

    statusBar()->showMessage(tr("Fichier exporté : %1").arg(filePath), 5000);
    QMessageBox::information(
        this,
        tr("Export terminé"),
        tr("Le programme Mach3 a été enregistré dans :\n%1").arg(filePath));
}

void MainWindow::showAboutDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("À propos de Mach3 Surfacing Generator"));
    dialog.setWindowIcon(QIcon(QStringLiteral(":/branding/logo.png")));
    dialog.setMinimumWidth(460);

    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(24, 22, 24, 18);
    layout->setSpacing(12);

    auto *logoLabel = new QLabel(&dialog);
    logoLabel->setAlignment(Qt::AlignCenter);
    logoLabel->setPixmap(
        QPixmap(QStringLiteral(":/branding/logo.png"))
            .scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    layout->addWidget(logoLabel);

    auto *titleLabel = new QLabel(tr("<b>Mach3 Surfacing Generator</b>"), &dialog);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(titleFont.pointSize() + 3);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);

    auto *detailsLabel = new QLabel(
        tr("Version %1<br>"
           "Développé par %2<br><br>"
           "<b>Logiciel open source sous licence MIT</b><br>"
           "Copyright © 2026 Nicolas Truffier<br><br>"
           "Générateur de parcours de surfaçage rectangulaire pour Mach3.")
            .arg(QCoreApplication::applicationVersion(),
                 QStringLiteral(MACH3_APP_AUTHOR)),
        &dialog);
    detailsLabel->setAlignment(Qt::AlignCenter);
    detailsLabel->setWordWrap(true);
    layout->addWidget(detailsLabel);

    auto *noticeLabel = new QLabel(
        tr("Mach3 est une marque de son propriétaire respectif. "
           "Ce projet indépendant n'est ni affilié ni approuvé par celui-ci."),
        &dialog);
    noticeLabel->setAlignment(Qt::AlignCenter);
    noticeLabel->setWordWrap(true);
    noticeLabel->setStyleSheet(QStringLiteral("color: #666;"));
    layout->addWidget(noticeLabel);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    layout->addWidget(buttons);

    dialog.exec();
}

QString MainWindow::presetFilePath()
{
    const QString directory =
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return QDir(directory).filePath(QStringLiteral("presets.json"));
}

QString MainWindow::defaultOutputDirectory()
{
    QString documents =
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (documents.isEmpty()) {
        documents = QDir::homePath();
    }
    return QDir(documents).filePath(QStringLiteral("Mach3SurfacingGenerator"));
}

void MainWindow::updateSummary(const JobSummary &summary)
{
    m_passCountValue->setText(QString::number(summary.depthPassCount));
    m_scanLineCountValue->setText(QString::number(summary.scanLineCount));
    m_finalDepthValue->setText(tr("%1 mm").arg(summary.finalDepth, 0, 'f', 3));
    m_feedXYValue->setText(tr("%1 mm/min").arg(summary.feedXY, 0, 'f', 0));
    m_plungeFeedValue->setText(tr("%1 mm/min").arg(summary.plungeFeed, 0, 'f', 0));
    m_spindleValue->setText(tr("%1 tr/min").arg(summary.spindleSpeed));
    m_pathLengthValue->setText(tr("%1 mm").arg(summary.estimatedPathLength, 0, 'f', 1));
}

void MainWindow::clearSummary()
{
    for (QLabel *label : {m_passCountValue,
                          m_scanLineCountValue,
                          m_finalDepthValue,
                          m_feedXYValue,
                          m_plungeFeedValue,
                          m_spindleValue,
                          m_pathLengthValue}) {
        label->setText(QStringLiteral("—"));
    }
}

QString MainWindow::summaryText(const JobSummary &summary) const
{
    return tr("Passes Z : %1\n"
              "Lignes de balayage : %2\n"
              "Profondeur finale : %3 mm\n"
              "Avance XY : %4 mm/min\n"
              "Avance Z : %5 mm/min\n"
              "Vitesse broche : %6 tr/min\n"
              "Longueur estimée : %7 mm")
        .arg(summary.depthPassCount)
        .arg(summary.scanLineCount)
        .arg(summary.finalDepth, 0, 'f', 3)
        .arg(summary.feedXY, 0, 'f', 0)
        .arg(summary.plungeFeed, 0, 'f', 0)
        .arg(summary.spindleSpeed)
        .arg(summary.estimatedPathLength, 0, 'f', 1);
}

bool MainWindow::savePresets()
{
    QString error;
    if (!m_presetManager.save(&error)) {
        QMessageBox::critical(this, tr("Erreur de presets"), error);
        return false;
    }
    return true;
}

bool MainWindow::editToolDialog(ToolPreset &preset, const QString &title)
{
    QDialog dialog(this);
    dialog.setWindowTitle(title);
    auto *layout = new QFormLayout(&dialog);

    auto *nameEdit = new QLineEdit(preset.name, &dialog);
    auto *toolNumberSpin = new QSpinBox(&dialog);
    toolNumberSpin->setRange(1, 999);
    toolNumberSpin->setPrefix(QStringLiteral("T"));
    toolNumberSpin->setValue(preset.toolNumber);
    auto *diameterSpin = new QDoubleSpinBox(&dialog);
    configureMillimeterSpin(diameterSpin, 0.001, 1000.0, preset.diameter);
    auto *fluteCountSpin = new QSpinBox(&dialog);
    fluteCountSpin->setRange(1, 20);
    fluteCountSpin->setSuffix(tr(" dents"));
    fluteCountSpin->setValue(preset.fluteCount);
    auto *overlapSpin = new QDoubleSpinBox(&dialog);
    overlapSpin->setRange(0.0, 99.9);
    overlapSpin->setDecimals(1);
    overlapSpin->setSuffix(QStringLiteral(" %"));
    overlapSpin->setValue(preset.overlapPercent);
    auto *safeZSpin = new QDoubleSpinBox(&dialog);
    configureMillimeterSpin(safeZSpin, 0.001, 10000.0, preset.recommendedSafeZ);
    auto *stepoverLabel = new QLabel(&dialog);

    const auto updateStepover = [=] {
        const double stepover =
            diameterSpin->value() * (1.0 - overlapSpin->value() / 100.0);
        stepoverLabel->setText(tr("%1 mm").arg(stepover, 0, 'f', 3));
    };
    connect(diameterSpin, qOverload<double>(&QDoubleSpinBox::valueChanged),
            &dialog, updateStepover);
    connect(overlapSpin, qOverload<double>(&QDoubleSpinBox::valueChanged),
            &dialog, updateStepover);
    updateStepover();

    layout->addRow(tr("Nom"), nameEdit);
    layout->addRow(tr("Numéro d'outil"), toolNumberSpin);
    layout->addRow(tr("Diamètre"), diameterSpin);
    layout->addRow(tr("Nombre de dents"), fluteCountSpin);
    layout->addRow(tr("Recouvrement"), overlapSpin);
    layout->addRow(tr("Stepover calculé"), stepoverLabel);
    layout->addRow(tr("Z sécurité conseillé"), safeZSpin);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addRow(buttons);

    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }
    if (nameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Preset invalide"), tr("Le nom est obligatoire."));
        return false;
    }

    preset.name = nameEdit->text().trimmed();
    preset.toolNumber = toolNumberSpin->value();
    preset.diameter = diameterSpin->value();
    preset.fluteCount = fluteCountSpin->value();
    preset.overlapPercent = overlapSpin->value();
    preset.recommendedSafeZ = safeZSpin->value();
    return true;
}

bool MainWindow::editMaterialDialog(MaterialPreset &preset, const QString &title)
{
    QDialog dialog(this);
    dialog.setWindowTitle(title);
    auto *layout = new QFormLayout(&dialog);

    auto *nameEdit = new QLineEdit(preset.name, &dialog);
    auto *spindleSpin = new QSpinBox(&dialog);
    spindleSpin->setRange(1, 100000);
    spindleSpin->setSuffix(QStringLiteral(" tr/min"));
    spindleSpin->setValue(preset.spindleSpeed);
    auto *feedXYSpin = new QDoubleSpinBox(&dialog);
    feedXYSpin->setRange(0.1, 100000.0);
    feedXYSpin->setDecimals(1);
    feedXYSpin->setSuffix(QStringLiteral(" mm/min"));
    feedXYSpin->setValue(preset.feedXY);
    auto *plungeSpin = new QDoubleSpinBox(&dialog);
    plungeSpin->setRange(0.1, 100000.0);
    plungeSpin->setDecimals(1);
    plungeSpin->setSuffix(QStringLiteral(" mm/min"));
    plungeSpin->setValue(preset.plungeFeed);
    auto *depthSpin = new QDoubleSpinBox(&dialog);
    configureMillimeterSpin(depthSpin, 0.001, 1000.0, preset.recommendedDepthPerPass);

    layout->addRow(tr("Nom"), nameEdit);
    layout->addRow(tr("Vitesse broche"), spindleSpin);
    layout->addRow(tr("Avance XY"), feedXYSpin);
    layout->addRow(tr("Avance plongée Z"), plungeSpin);
    layout->addRow(tr("Profondeur par passe"), depthSpin);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addRow(buttons);

    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }
    if (nameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Preset invalide"), tr("Le nom est obligatoire."));
        return false;
    }

    preset.name = nameEdit->text().trimmed();
    preset.spindleSpeed = spindleSpin->value();
    preset.feedXY = feedXYSpin->value();
    preset.plungeFeed = plungeSpin->value();
    preset.recommendedDepthPerPass = depthSpin->value();
    return true;
}

void MainWindow::addToolPreset()
{
    ToolPreset preset;
    preset.name = tr("Nouvel outil");
    if (!editToolDialog(preset, tr("Ajouter un outil"))) {
        return;
    }

    m_presetManager.addTool(preset);
    if (!savePresets()) {
        return;
    }
    reloadPresetCombos(m_presetManager.tools().size() - 1, m_materialCombo->currentIndex());
    applySelectedTool();
}

void MainWindow::editToolPreset()
{
    const int index = m_toolCombo->currentIndex();
    if (index < 0 || index >= m_presetManager.tools().size()) {
        return;
    }

    ToolPreset preset = m_presetManager.tools().at(index);
    if (!editToolDialog(preset, tr("Modifier l'outil"))) {
        return;
    }

    m_presetManager.updateTool(index, preset);
    if (!savePresets()) {
        return;
    }
    reloadPresetCombos(index, m_materialCombo->currentIndex());
    applySelectedTool();
}

void MainWindow::removeToolPreset()
{
    const int index = m_toolCombo->currentIndex();
    if (index < 0 || index >= m_presetManager.tools().size()) {
        return;
    }
    if (m_presetManager.tools().size() <= 1) {
        QMessageBox::warning(this, tr("Suppression impossible"),
                             tr("Au moins un preset outil doit être conservé."));
        return;
    }
    if (QMessageBox::question(
            this,
            tr("Supprimer l'outil"),
            tr("Supprimer le preset « %1 » ?").arg(m_presetManager.tools().at(index).name))
        != QMessageBox::Yes) {
        return;
    }

    m_presetManager.removeTool(index);
    if (!savePresets()) {
        return;
    }
    reloadPresetCombos(std::min(index, static_cast<int>(m_presetManager.tools().size()) - 1),
                       m_materialCombo->currentIndex());
    applySelectedTool();
}

void MainWindow::addMaterialPreset()
{
    MaterialPreset preset;
    preset.name = tr("Nouvelle matière");
    if (!editMaterialDialog(preset, tr("Ajouter une matière"))) {
        return;
    }

    m_presetManager.addMaterial(preset);
    if (!savePresets()) {
        return;
    }
    reloadPresetCombos(m_toolCombo->currentIndex(),
                       m_presetManager.materials().size() - 1);
    applySelectedMaterial();
}

void MainWindow::editMaterialPreset()
{
    const int index = m_materialCombo->currentIndex();
    if (index < 0 || index >= m_presetManager.materials().size()) {
        return;
    }

    MaterialPreset preset = m_presetManager.materials().at(index);
    if (!editMaterialDialog(preset, tr("Modifier la matière"))) {
        return;
    }

    m_presetManager.updateMaterial(index, preset);
    if (!savePresets()) {
        return;
    }
    reloadPresetCombos(m_toolCombo->currentIndex(), index);
    applySelectedMaterial();
}

void MainWindow::removeMaterialPreset()
{
    const int index = m_materialCombo->currentIndex();
    if (index < 0 || index >= m_presetManager.materials().size()) {
        return;
    }
    if (m_presetManager.materials().size() <= 1) {
        QMessageBox::warning(this, tr("Suppression impossible"),
                             tr("Au moins un preset matière doit être conservé."));
        return;
    }
    if (QMessageBox::question(
            this,
            tr("Supprimer la matière"),
            tr("Supprimer le preset « %1 » ?")
                .arg(m_presetManager.materials().at(index).name))
        != QMessageBox::Yes) {
        return;
    }

    m_presetManager.removeMaterial(index);
    if (!savePresets()) {
        return;
    }
    reloadPresetCombos(m_toolCombo->currentIndex(),
                       std::min(index,
                                static_cast<int>(m_presetManager.materials().size()) - 1));
    applySelectedMaterial();
}
