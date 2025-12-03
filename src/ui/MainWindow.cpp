#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QToolButton>
#include <QVBoxLayout>
#include <optional>

#include "AchievementGallery.h"
#include "CustomizationPanel.h"
#include "DashboardWidget.h"
#include "GrowthDashboard.h"
#include "LogBrowser.h"
#include "ShopInterface.h"
#include "../core/ShopItem.h"
#include "../core/LogEntry.h"
#include "TaskView.h"
#include "TutorialManager.h"

/**
 * @brief 构造函数：创建所有子组件并连接业务逻辑。
 */
MainWindow::MainWindow(rove::data::UserManager& userManager,
                       rove::data::TaskManager& taskManager,
                       rove::data::AchievementManager& achievementManager,
                       rove::data::LogManager& logManager,
                       rove::data::ShopManager& shopManager,
                       rove::data::InventoryManager& inventoryManager,
                       rove::simulation::SerendipityEngine& serendipityEngine,
                       rove::data::GrowthVisualizer& growthVisualizer,
                       QWidget* parent)
    : QMainWindow(parent)
    , ui(std::make_unique<Ui::MainWindow>())
    , m_userManager(userManager)
    , m_taskManager(taskManager)
    , m_achievementManager(achievementManager)
    , m_logManager(logManager)
    , m_shopManager(shopManager)
    , m_inventoryManager(inventoryManager)
    , m_serendipityEngine(serendipityEngine)
    , m_growthVisualizer(growthVisualizer) {
    ui->setupUi(this);

    // 初始化子组件
    m_dashboard = new DashboardWidget(this);
    m_taskView = new TaskView(m_taskManager, this);
    m_achievementGallery = new AchievementGallery(m_achievementManager, this);
    m_growthDashboard = new GrowthDashboard(m_growthVisualizer, this);
    m_shopInterface = new ShopInterface(m_shopManager, m_inventoryManager, this);
    m_logBrowser = new LogBrowser(m_logManager, this);
    m_customizationPanel = new CustomizationPanel(m_taskManager, m_achievementManager, m_serendipityEngine, this);
    m_tutorialManager = new TutorialManager(this);

    // 安装布局到各页面
    auto setPage = [](QWidget* page, QWidget* child) {
        auto* layout = new QVBoxLayout(page);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(child);
    };
    setPage(ui->pageDashboard, m_dashboard);
    setPage(ui->pageTask, m_taskView);
    setPage(ui->pageAchievement, m_achievementGallery);
    setPage(ui->pageGrowth, m_growthDashboard);
    setPage(ui->pageShop, m_shopInterface);
    setPage(ui->pageLog, m_logBrowser);
    setPage(ui->pageCustom, m_customizationPanel);

    setupNavigation();
    connectSignals();
    setupTrayIcon();
    refreshDashboard();
    showRealtimeNotification(m_tutorialManager->currentHint());
}

MainWindow::~MainWindow() = default;

void MainWindow::setupNavigation() {
    // 导航按钮与堆叠页对应
    connect(ui->dashboardBtn, &QPushButton::clicked, [this] { onSectionChanged(0); });
    connect(ui->taskBtn, &QPushButton::clicked, [this] { onSectionChanged(1); });
    connect(ui->achievementBtn, &QPushButton::clicked, [this] { onSectionChanged(2); });
    connect(ui->growthBtn, &QPushButton::clicked, [this] { onSectionChanged(3); });
    connect(ui->shopBtn, &QPushButton::clicked, [this] { onSectionChanged(4); });
    connect(ui->logBtn, &QPushButton::clicked, [this] { onSectionChanged(5); });
    connect(ui->customBtn, &QPushButton::clicked, [this] { onSectionChanged(6); });
    connect(ui->skipTutorialBtn, &QPushButton::clicked, m_tutorialManager, &TutorialManager::skip);

    // 皮肤与心情/宽恕券/背包面板
    auto* navLayout = qobject_cast<QVBoxLayout*>(ui->navPanel->layout());
    auto* skinCombo = new QComboBox(ui->navPanel);
    skinCombo->addItems({QStringLiteral("校园蓝"), QStringLiteral("绿意"), QStringLiteral("夜间")});
    navLayout->insertWidget(navLayout->count() - 1, new QLabel(QStringLiteral("外观主题"), ui->navPanel));
    navLayout->insertWidget(navLayout->count() - 1, skinCombo);
    connect(skinCombo, &QComboBox::currentTextChanged, [this](const QString& skin) {
        if (skin.contains(QStringLiteral("夜"))) {
            this->setStyleSheet("background:#1f1f2e;color:#f0f0f0;");
        } else if (skin.contains(QStringLiteral("绿"))) {
            this->setStyleSheet("background:#f0fff4;");
        } else {
            this->setStyleSheet("");
        }
    });

    auto* moodCombo = new QComboBox(ui->navPanel);
    moodCombo->addItems({QStringLiteral("😊 开心"), QStringLiteral("😐 平静"), QStringLiteral("😢 低落")});
    navLayout->insertWidget(navLayout->count() - 1, new QLabel(QStringLiteral("今日心情"), ui->navPanel));
    navLayout->insertWidget(navLayout->count() - 1, moodCombo);
    connect(moodCombo, &QComboBox::currentTextChanged, [this](const QString& mood) {
        rove::logging::LogEntry::MoodTag tag = rove::logging::LogEntry::MoodTag::Neutral;
        if (mood.contains(QStringLiteral("开心"))) {
            tag = rove::logging::LogEntry::MoodTag::Happy;
        } else if (mood.contains(QStringLiteral("低落"))) {
            tag = rove::logging::LogEntry::MoodTag::Sad;
        }
        m_logManager.recordManualLog(mood.toStdString(), tag);
        showRealtimeNotification(QStringLiteral("已记录心情：%1").arg(mood));
    });

    auto* couponBtn = new QPushButton(QStringLiteral("使用宽恕券"), ui->navPanel);
    navLayout->insertWidget(navLayout->count() - 1, couponBtn);
    connect(couponBtn, &QPushButton::clicked, [this] {
        const auto username = m_userManager.activeUser().username();
        bool ok = m_inventoryManager.consumeEffectToken(username, rove::data::ShopItem::PropEffectType::ForgivenessCoupon);
        showRealtimeNotification(ok ? QStringLiteral("宽恕券已使用，下一次失败免惩罚")
                                    : QStringLiteral("没有可用的宽恕券"));
    });

    // 背包简易表格
    auto* inventoryTable = new QTableWidget(ui->navPanel);
    inventoryTable->setColumnCount(3);
    inventoryTable->setHorizontalHeaderLabels({QStringLiteral("名称"), QStringLiteral("数量"), QStringLiteral("属性")});
    navLayout->insertWidget(navLayout->count() - 1, new QLabel(QStringLiteral("随身道具"), ui->navPanel));
    navLayout->insertWidget(navLayout->count() - 1, inventoryTable);
    auto refreshInventory = [this, inventoryTable]() {
        const auto username = m_userManager.activeUser().username();
        const auto items = m_inventoryManager.listByOwner(username);
        inventoryTable->setRowCount(static_cast<int>(items.size()));
        int row = 0;
        for (const auto& it : items) {
            inventoryTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(it.name())));
            inventoryTable->setItem(row, 1, new QTableWidgetItem(QString::number(it.quantity())));
            inventoryTable->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(it.specialAttributes())));
            ++row;
        }
    };
    refreshInventory();
    connect(m_shopInterface, &ShopInterface::purchaseRequested, this, [refreshInventory]() { refreshInventory(); });
}

void MainWindow::connectSignals() {
    connect(m_taskView, &TaskView::taskCompletionRequested, this, [this](int taskId) {
        m_taskManager.markTaskCompleted(taskId);
        m_logManager.recordAutoLog(rove::logging::LogEntry::LogType::Task,
                                   QStringLiteral("任务完成").toStdString(),
                                   {});
        m_tutorialManager->markStepDone(QStringLiteral("createTask"));
        refreshDashboard();
    });

    connect(m_shopInterface, &ShopInterface::purchaseRequested, this, [this](int itemId) {
        auto result = m_shopManager.purchaseItem(itemId, 1);
        showRealtimeNotification(QString::fromStdString(result.message));
        m_tutorialManager->markStepDone(QStringLiteral("firstPurchase"));
        refreshDashboard();
    });

    connect(m_customizationPanel,
            &CustomizationPanel::customAchievementCreated,
            this,
            [this](const QString&, const QString&) { m_tutorialManager->markStepDone(QStringLiteral("firstAchievement")); });

    connect(&m_achievementManager,
            &rove::data::AchievementManager::achievementUnlocked,
            this,
            [this](int) { showRealtimeNotification(QStringLiteral("新的成就已解锁！")); });

    connect(m_userManager.signalProxy(), &rove::data::UserManager::SignalProxy::coinsChanged, this, [this](int) {
        refreshDashboard();
    });

    connect(m_tutorialManager, &TutorialManager::tutorialHintChanged, this, &MainWindow::showRealtimeNotification);
    connect(m_tutorialManager, &TutorialManager::tutorialFinished, this, &MainWindow::handleTutorialFinished);

    // 任务提醒定时器
    m_reminderTimer = new QTimer(this);
    m_reminderTimer->setInterval(600000);
    connect(m_reminderTimer, &QTimer::timeout, this, [this] {
        showRealtimeNotification(QStringLiteral("记得查看今日任务，保持成长节奏！"));
    });
    m_reminderTimer->start();

    // 触发一次奇遇
    auto result = m_serendipityEngine.triggerDailyLogin();
    if (result.triggered) {
        showRealtimeNotification(QString::fromStdString(result.description));
    }
}

void MainWindow::setupTrayIcon() {
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setToolTip(QStringLiteral("兰大成长模拟"));
    m_trayIcon->show();
}

void MainWindow::onSectionChanged(int index) {
    ui->stackedWidget->setCurrentIndex(index);
}

void MainWindow::showRealtimeNotification(const QString& message) {
    ui->notificationLabel->setText(message);
    if (m_trayIcon) {
        m_trayIcon->showMessage(QStringLiteral("校园提醒"), message);
    }
}

void MainWindow::refreshDashboard() {
    if (!m_userManager.hasActiveUser()) {
        return;
    }
    const auto& user = m_userManager.activeUser();
    m_dashboard->renderUser(user);
    m_achievementGallery->reload();
    m_taskView->reloadTasks();
    m_shopInterface->reload();
    m_logBrowser->reload();

    const auto snapshots = m_logManager.querySnapshots(std::nullopt, std::nullopt);
    m_growthDashboard->render(user, snapshots);
}

void MainWindow::handleTutorialFinished() {
    showRealtimeNotification(QStringLiteral("教程完成，奖励金币已发放"));
}
