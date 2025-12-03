#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QSystemTrayIcon>
#include <QTimer>
#include <memory>

#include "../core/UserManager.h"
#include "../core/TaskManager.h"
#include "../core/AchievementManager.h"
#include "../core/LogManager.h"
#include "../core/ShopManager.h"
#include "../core/InventoryManager.h"
#include "../core/SerendipityEngine.h"
#include "../core/GrowthVisualizer.h"

class DashboardWidget;
class TaskView;
class AchievementGallery;
class GrowthDashboard;
class ShopInterface;
class LogBrowser;
class CustomizationPanel;
class TutorialManager;

namespace Ui {
class MainWindow;
}

/**
 * @class MainWindow
 * @brief 主窗口负责整合所有子系统，提供导航、实时通知以及用户概览。
 * 中文说明：该类承担 MVC 中的视图与部分协调职责，持有各个子视图组件并通过信号槽与核心业务管理器交互，确保 UI 状态与后端数据保持一致。
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    /**
     * @brief 构造函数注入所有业务管理器依赖并初始化界面。
     * @param userManager 用户会话及属性管理器引用。
     * @param taskManager 任务管理器引用，用于任务列表加载与状态更新。
     * @param achievementManager 成就管理器引用，用于展示与实时解锁通知。
     * @param logManager 日志管理器引用，驱动时间轴与过滤功能。
     * @param shopManager 商店管理器引用，支撑购买逻辑与库存刷新。
     * @param inventoryManager 背包管理器引用，支撑库存界面与优惠券应用。
     * @param serendipityEngine 奇遇系统引用，用于实时事件提醒。
     */
    MainWindow(rove::data::UserManager& userManager,
               rove::data::TaskManager& taskManager,
               rove::data::AchievementManager& achievementManager,
               rove::data::LogManager& logManager,
               rove::data::ShopManager& shopManager,
               rove::data::InventoryManager& inventoryManager,
               rove::simulation::SerendipityEngine& serendipityEngine,
               rove::data::GrowthVisualizer& growthVisualizer,
               QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    /**
     * @brief 切换主堆叠页，驱动导航按钮状态。
     * @param index 目标页索引。
     */
    void onSectionChanged(int index);

    /**
     * @brief 显示来自成就或奇遇的实时通知。
     * @param message 展示文本。
     */
    void showRealtimeNotification(const QString& message);

    /**
     * @brief 刷新仪表盘的核心统计数据。
     */
    void refreshDashboard();

    /**
     * @brief 响应教程跳过或完成，关闭引导提示。
     */
    void handleTutorialFinished();

private:
    /**
     * @brief 建立导航按钮与堆叠页之间的映射关系。
     */
    void setupNavigation();

    /**
     * @brief 将业务信号与 UI 更新槽函数连接。
     */
    void connectSignals();

    /**
     * @brief 初始化托盘图标和提示气泡，用于后台通知。
     */
    void setupTrayIcon();

    std::unique_ptr<Ui::MainWindow> ui;  //!< UI 指针负责托管 .ui 生成的控件

    rove::data::UserManager& m_userManager;
    rove::data::TaskManager& m_taskManager;
    rove::data::AchievementManager& m_achievementManager;
    rove::data::LogManager& m_logManager;
    rove::data::ShopManager& m_shopManager;
    rove::data::InventoryManager& m_inventoryManager;
    rove::simulation::SerendipityEngine& m_serendipityEngine;
    rove::data::GrowthVisualizer& m_growthVisualizer;

    DashboardWidget* m_dashboard{nullptr};
    TaskView* m_taskView{nullptr};
    AchievementGallery* m_achievementGallery{nullptr};
    GrowthDashboard* m_growthDashboard{nullptr};
    ShopInterface* m_shopInterface{nullptr};
    LogBrowser* m_logBrowser{nullptr};
    CustomizationPanel* m_customizationPanel{nullptr};
    TutorialManager* m_tutorialManager{nullptr};

    QSystemTrayIcon* m_trayIcon{nullptr};
    QTimer* m_reminderTimer{nullptr};
};

#endif  // MAINWINDOW_H
