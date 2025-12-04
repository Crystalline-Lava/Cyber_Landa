#include "CustomizationPanel.h"
#include "ui_CustomizationPanel.h"

#include <QMessageBox>
#include <QDateTime>
#include <QRegularExpression>
#include <QColor>
#include <algorithm>
#include "../core/Task.h"
#include "../core/Achievement.h"

CustomizationPanel::CustomizationPanel(rove::data::TaskManager& taskManager,
                                       rove::data::AchievementManager& achievementManager,
                                       rove::data::SerendipityEngine& engine,
                                       QWidget* parent)
    : QWidget(parent)
    , ui(std::make_unique<Ui::CustomizationPanel>())
    , m_taskManager(taskManager)
    , m_achievementManager(achievementManager)
    , m_engine(engine) {
    ui->setupUi(this);
    setupForms();
}

CustomizationPanel::~CustomizationPanel() = default;

void CustomizationPanel::setupForms() {
    connect(ui->createTaskBtn, &QPushButton::clicked, this, &CustomizationPanel::onCreateTaskClicked);
    connect(ui->createAchievementBtn,
            &QPushButton::clicked,
            this,
            &CustomizationPanel::onCreateAchievementClicked);
    connect(ui->createSerendipityBtn,
            &QPushButton::clicked,
            this,
            &CustomizationPanel::onCreateSerendipityClicked);
}

void CustomizationPanel::onCreateTaskClicked() {
    const auto title = ui->taskTitleEdit->text();
    const auto desc = ui->taskDescEdit->toPlainText();
    const int reward = ui->taskRewardEdit->text().toInt();
    if (title.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提醒"), QStringLiteral("标题不能为空"));
        return;
    }
    emit customTaskCreated(title, desc, reward);
    rove::data::Task task;
    task.setName(title.toStdString());
    task.setDescription(desc.toStdString());
    task.setType(rove::data::Task::TaskType::Custom);
    task.setCoinReward(reward);
    task.setGrowthReward(reward / 2);
    task.setDeadline(QDateTime::currentDateTime().addDays(7));
    m_taskManager.createTask(task);
}

/**
 * @brief 创建自定义成就，带有最小化的输入校验与默认模板。
 * 中文：AchievementManager 要求至少存在一条条件，否则会抛出运行时异常导致应用退出。
 *       因此这里主动解析“条件”文本，生成一个可用的 CustomCounter 条件并捕获异常，
 *       以提示方式反馈给学生。
 */
void CustomizationPanel::onCreateAchievementClicked() {
    const QString name = ui->achNameEdit->text().trimmed();
    const QString condText = ui->achCondEdit->toPlainText().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提醒"), QStringLiteral("成就名称不能为空"));
        return;
    }

    // 解析“条件”输入中的数值；若未找到数字则默认一次完成即可解锁。
    bool ok = false;
    int targetValue = condText.toInt(&ok);
    if (!ok) {
        QRegularExpression re("(\\d+)");
        auto match = re.match(condText);
        if (match.hasMatch()) {
            targetValue = match.captured(1).toInt();
        } else {
            targetValue = 1;
        }
    }
    targetValue = std::max(1, targetValue);

    rove::data::Achievement::Condition condition;
    condition.type = rove::data::Achievement::Condition::ConditionType::CustomCounter;
    condition.targetValue = targetValue;
    condition.currentValue = 0;
    condition.metadata = condText.toStdString();

    rove::data::Achievement achievement;
    achievement.setName(name.toStdString());
    achievement.setDescription(condText.isEmpty() ? QStringLiteral("完成一次自定义目标").toStdString()
                                                  : condText.toStdString());
    achievement.setProgressMode(rove::data::Achievement::ProgressMode::Incremental);
    achievement.setRewardType(rove::data::Achievement::RewardType::NoReward);
    achievement.setDisplayColor(QColor("#2196F3"));
    achievement.setIconPath(":/icons/custom.png");
    achievement.setGalleryGroup("自定义成就");
    achievement.setConditions({condition});
    achievement.setProgressGoal(targetValue);

    emit customAchievementCreated(name, condText);
    try {
        m_achievementManager.createCustomAchievement(achievement);
        QMessageBox::information(this, QStringLiteral("成功"), QStringLiteral("自定义成就已创建"));
    } catch (const std::exception& ex) {
        QMessageBox::critical(this, QStringLiteral("创建失败"), QString::fromStdString(ex.what()));
    }
}

void CustomizationPanel::onCreateSerendipityClicked() {
    const auto name = ui->serNameEdit->text();
    const int weight = ui->serWeightEdit->text().toInt();
    emit customSerendipityCreated(name, weight);
    auto config = m_engine.probability();
    config.buffChance = std::min(0.9, config.buffChance + weight / 100.0);
    m_engine.updateProbability(config);
}


