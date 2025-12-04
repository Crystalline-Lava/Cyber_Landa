#include "GrowthVisualizer.h"

#include <QtCharts/QCategoryAxis>
#include <QtCharts/QSplineSeries>

#include <sstream>
#include <limits>
#include <cmath>

namespace QtCharts {}
using namespace QtCharts;

namespace rove {

GrowthVisualizer& GrowthVisualizer::instance() {
    static GrowthVisualizer instance;
    return instance;
}

std::unique_ptr<QChart> GrowthVisualizer::buildRadarChart(const data::GrowthSnapshot& snapshot) const {
    auto chart = std::make_unique<QPolarChart>();
    chart->setTitle(QStringLiteral("六维属性雷达图（宽恕后展示）"));

    auto axisAngular = new QCategoryAxis(chart.get());
    axisAngular->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);

    auto axisRadial = new QValueAxis(chart.get());
    axisRadial->setRange(0, snapshot.attributes().totalPoints() + 10);
    axisRadial->setLabelFormat("%d");

    auto series = new QSplineSeries(chart.get());
    series->setName(QStringLiteral("属性分布"));

    std::vector<std::pair<QString, int>> points = {
        {QStringLiteral("执行力"), snapshot.attributes().execution},
        {QStringLiteral("毅力"), snapshot.attributes().perseverance},
        {QStringLiteral("决断力"), snapshot.attributes().decision},
        {QStringLiteral("知识力"), snapshot.attributes().knowledge},
        {QStringLiteral("社交力"), snapshot.attributes().social},
        {QStringLiteral("自豪感"), snapshot.attributes().pride},
    };

    if (points.empty()) {
        // 没有可用数据时直接返回空图表，避免除零。
        return std::unique_ptr<QChart>(chart.release());
    }

    const double step = 360.0 / static_cast<double>(points.size());
    for (int i = 0; i < static_cast<int>(points.size()); ++i) {
        double angle = i * step;
        axisAngular->append(points[i].first, angle);
        series->append(angle, points[i].second);
    }
    // 闭合雷达图形状
    series->append(360.0, points.front().second);

    chart->addSeries(series);
    chart->addAxis(axisAngular, QPolarChart::PolarOrientationAngular);
    chart->addAxis(axisRadial, QPolarChart::PolarOrientationRadial);
    series->attachAxis(axisAngular);
    series->attachAxis(axisRadial);

    return std::unique_ptr<QChart>(chart.release());
}

std::unique_ptr<QChart> GrowthVisualizer::buildGrowthLineChart(
    const std::vector<data::GrowthSnapshot>& snapshots,
    const std::vector<data::LogEntry>& milestones,
    const std::set<int>& forgivenIds) const {
    auto chart = std::make_unique<QChart>();
    chart->setTitle(QStringLiteral("等级与成长值曲线"));

    auto levelSeries = buildLevelSeries(snapshots);
    auto growthSeries = buildGrowthSeries(snapshots);
    auto milestoneSeries = buildMilestoneSeries(milestones, snapshots, forgivenIds);

    chart->addSeries(levelSeries);
    chart->addSeries(growthSeries);
    chart->addSeries(milestoneSeries);

    auto axisX = new QValueAxis(chart.get());
    axisX->setLabelFormat("%i");
    axisX->setTitleText(QStringLiteral("时间序号"));

    auto axisY = new QValueAxis(chart.get());
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

QString GrowthVisualizer::exportCsv(const std::vector<data::GrowthSnapshot>& snapshots) const {
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

QLineSeries* GrowthVisualizer::buildLevelSeries(const std::vector<data::GrowthSnapshot>& snapshots) const {
    auto series = new QLineSeries();
    series->setName(QStringLiteral("等级"));
    for (int i = 0; i < static_cast<int>(snapshots.size()); ++i) {
        series->append(i, snapshots[i].level());
    }
    return series;
}

QLineSeries* GrowthVisualizer::buildGrowthSeries(const std::vector<data::GrowthSnapshot>& snapshots) const {
    auto series = new QLineSeries();
    series->setName(QStringLiteral("成长值"));
    for (int i = 0; i < static_cast<int>(snapshots.size()); ++i) {
        series->append(i, snapshots[i].growthPoints());
    }
    return series;
}

QScatterSeries* GrowthVisualizer::buildMilestoneSeries(const std::vector<data::LogEntry>& milestones,
                                                                 const std::vector<data::GrowthSnapshot>& snapshots,
                                                                 const std::set<int>& forgivenIds) const {
    auto series = new QScatterSeries();
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
        if (!snapshots.empty()) {
            series->append(closestIndex, snapshots[closestIndex].growthPoints());
        } else {
            series->append(closestIndex, 0);
        }
    }
    return series;
}

}  // namespace rove


