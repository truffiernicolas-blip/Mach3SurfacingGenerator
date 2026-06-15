#pragma once

#include "MaterialPreset.h"
#include "ToolPreset.h"

#include <QString>
#include <QVector>

class PresetManager
{
public:
    explicit PresetManager(QString filePath);

    [[nodiscard]] bool load(QString *error = nullptr);
    [[nodiscard]] bool save(QString *error = nullptr) const;

    [[nodiscard]] const QVector<ToolPreset> &tools() const;
    [[nodiscard]] const QVector<MaterialPreset> &materials() const;
    [[nodiscard]] QString filePath() const;

    void addTool(const ToolPreset &preset);
    void updateTool(int index, const ToolPreset &preset);
    void removeTool(int index);

    void addMaterial(const MaterialPreset &preset);
    void updateMaterial(int index, const MaterialPreset &preset);
    void removeMaterial(int index);

private:
    void loadBuiltInDefaults();

    QString m_filePath;
    QVector<ToolPreset> m_tools;
    QVector<MaterialPreset> m_materials;
};

