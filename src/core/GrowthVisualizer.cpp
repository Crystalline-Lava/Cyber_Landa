#include "GrowthVisualizer.h"

#include <QtCharts/QCategoryAxis>
#include <QtCharts/QSplineSeries>

#include <sstream>
#include <limits>
#include <cmath>

namespace rove::data {

GrowthVisualizer& GrowthVisualizer::instance() {
    static GrowthVisualizer instance;
    return instance;
}

std::unique_ptr<QtCharts::QChart> GrowthVisualizer::buildRadarChart(const GrowthSnapshot& snapshot) const {
    auto chart = std::make_unique<QtCharts::QPolarChart>();
    chart->setTitle(QStringLiteral("六维属性雷达图（宽恕后展示）"));

    auto axisAngular = new QtCharts::QCategoryAxis(chart.get());
    axisAngular->setLabelsPosition(QtCharts::QCategoryAxis::AxisLabelsPositionOnValue);

    auto axisRadial = new QtCharts::QValueAxis(chart.get());
    axisRadial->setRange(0, snapshot.attributes().totalPoints() + 10);
    axisRadial->setLabelFormat("%d");

    auto series = new QtCharts::QSplineSeries(chart.get());
    series->setName(QStringLiteral("属性分布"));

    std::vector<std::pair<QString, int>> points = {
        {QStringLiteral("执行力"), snapshot.attributes().execution},
        {QStringLiteral("毅力"), snapshot.attributes().perseverance},
        {QStringLiteral("决断力"), snapshot.attributes().decision},
        {QStringLiteral("知识力"), snapshot.attributes().knowledge},
        {QStringLiteral("社交力"), snapshot.attributes().social},
        {QStringLiteral("自豪感"), snapshot.attributes().pride},
    };

    const double step = 360.0 / static_cast<double>(points.size());
    for (int i = 0; i < static_cast<int>(points.size()); ++i) {
        double angle = i * step;
        axisAngular->append(points[i].first, angle);
        series->append(angle, points[i].second);
    }
    // 闭合雷达图形状
    series->append(360.0, points.front().second);

    chart->addSeries(series);
    chart->addAxis(axisAngular, Qt::AlignLeft);
    chart->addAxis(axisRadial, Qt::AlignLeft);
    series->attachAxis(axisAngular);
    series->attachAxis(axisRadial);

    return chart;
}

std::unique_ptr<QtCharts::QChart> GrowthVisualizer::buildGrowthLineChart(
    const std::vector<GrowthSnapshot>& snapshots,
    const std::vector<LogEntry>& milestones,
    const std::set<int>& forgivenIds) const {
    auto chart = std::make_unique<QtCharts::QChart>();
    chart->setTitle(QStringLiteral("等级与成长值曲线"));

    auto levelSeries = buildLevelSeries(snapshots);
    auto growthSeries = buildGrowthSeries(snapshots);
    auto milestoneSeries = buildMilestoneSeries(milestones, snapshots, forgivenIds);

    chart->addSeries(levelSeries);
    chart->addSeries(growthSeries);
    chart->addSeries(milestoneSeries);

    auto axisX = new QtCharts::QValueAxis(chart.get());
    axisX->setLabelFormat("%i");
    axisX->setTitleText(QStringLiteral("时间序号"));

    auto axisY = new QtCharts::QValueAxis(chart.get());
    axisY->setLabelFormat("%i");
    axisY->setTitleText(QStringLiteral("数值"));

    chart->setAxisX(axisX, levelSeries);
    chart->setAxisX(axisX, growthSeries);
    chart->setAxisX(axisX, milestoneSeries);
    chart->setAxisY(axisY, levelSeries);
    chart->setAxisY(axisY, growthSeries);
    chart->setAxisY(axisY, milestoneSeries);

    return chart;
}

QString GrowthVisualizer::exportCsv(const std::vector<GrowthSnapshot>& snapshots) const {
    std::ostringstream oss;
    oss << "timestamp,level,growth,execution,perseverance,decision,knowledge,social,pride,achievements,completed,failed,manual_logs\n";
    for (const auto& snapshot : snapshots) {
        oss << snapshot.timestamp().toString(Qt::ISODate).toStdString() << ',' << snapshot.level() << ','
            << snapshot.growthPoints() << ',' << snapshot.attributes().execution << ','
            << snapshot.attributes().perseverance << ',' << snapshot.attributes().decision << ','
            << snapshot.attributes().knowledge << ',' << snapshot.attributes().social << ','
            << snapshot.attributes().pride << ',' << snapshot.achievementCount() << ','
            << snapshot.completedTasks() << ',' << snapshot.failedTasks() << ',' << snapshot.manualLogCount() << "\n";
    }
    return QString::fromStdString(oss.str());
}

QtCharts::QLineSeries* GrowthVisualizer::buildLevelSeries(const std::vector<GrowthSnapshot>& snapshots) const {
    auto series = new QtCharts::QLineSeries();
    series->setName(QStringLiteral("等级"));
    for (int i = 0; i < static_cast<int>(snapshots.size()); ++i) {
        series->append(i, snapshots[i].level());
    }
    return series;
}

QtCharts::QLineSeries* GrowthVisualizer::buildGrowthSeries(const std::vector<GrowthSnapshot>& snapshots) const {
    auto series = new QtCharts::QLineSeries();
    series->setName(QStringLiteral("成长值"));
    for (int i = 0; i < static_cast<int>(snapshots.size()); ++i) {
        series->append(i, snapshots[i].growthPoints());
    }
    return series;
}

QtCharts::QScatterSeries* GrowthVisualizer::buildMilestoneSeries(const std::vector<LogEntry>& milestones,
                                                                 const std::vector<GrowthSnapshot>& snapshots,
                                                                 const std::set<int>& forgivenIds) const {
    auto series = new QtCharts::QScatterSeries();
    series->setName(QStringLiteral("里程碑"));
    series->setMarkerSize(10.0);

    for (const auto& log : milestones) {
        if (forgivenIds.count(log.id()) > 0) {
            continue;  // 宽恕券隐藏负面记录
        }
        int closestIndex = 0;
        qint64 minDiff = std::numeric_limits<qint64>::max();
        for (int i = 0; i < static_cast<int>(snapshots.size()); ++i) {
            qint64 diff = std::abs(log.timestamp().msecsTo(snapshots[i].timestamp()));
            if (diff < minDiff) {
                minDiff = diff;
                closestIndex = i;
            }
        }
        series->append(closestIndex, snapshots.empty() ? 0 : snapshots[closestIndex].growthPoints());
    }
    return series;
}

}  // namespace rove::data
