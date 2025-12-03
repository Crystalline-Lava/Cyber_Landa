#ifndef GROWTHVISUALIZER_H
#define GROWTHVISUALIZER_H

#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QPolarChart>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QValueAxis>

#include <memory>
#include <set>
#include <vector>

#include "GrowthSnapshot.h"
#include "LogEntry.h"

namespace rove::data {

/**
 * @class GrowthVisualizer
 * @brief 成长可视化组件单例，负责绘制雷达图、折线图并提供数据导出与宽恕券美化。
 */
class GrowthVisualizer {
public:
    static GrowthVisualizer& instance();

    /**
     * @brief 绘制六维属性雷达图，采用 QPolarChart 美化展示。
     */
    [[nodiscard]] std::unique_ptr<QtCharts::QChart> buildRadarChart(const GrowthSnapshot& snapshot) const;

    /**
     * @brief 绘制等级与成长值折线图，同时标记里程碑与宽恕隐藏点。
     */
    [[nodiscard]] std::unique_ptr<QtCharts::QChart> buildGrowthLineChart(
        const std::vector<GrowthSnapshot>& snapshots,
        const std::vector<LogEntry>& milestones,
        const std::set<int>& forgivenIds) const;

    /**
     * @brief 导出可视化使用的数据为 CSV 字符串，便于教师留档。
     */
    [[nodiscard]] QString exportCsv(const std::vector<GrowthSnapshot>& snapshots) const;

private:
    GrowthVisualizer() = default;
    QtCharts::QLineSeries* buildLevelSeries(const std::vector<GrowthSnapshot>& snapshots) const;
    QtCharts::QLineSeries* buildGrowthSeries(const std::vector<GrowthSnapshot>& snapshots) const;
    QtCharts::QScatterSeries* buildMilestoneSeries(const std::vector<LogEntry>& milestones,
                                                   const std::vector<GrowthSnapshot>& snapshots,
                                                   const std::set<int>& forgivenIds) const;
};

}  // namespace rove::data

#endif  // GROWTHVISUALIZER_H
