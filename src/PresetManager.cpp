#include "PresetManager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <utility>

PresetManager::PresetManager(QString filePath)
    : m_filePath(std::move(filePath))
{
}

bool PresetManager::load(QString *error)
{
    QFile file(m_filePath);
    if (!file.exists()) {
        loadBuiltInDefaults();
        return save(error);
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error) {
            *error = QStringLiteral("Impossible d'ouvrir %1 : %2")
                         .arg(m_filePath, file.errorString());
        }
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error) {
            *error = QStringLiteral("JSON invalide dans %1 : %2")
                         .arg(m_filePath, parseError.errorString());
        }
        return false;
    }

    QVector<ToolPreset> tools;
    const QJsonArray toolArray = document.object().value(QStringLiteral("tools")).toArray();
    for (const QJsonValue &value : toolArray) {
        const QJsonObject object = value.toObject();
        ToolPreset preset;
        preset.name = object.value(QStringLiteral("name")).toString();
        preset.toolNumber = object.value(QStringLiteral("toolNumber")).toInt(1);
        preset.diameter = object.value(QStringLiteral("diameter")).toDouble();
        preset.fluteCount = object.value(QStringLiteral("fluteCount")).toInt(2);
        preset.overlapPercent = object.value(QStringLiteral("overlapPercent")).toDouble();
        preset.recommendedSafeZ = object.value(QStringLiteral("recommendedSafeZ")).toDouble();
        if (!preset.name.isEmpty()) {
            tools << preset;
        }
    }

    QVector<MaterialPreset> materials;
    const QJsonArray materialArray =
        document.object().value(QStringLiteral("materials")).toArray();
    for (const QJsonValue &value : materialArray) {
        const QJsonObject object = value.toObject();
        MaterialPreset preset;
        preset.name = object.value(QStringLiteral("name")).toString();
        preset.spindleSpeed = object.value(QStringLiteral("spindleSpeed")).toInt();
        preset.feedXY = object.value(QStringLiteral("feedXY")).toDouble();
        preset.plungeFeed = object.value(QStringLiteral("plungeFeed")).toDouble();
        preset.recommendedDepthPerPass =
            object.value(QStringLiteral("recommendedDepthPerPass")).toDouble();
        if (!preset.name.isEmpty()) {
            materials << preset;
        }
    }

    if (tools.isEmpty() || materials.isEmpty()) {
        if (error) {
            *error = QStringLiteral(
                "Le fichier de presets doit contenir au moins un outil et une matière.");
        }
        return false;
    }

    m_tools = std::move(tools);
    m_materials = std::move(materials);
    return true;
}

bool PresetManager::save(QString *error) const
{
    const QFileInfo fileInfo(m_filePath);
    if (!QDir().mkpath(fileInfo.absolutePath())) {
        if (error) {
            *error = QStringLiteral("Impossible de créer le dossier %1.")
                         .arg(fileInfo.absolutePath());
        }
        return false;
    }

    QJsonArray toolArray;
    for (const ToolPreset &preset : m_tools) {
        QJsonObject object;
        object.insert(QStringLiteral("name"), preset.name);
        object.insert(QStringLiteral("toolNumber"), preset.toolNumber);
        object.insert(QStringLiteral("diameter"), preset.diameter);
        object.insert(QStringLiteral("fluteCount"), preset.fluteCount);
        object.insert(QStringLiteral("overlapPercent"), preset.overlapPercent);
        object.insert(QStringLiteral("recommendedSafeZ"), preset.recommendedSafeZ);
        toolArray.append(object);
    }

    QJsonArray materialArray;
    for (const MaterialPreset &preset : m_materials) {
        QJsonObject object;
        object.insert(QStringLiteral("name"), preset.name);
        object.insert(QStringLiteral("spindleSpeed"), preset.spindleSpeed);
        object.insert(QStringLiteral("feedXY"), preset.feedXY);
        object.insert(QStringLiteral("plungeFeed"), preset.plungeFeed);
        object.insert(QStringLiteral("recommendedDepthPerPass"),
                      preset.recommendedDepthPerPass);
        materialArray.append(object);
    }

    QJsonObject root;
    root.insert(QStringLiteral("tools"), toolArray);
    root.insert(QStringLiteral("materials"), materialArray);

    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        if (error) {
            *error = QStringLiteral("Impossible d'écrire %1 : %2")
                         .arg(m_filePath, file.errorString());
        }
        return false;
    }

    if (file.write(QJsonDocument(root).toJson(QJsonDocument::Indented)) < 0) {
        if (error) {
            *error = QStringLiteral("Erreur pendant l'écriture de %1 : %2")
                         .arg(m_filePath, file.errorString());
        }
        return false;
    }

    return true;
}

const QVector<ToolPreset> &PresetManager::tools() const
{
    return m_tools;
}

const QVector<MaterialPreset> &PresetManager::materials() const
{
    return m_materials;
}

QString PresetManager::filePath() const
{
    return m_filePath;
}

void PresetManager::addTool(const ToolPreset &preset)
{
    m_tools << preset;
}

void PresetManager::updateTool(int index, const ToolPreset &preset)
{
    if (index >= 0 && index < m_tools.size()) {
        m_tools[index] = preset;
    }
}

void PresetManager::removeTool(int index)
{
    if (index >= 0 && index < m_tools.size()) {
        m_tools.removeAt(index);
    }
}

void PresetManager::addMaterial(const MaterialPreset &preset)
{
    m_materials << preset;
}

void PresetManager::updateMaterial(int index, const MaterialPreset &preset)
{
    if (index >= 0 && index < m_materials.size()) {
        m_materials[index] = preset;
    }
}

void PresetManager::removeMaterial(int index)
{
    if (index >= 0 && index < m_materials.size()) {
        m_materials.removeAt(index);
    }
}

void PresetManager::loadBuiltInDefaults()
{
    m_tools = {
        {QStringLiteral("Fraise 2 dents 6 mm"), 2, 6.0, 2, 30.0, 10.0},
        {QStringLiteral("Fraise a surfacer 20 mm"), 1, 20.0, 2, 30.0, 5.0},
        {QStringLiteral("Fraise a surfacer 40 mm"), 1, 40.0, 2, 35.0, 8.0},
    };
    m_materials = {
        {QStringLiteral("Labelite - finition 2 dents"), 12000, 2002.1, 667.4, 0.5},
        {QStringLiteral("Labelite - rainurage 2 dents"), 12000, 1598.9, 533.0, 0.5},
        {QStringLiteral("Bois tendre"), 16000, 1200.0, 300.0, 1.0},
        {QStringLiteral("Aluminium"), 10000, 500.0, 120.0, 0.3},
    };
}
