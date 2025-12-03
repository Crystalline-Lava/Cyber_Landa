#include "DashboardWidget.h"
#include "ui_DashboardWidget.h"

#include <QtCharts/QCategoryAxis>
#include <QtCharts/QRadarSeries>
#include <QVBoxLayout>

using namespace QtCharts;

/**
 * @brief 构造函数：加载 UI 并初始化雷达图。
 */
DashboardWidget::DashboardWidget(QWidget* parent)
    : QWidget(parent), ui(std::make_unique<Ui::DashboardWidget>()) {
    ui->setupUi(this);
    buildRadarChart();
}

DashboardWidget::~DashboardWidget() = default;

/**
 * @brief 渲染用户基本数据与属性雷达图。
 */
void DashboardWidget::renderUser(const rove::data::User& user) {
    ui->statsLabel->setText(
        QStringLiteral("等级 %1 | 金币 %2 | 成就 %3")
            .arg(user.level())
            .arg(user.coins())
            .arg(user.achievementsUnlocked()));
    updateRadar(user.attributes());
}

/**
 * @brief 更新最近活动列表。
 */
void DashboardWidget::setRecentActivities(const QStringList& activities) {
    ui->activityList->clear();
    ui->activityList->addItems(activities);
}

/**
 * @brief 内部函数：创建雷达图，绑定到布局。
 */
void DashboardWidget::buildRadarChart() {
    m_polarChart = new QPolarChart();
    m_polarChart->setTitle(QStringLiteral("属性雷达"));

    auto* radial = new QValueAxis();
    radial->setRange(0, 100);
    radial->setTickCount(6);
    m_polarChart->addAxis(radial, QPolarChart::PolarOrientationRadial);

    auto* angular = new QCategoryAxis();
    angular->append(QStringLiteral("学术"), 1);
    angular->append(QStringLiteral("社交"), 2);
    angular->append(QStringLiteral("体魄"), 3);
    angular->append(QStringLiteral("艺术"), 4);
    angular->append(QStringLiteral("奉献"), 5);
    m_polarChart->addAxis(angular, QPolarChart::PolarOrientationAngular);

    m_chartView = new QChartView(m_polarChart, this);
    ui->chartLayout->addWidget(m_chartView);
}

/**
 * @brief 更新雷达图数据集。
 */
void DashboardWidget::updateRadar(const rove::data::User::AttributeSet& attrs) {
    auto series = new QRadarSeries();
    series->append(1, attrs.academic);
    series->append(2, attrs.social);
    series->append(3, attrs.physical);
    series->append(4, attrs.art);
    series->append(5, attrs.volunteer);
    series->setName(QStringLiteral("当前属性"));

    const auto allSeries = m_polarChart->series();
    for (QAbstractSeries* old : allSeries) {
        m_polarChart->removeSeries(old);
        old->deleteLater();
    }
    m_polarChart->addSeries(series);
    for (QAbstractAxis* axis : m_polarChart->axes()) {
        series->attachAxis(axis);
    }
}
