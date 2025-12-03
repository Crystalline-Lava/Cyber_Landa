#include "TaskView.h"
#include "ui_TaskView.h"

#include <QMessageBox>

/**
 * @brief 构造函数：加载 UI 并绑定信号。
 */
TaskView::TaskView(rove::data::TaskManager& manager, QWidget* parent)
    : QWidget(parent), ui(std::make_unique<Ui::TaskView>()), m_taskManager(manager) {
    ui->setupUi(this);
    m_taskTree = ui->taskTree;
    ui->periodCombo->addItems({QStringLiteral("全部"), QStringLiteral("日常"), QStringLiteral("每周"),
                               QStringLiteral("学期")});
    connect(ui->completeBtn, &QPushButton::clicked, this, &TaskView::onCompleteClicked);
    connect(ui->periodCombo,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            &TaskView::onPeriodChanged);
    reloadTasks();
}

TaskView::~TaskView() = default;

void TaskView::reloadTasks() {
    std::vector<rove::data::Task> tasks;
    const auto daily = m_taskManager.tasksByType(rove::data::Task::TaskType::Daily);
    const auto weekly = m_taskManager.tasksByType(rove::data::Task::TaskType::Weekly);
    const auto semester = m_taskManager.tasksByType(rove::data::Task::TaskType::Semester);
    const auto custom = m_taskManager.tasksByType(rove::data::Task::TaskType::Custom);
    tasks.insert(tasks.end(), daily.begin(), daily.end());
    tasks.insert(tasks.end(), weekly.begin(), weekly.end());
    tasks.insert(tasks.end(), semester.begin(), semester.end());
    tasks.insert(tasks.end(), custom.begin(), custom.end());
    populateTree(tasks);
}

/**
 * @brief 点击完成按钮后的槽函数，校验选择并发射信号。
 */
void TaskView::onCompleteClicked() {
    auto* item = m_taskTree->currentItem();
    if (!item) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先选择一个任务"));
        return;
    }
    const int taskId = item->data(0, Qt::UserRole).toInt();
    emit taskCompletionRequested(taskId);
}

void TaskView::onPeriodChanged(int index) {
    std::vector<rove::data::Task> tasks;
    const auto daily = m_taskManager.tasksByType(rove::data::Task::TaskType::Daily);
    const auto weekly = m_taskManager.tasksByType(rove::data::Task::TaskType::Weekly);
    const auto semester = m_taskManager.tasksByType(rove::data::Task::TaskType::Semester);
    const auto custom = m_taskManager.tasksByType(rove::data::Task::TaskType::Custom);
    tasks.insert(tasks.end(), daily.begin(), daily.end());
    tasks.insert(tasks.end(), weekly.begin(), weekly.end());
    tasks.insert(tasks.end(), semester.begin(), semester.end());
    tasks.insert(tasks.end(), custom.begin(), custom.end());
    std::vector<rove::data::Task> filtered;
    for (const auto& task : tasks) {
        if (index == 0) {
            filtered.push_back(task);
        } else if (index == 1 && task.type() == rove::data::Task::TaskType::Daily) {
            filtered.push_back(task);
        } else if (index == 2 && task.type() == rove::data::Task::TaskType::Weekly) {
            filtered.push_back(task);
        } else if (index == 3 && task.type() == rove::data::Task::TaskType::Semester) {
            filtered.push_back(task);
        }
    }
    populateTree(filtered);
}

/**
 * @brief 将任务列表填充到树控件，支持懒加载。
 */
void TaskView::populateTree(const std::vector<rove::data::Task>& tasks) {
    m_taskTree->clear();
    for (const auto& task : tasks) {
        auto* item = new QTreeWidgetItem(m_taskTree);
        item->setText(0, QString::fromStdString(task.name()));
        item->setText(1, QStringLiteral("成长 %1 / 金币 %2")
                             .arg(task.growthReward())
                             .arg(task.coinReward()));
        item->setText(2, task.isCompleted() ? QStringLiteral("已完成") : QStringLiteral("未完成"));
        item->setData(0, Qt::UserRole, task.id());
    }
    m_taskTree->resizeColumnToContents(0);
}
