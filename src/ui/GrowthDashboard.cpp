#include "GrowthDashboard.h"
#include "ui_GrowthDashboard.h"

#include <QtCharts/QCategoryAxis>
#include <QtCharts/QLineSeries>
#include <QtCharts/QSplineSeries>
#include <QtCharts/QValueAxis>
#include <QVBoxLayout>

// Qt6 charts namespace handling
namespace QtCharts {}
using namespace QtCharts;

GrowthDashboard::GrowthDashboard(rove::GrowthVisualizer& visualizer, QWidget* parent)
    : QWidget(parent), ui(std::make_unique<Ui::GrowthDashboard>()), m_visualizer(visualizer) {
    ui->setupUi(this);
    m_radarView = new QChartView(new QChart(), this);
    ui->radarLayout->addWidget(m_radarView);

    m_lineView = new QChartView(new QChart(), this);
    ui->timelineLayout->addWidget(m_lineView);
}

GrowthDashboard::~GrowthDashboard() = default;

void GrowthDashboard::render(const rove::data::User& user,
                             const std::vector<rove::data::GrowthSnapshot>& snapshots) {
    updateRadar(user.attributes());
    buildTimeline(snapshots);
}

/**
 * @brief 绘制成长时间线，使用折线图展示成长值趋势。
 */
void GrowthDashboard::buildTimeline(const std::vector<rove::data::GrowthSnapshot>& snapshots) {
    auto* chart = new QChart();
    auto* series = new QLineSeries(chart);
    int index = 0;
    for (const auto& snap : snapshots) {
        series->append(index++, snap.growthPoints());
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
    angular->append(QStringLiteral("行动"), 1);
    angular->append(QStringLiteral("毅力"), 2);
    angular->append(QStringLiteral("决断"), 3);
    angular->append(QStringLiteral("知识"), 4);
    angular->append(QStringLiteral("社交"), 5);
    angular->append(QStringLiteral("自豪"), 6);
    chart->addAxis(angular, QPolarChart::PolarOrientationAngular);

    auto* series = new QSplineSeries(chart);
    series->append(1, attrs.execution);
    series->append(2, attrs.perseverance);
    series->append(3, attrs.decision);
    series->append(4, attrs.knowledge);
    series->append(5, attrs.social);
    series->append(6, attrs.pride);
    chart->addSeries(series);
    series->attachAxis(radial);
    series->attachAxis(angular);

    m_radarView->setChart(chart);
}



