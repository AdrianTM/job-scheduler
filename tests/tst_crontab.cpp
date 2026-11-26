#include "Crontab.h"

#include <QTest>

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

QTEST_GUILESS_MAIN(TstCrontab)
#include "tst_crontab.moc"
