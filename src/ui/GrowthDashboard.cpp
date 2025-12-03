#include "GrowthDashboard.h"
#include "ui_GrowthDashboard.h"

#include <QtCharts/QCategoryAxis>
#include <QtCharts/QRadarSeries>
#include <QtCharts/QLineSeries>
#include <QVBoxLayout>

using namespace QtCharts;

GrowthDashboard::GrowthDashboard(rove::analysis::GrowthVisualizer& visualizer, QWidget* parent)
    : QWidget(parent), ui(std::make_unique<Ui::GrowthDashboard>()), m_visualizer(visualizer) {
    ui->setupUi(this);
    m_radarView = new QChartView(new QChart(), this);
    ui->radarLayout->addWidget(m_radarView);

    m_lineView = new QChartView(new QChart(), this);
    ui->timelineLayout->addWidget(m_lineView);
}

GrowthDashboard::~GrowthDashboard() = default;

void GrowthDashboard::render(const rove::data::User& user,
                             const std::vector<rove::analysis::GrowthSnapshot>& snapshots) {
    updateRadar(user.attributes());
    buildTimeline(snapshots);
}

/**
 * @brief 绘制成长时间线，使用折线图展示成长值趋势。
 */
void GrowthDashboard::buildTimeline(const std::vector<rove::analysis::GrowthSnapshot>& snapshots) {
    auto* chart = new QChart();
    auto* series = new QLineSeries(chart);
    int index = 0;
    for (const auto& snap : snapshots) {
        series->append(index++, snap.growthValue);
    }
    chart->addSeries(series);
    auto* axisX = new QValueAxis();
    axisX->setTitleText(QStringLiteral("时间"));
    chart->addAxis(axisX, Qt::AlignBottom);
    auto* axisY = new QValueAxis();
    axisY->setTitleText(QStringLiteral("成长"));
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisX);
    series->attachAxis(axisY);
    m_lineView->setChart(chart);
}

/**
 * @brief 使用 GrowthVisualizer 输出的属性集更新雷达图。
 */
void GrowthDashboard::updateRadar(const rove::data::User::AttributeSet& attrs) {
    auto* chart = new QPolarChart();
    auto* radial = new QValueAxis();
    radial->setRange(0, 100);
    chart->addAxis(radial, QPolarChart::PolarOrientationRadial);
    auto* angular = new QCategoryAxis();
    angular->append(QStringLiteral("学术"), 1);
    angular->append(QStringLiteral("社交"), 2);
    angular->append(QStringLiteral("体魄"), 3);
    angular->append(QStringLiteral("艺术"), 4);
    angular->append(QStringLiteral("奉献"), 5);
    chart->addAxis(angular, QPolarChart::PolarOrientationAngular);

    auto* series = new QRadarSeries(chart);
    series->append(1, attrs.academic);
    series->append(2, attrs.social);
    series->append(3, attrs.physical);
    series->append(4, attrs.art);
    series->append(5, attrs.volunteer);
    chart->addSeries(series);
    series->attachAxis(radial);
    series->attachAxis(angular);

    m_radarView->setChart(chart);
}
