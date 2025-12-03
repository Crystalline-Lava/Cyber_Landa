#ifndef TASKVIEW_H
#define TASKVIEW_H

#include <QWidget>
#include <QTreeWidget>
#include <QPushButton>
#include <QComboBox>
#include <vector>
#include <memory>
#include "../core/TaskManager.h"

namespace Ui {
class TaskView;
}

/**
 * @class TaskView
 * @brief 任务管理界面，展示日/周/学期任务并支持完成控制。
 * 中文说明：通过树状视图按周期分类任务，提供完成、提醒与刷新功能，并通过信号将操作交给 TaskManager。
 */
class TaskView : public QWidget {
    Q_OBJECT

public:
    explicit TaskView(rove::data::TaskManager& manager, QWidget* parent = nullptr);
    ~TaskView() override;

signals:
    /**
     * @brief 当用户请求完成任务时发射，供业务层处理。
     * @param taskId 任务标识。
     */
    void taskCompletionRequested(int taskId);

public slots:
    /**
     * @brief 从管理器重新加载任务列表。
     */
    void reloadTasks();

private slots:
    /**
     * @brief 点击完成按钮时触发，提取所选任务信息。
     */
    void onCompleteClicked();

    /**
     * @brief 周期过滤变更时刷新视图。
     * @param index 组合框索引。
     */
    void onPeriodChanged(int index);

private:
    /**
     * @brief 将任务填充到树形控件中。
     */
    void populateTree(const std::vector<rove::data::Task>& tasks);

    std::unique_ptr<Ui::TaskView> ui;
    rove::data::TaskManager& m_taskManager;
    QTreeWidget* m_taskTree{nullptr};
};

#endif  // TASKVIEW_H
