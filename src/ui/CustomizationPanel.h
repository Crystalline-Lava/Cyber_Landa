#ifndef CUSTOMIZATIONPANEL_H
#define CUSTOMIZATIONPANEL_H

#include <QWidget>
#include <QTabWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <memory>
#include "../core/TaskManager.h"
#include "../core/AchievementManager.h"
#include "../core/SerendipityEngine.h"

namespace Ui {
class CustomizationPanel;
}

/**
 * @class CustomizationPanel
 * @brief 自定义内容统一入口，支持创建自定义任务、成就与奇遇事件。
 * 中文说明：采用 Tab 切分不同自定义类型，表单收集后发射信号交给业务层。
 */
class CustomizationPanel : public QWidget {
    Q_OBJECT

public:
    CustomizationPanel(rove::data::TaskManager& taskManager,
                       rove::data::AchievementManager& achievementManager,
                       rove::simulation::SerendipityEngine& engine,
                       QWidget* parent = nullptr);
    ~CustomizationPanel() override;

signals:
    /**
     * @brief 提交自定义任务请求。
     */
    void customTaskCreated(const QString& title, const QString& description, int reward);

    /**
     * @brief 提交自定义成就请求。
     */
    void customAchievementCreated(const QString& name, const QString& condition);

    /**
     * @brief 提交自定义奇遇事件。
     */
    void customSerendipityCreated(const QString& name, int weight);

private slots:
    void onCreateTaskClicked();
    void onCreateAchievementClicked();
    void onCreateSerendipityClicked();

private:
    void setupForms();

    std::unique_ptr<Ui::CustomizationPanel> ui;
    rove::data::TaskManager& m_taskManager;
    rove::data::AchievementManager& m_achievementManager;
    rove::simulation::SerendipityEngine& m_engine;
};

#endif  // CUSTOMIZATIONPANEL_H
