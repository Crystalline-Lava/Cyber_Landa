#include "AchievementGallery.h"
#include "ui_AchievementGallery.h"

#include <QLabel>
#include <QVBoxLayout>

/**
 * @brief 构造函数：搭建网格布局。
 */
AchievementGallery::AchievementGallery(rove::data::AchievementManager& manager, QWidget* parent)
    : QWidget(parent), ui(std::make_unique<Ui::AchievementGallery>()), m_manager(manager) {
    ui->setupUi(this);
    m_grid = ui->gridLayout;
}

AchievementGallery::~AchievementGallery() = default;

/**
 * @brief 重新加载成就网格：清理旧控件，按行列填充新卡片。
 * 中文：当 AchievementManager 状态刷新或新增自定义成就时调用，避免残留指针导致崩溃。
 */
void AchievementGallery::reload() {
    const auto all = m_manager.achievements();
    while (m_grid->count() > 0) {
        if (auto* item = m_grid->takeAt(0)) {
            delete item->widget();
            delete item;
        }
    }

    if (all.empty()) {
        auto* placeholder = new QLabel(QStringLiteral("暂无成就，先去完成任务试试吧"), this);
        placeholder->setAlignment(Qt::AlignCenter);
        m_grid->addWidget(placeholder, 0, 0);
        return;
    }

    int row = 0;
    int col = 0;
    for (const auto& ac : all) {
        m_grid->addWidget(createCard(ac), row, col);
        ++col;
        if (col > 2) {
            col = 0;
            ++row;
        }
    }
}

/**
 * @brief 创建单个成就卡片，展示图标、名称与状态。
 */
QWidget* AchievementGallery::createCard(const rove::data::Achievement& achievement) {
    auto* card = new QWidget(this);
    auto* layout = new QVBoxLayout(card);
    auto* nameLabel = new QLabel(QString::fromStdString(achievement.name()), card);
    auto* descLabel = new QLabel(QString::fromStdString(achievement.description()), card);
    auto* statusLabel = new QLabel(achievement.unlocked() ? QStringLiteral("已解锁")
                                                           : QStringLiteral("未解锁"),
                                  card);
    layout->addWidget(nameLabel);
    layout->addWidget(descLabel);
    layout->addWidget(statusLabel);
    card->setStyleSheet(achievement.unlocked() ? "background:#e6ffe6;" : "background:#f2f2f2;");
    return card;
}



