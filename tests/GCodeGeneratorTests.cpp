#include "GCodeGenerator.h"

#include <QtTest>

class GCodeGeneratorTests : public QObject
{
    Q_OBJECT

private slots:
    void rejectsInvalidValues();
    void createsMultipleDepthPasses();
    void createsMach3Program();
    void supportsBottomRightOrigin();
};

void GCodeGeneratorTests::rejectsInvalidValues()
{
    SurfacingJob job;
    job.lengthX = 0.0;

    GCodeGenerator generator;
    QVERIFY(!generator.validate(job).isEmpty());
    QVERIFY(!generator.generate(job).success);
}

void GCodeGeneratorTests::createsMultipleDepthPasses()
{
    SurfacingJob job;
    job.totalDepth = 1.1;
    job.depthPerPass = 0.5;

    GCodeGenerator generator;
    const QVector<double> depths = generator.buildDepths(job);

    QCOMPARE(depths.size(), 3);
    QCOMPARE(depths.at(0), -0.5);
    QCOMPARE(depths.at(1), -1.0);
    QCOMPARE(depths.at(2), -1.1);
}

void GCodeGeneratorTests::createsMach3Program()
{
    SurfacingJob job;
    job.tool.name = QStringLiteral("Outil test");
    job.tool.toolNumber = 2;
    job.tool.fluteCount = 2;
    job.material.name = QStringLiteral("Matiere test");
    job.material.spindleSpeed = 12000;

    GCodeGenerator generator;
    const GenerationResult result = generator.generate(job);

    QVERIFY2(result.success, qPrintable(result.error));
    QVERIFY(result.gcode.contains(QStringLiteral("G21")));
    QVERIFY(result.gcode.contains(QStringLiteral("G90 G94")));
    QVERIFY(result.gcode.contains(QStringLiteral("T2\n")));
    QVERIFY(result.gcode.contains(QStringLiteral("S12000 M3")));
    QVERIFY(result.gcode.contains(QStringLiteral("G54")));
    QVERIFY(result.gcode.contains(QStringLiteral("G0 Z5\n")));
    QVERIFY(!result.gcode.contains(QStringLiteral("Z5.000")));
    QVERIFY(result.gcode.indexOf(QStringLiteral("G0 Z5\n"))
            < result.gcode.indexOf(QStringLiteral("G0 X")));
    QVERIFY(result.gcode.endsWith(QStringLiteral("M30\n")));
    QVERIFY(result.geometry.scanLineCount >= 2);
    QVERIFY(result.summary.estimatedPathLength > 0.0);
}

void GCodeGeneratorTests::supportsBottomRightOrigin()
{
    SurfacingJob job;
    job.origin = JobOrigin::BottomRight;

    GCodeGenerator generator;
    const ToolpathGeometry geometry = generator.buildGeometry(job);

    QCOMPARE(geometry.workpiece.right(), 0.0);
    QCOMPARE(geometry.workpiece.left(), -job.lengthX);
    QCOMPARE(geometry.origin, QPointF(0.0, 0.0));
}

QTEST_APPLESS_MAIN(GCodeGeneratorTests)

#include "GCodeGeneratorTests.moc"
