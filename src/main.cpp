#include <QApplication>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>

#include "ui/MainWindow.h"
#include "core/DatabaseManager.h"
#include "core/UserManager.h"
#include "core/TaskManager.h"
#include "core/AchievementManager.h"
#include "core/LogManager.h"
#include "core/ShopManager.h"
#include "core/InventoryManager.h"
#include "core/SerendipityEngine.h"
#include "core/GrowthVisualizer.h"

/**
 * @brief 应用程序入口点
 * 
 * 初始化流程：
 * 1. 初始化数据库连接
 * 2. 创建所有核心管理器实例
 * 3. 创建并显示主窗口
 */
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // 设置应用程序信息
    QApplication::setOrganizationName("LanzhouUniversity");
    QApplication::setApplicationName("Cyber_Landa");
    QApplication::setApplicationVersion("1.0.0");

    try {
        // 初始化数据库
        auto dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(dataPath);
        QString dbPath = dataPath + "/growth.db";
        
        auto& dbManager = rove::data::DatabaseManager::instance();
        dbManager.initialize(dbPath.toStdString());

        // 获取和创建管理器实例
        rove::data::UserManager userManager(dbManager);
        auto& taskManager = rove::data::TaskManager::instance(dbManager, userManager);
        auto& achievementManager = rove::data::AchievementManager::instance(dbManager, userManager, taskManager);
        auto& logManager = rove::data::LogManager::instance(dbManager, userManager, achievementManager, taskManager);
        auto& inventoryManager = rove::data::InventoryManager::instance();
        inventoryManager.initialize(dbManager);
        auto& shopManager = rove::data::ShopManager::instance();
        shopManager.initialize(dbManager, userManager, inventoryManager);
        auto& serendipityEngine = rove::data::SerendipityEngine::instance(dbManager, logManager, userManager);
        auto& growthVisualizer = rove::GrowthVisualizer::instance();

        // 使用预置账号登录 (用户名: x, 密码: 1)
        if (!userManager.login("x", "1")) {
            QMessageBox::critical(nullptr,
                QStringLiteral("登录失败"),
                QStringLiteral("无法登录预置账号，请检查数据库初始化"));
            return 1;
        }

        // 预加载系统成就，确保画廊与奖励逻辑立即可用。
        achievementManager.refreshFromDatabase();

        // 创建主窗口
        MainWindow mainWindow(
            userManager,
            taskManager,
            achievementManager,
            logManager,
            shopManager,
            inventoryManager,
            serendipityEngine,
            growthVisualizer
        );

        mainWindow.setWindowTitle(QStringLiteral("兰大成长模拟 - Cyber Landa"));
        mainWindow.resize(1200, 800);
        mainWindow.show();

        return app.exec();

    } catch (const std::exception& e) {
        QMessageBox::critical(nullptr, 
            QStringLiteral("启动失败"), 
            QString::fromStdString(e.what()));
        return 1;
    }
}
