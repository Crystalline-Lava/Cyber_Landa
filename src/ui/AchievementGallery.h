#ifndef ACHIEVEMENTGALLERY_H
#define ACHIEVEMENTGALLERY_H

#include <QWidget>
#include <QGridLayout>
#include <QScrollArea>
#include <memory>
#include "../core/AchievementManager.h"

namespace Ui {
class AchievementGallery;
}

/**
 * @class AchievementGallery
 * @brief 成就陈列室，网格展示已获得与锁定的徽章。
 * 中文说明：使用滚动网格布局，支持懒加载和实时解锁高亮。
 */
class AchievementGallery : public QWidget {
    Q_OBJECT

public:
    AchievementGallery(rove::data::AchievementManager& manager, QWidget* parent = nullptr);
    ~AchievementGallery() override;

public slots:
    /**
     * @brief 从管理器拉取最新成就并刷新。
     */
    void reload();

private:
    /**
     * @brief 创建单个成就卡片控件。
     * @param achievement 业务成就数据。
     * @return QWidget* 卡片。
     */
    QWidget* createCard(const rove::data::Achievement& achievement);

    std::unique_ptr<Ui::AchievementGallery> ui;
    rove::data::AchievementManager& m_manager;
    QGridLayout* m_grid{nullptr};
};

#endif  // ACHIEVEMENTGALLERY_H
