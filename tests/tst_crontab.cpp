#include "Crontab.h"

#include <QTest>
#include <QFile>
#include <QTemporaryDir>
#include <QScopeGuard>
#include "Clib.h"

#include <memory>

class TstCrontab : public QObject
{
    Q_OBJECT

private slots:
    void commandCommentRoundTripsAfterVariables();
    void commandCommentRoundTripsWithoutVariables();
    void multiLineCommentPreservesInternalBlankLines();
    void commandWhitespaceSurvivesSave();
    void assignmentValuesSurviveSave();
    void staleWriterCannotReplaceCrontab();
    void scopedVariablesPreventLossySave();
    void trailingAndStandaloneCommentsSurviveSave();
};

// A fresh user crontab always has HOME/PATH/SHELL variables (MainWindow adds
// them), so the separating blank line cronText() writes between the
// variables block and the commands block is the common case, not an edge
// case. setup() must not fold that section-separator blank line into the
// next command's comment.
void TstCrontab::commandCommentRoundTripsAfterVariables()
{
    Crontab cron;
    cron.getVariables().push_back(
        std::make_unique<Variable>(QStringLiteral("HOME"), QStringLiteral("/home/user"), QStringLiteral("Home")));
    cron.getVariables().push_back(
        std::make_unique<Variable>(QStringLiteral("SHELL"), QStringLiteral("/bin/bash"), QStringLiteral("Shell")));
    cron.getTCommands().push_back(std::make_unique<TCommand>(QStringLiteral("0 * * * *"), QStringLiteral("user"),
                                                              QStringLiteral("echo hi"),
                                                              QStringLiteral("hello world"), &cron));

    QString text = cron.cronText();

    Crontab reparsed;
    reparsed.setup(text);

    QCOMPARE(reparsed.getTCommands().size(), size_t(1));
    QCOMPARE(reparsed.getTCommands().at(0)->getComment(), QStringLiteral("hello world"));
}

void TstCrontab::commandCommentRoundTripsWithoutVariables()
{
    Crontab cron;
    cron.getTCommands().push_back(std::make_unique<TCommand>(QStringLiteral("0 * * * *"), QStringLiteral("user"),
                                                              QStringLiteral("echo hi"),
                                                              QStringLiteral("hello world"), &cron));

    QString text = cron.cronText();

    Crontab reparsed;
    reparsed.setup(text);

    QCOMPARE(reparsed.getTCommands().size(), size_t(1));
    QCOMPARE(reparsed.getTCommands().at(0)->getComment(), QStringLiteral("hello world"));
}

void TstCrontab::multiLineCommentPreservesInternalBlankLines()
{
    Crontab cron;
    cron.getTCommands().push_back(std::make_unique<TCommand>(
        QStringLiteral("0 * * * *"), QStringLiteral("user"), QStringLiteral("echo hi"),
        QStringLiteral("first paragraph\n\nsecond paragraph"), &cron));

    QString text = cron.cronText();

    Crontab reparsed;
    reparsed.setup(text);

    QCOMPARE(reparsed.getTCommands().size(), size_t(1));
    QCOMPARE(reparsed.getTCommands().at(0)->getComment(), QStringLiteral("first paragraph\n\nsecond paragraph"));
}

void TstCrontab::commandWhitespaceSurvivesSave()
{
    Crontab cron;
    const QString command = QStringLiteral("printf 'two  spaces\tand a tab'  ");
    cron.setup(QStringLiteral("SHELL=/bin/sh  \n  0\t  4 * * *\t") + command + '\n');
    QCOMPARE(cron.getVariables().size(), size_t(1));
    QCOMPARE(cron.getVariables().front()->getValue(), QStringLiteral("/bin/sh"));
    QCOMPARE(cron.getTCommands().front()->getTime(), QStringLiteral("0 4 * * *"));
    QCOMPARE(cron.getTCommands().front()->getCommand(), command);
    QVERIFY(cron.cronText().endsWith(command + '\n'));
}

void TstCrontab::assignmentValuesSurviveSave()
{
    Crontab cron;
    cron.setup(QStringLiteral("LABEL = a=b=c\nTITLE=\"night  shift\"\nEMPTY=\n"));
    QCOMPARE(cron.getVariables().size(), size_t(3));
    QVERIFY(cron.getTCommands().empty());
    QCOMPARE(cron.cronText(), QStringLiteral("LABEL=a=b=c\nTITLE=\"night  shift\"\nEMPTY=\n\n"));
}

void TstCrontab::scopedVariablesPreventLossySave()
{
    Crontab cron;
    cron.setup(QStringLiteral("@daily echo first\nMAILTO=\"\"\n@daily echo second\n"));
    QVERIFY(!cron.saveRestriction().isEmpty());
    QVERIFY(!cron.putCrontab());
    QCOMPARE(cron.estr, cron.saveRestriction());
}

void TstCrontab::trailingAndStandaloneCommentsSurviveSave()
{
    const QString suffix = QStringLiteral("\n# disabled\n# @daily echo 'two  words'\n");
    Crontab cron;
    cron.setup(QStringLiteral("@daily echo live\n") + suffix);
    QVERIFY(cron.cronText().endsWith(QStringLiteral("echo live\n") + suffix));
    Crontab again;
    again.setup(cron.cronText());
    QCOMPARE(again.cronText(), cron.cronText());

    Crontab comments;
    comments.setup(QStringLiteral("# disabled\n# @daily echo later\n"));
    QCOMPARE(comments.getComment(), QStringLiteral("disabled\n@daily echo later"));
    QVERIFY(comments.cronText().contains(QStringLiteral("# @daily echo later\n")));
}

void TstCrontab::staleWriterCannotReplaceCrontab()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto oldPath = qgetenv("PATH");
    const auto restorePath = qScopeGuard([oldPath] { qputenv("PATH", oldPath); });
    QFile installer(dir.filePath("crontab"));
    QVERIFY(installer.open(QIODevice::WriteOnly));
    installer.write("#!/bin/sh\nfixture=${0%/*}/table\n"
                    "case $* in *-l) cat \"$fixture\" ;; *) for file do :; done; cp \"$file\" \"$fixture\" ;; esac\n");
    installer.close();
    QVERIFY(installer.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
    QVERIFY(qputenv("PATH", dir.path().toUtf8() + ':' + oldPath));
    QFile fixture(dir.filePath("table"));
    QVERIFY(fixture.open(QIODevice::WriteOnly));
    fixture.write("@daily echo initial\n");
    fixture.close();

    Crontab first(Clib::uName());
    Crontab second(Clib::uName());
    QVERIFY2(first.putCrontab("@daily echo external\n"), qPrintable(first.estr));
    QVERIFY(!second.putCrontab("@daily echo stale\n"));
    QVERIFY(second.estr.contains("Reload"));
    QVERIFY(fixture.open(QIODevice::ReadOnly));
    QCOMPARE(fixture.readAll(), QByteArray("@daily echo external\n"));
}

QTEST_GUILESS_MAIN(TstCrontab)
#include "tst_crontab.moc"
