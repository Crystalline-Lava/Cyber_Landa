#include "CustomizationPanel.h"
#include "ui_CustomizationPanel.h"

#include <QMessageBox>
#include <QDateTime>
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

void CustomizationPanel::onCreateAchievementClicked() {
    const auto name = ui->achNameEdit->text();
    const auto cond = ui->achCondEdit->toPlainText();
    emit customAchievementCreated(name, cond);
    rove::data::Achievement ac;
    ac.setName(name.toStdString());
    ac.setDescription(cond.toStdString());
    ac.setOwner("student");
    m_achievementManager.createCustomAchievement(ac);
}

void CustomizationPanel::onCreateSerendipityClicked() {
    const auto name = ui->serNameEdit->text();
    const int weight = ui->serWeightEdit->text().toInt();
    emit customSerendipityCreated(name, weight);
    auto config = m_engine.probability();
    config.buffChance = std::min(0.9, config.buffChance + weight / 100.0);
    m_engine.updateProbability(config);
}


