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

// 使用 Qt Charts 命名空间
namespace QtCharts {}
using namespace QtCharts;

namespace rove {

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
    [[nodiscard]] std::unique_ptr<QChart> buildRadarChart(const data::GrowthSnapshot& snapshot) const;

    /**
     * @brief 绘制等级与成长值折线图，同时标记里程碑与宽恕隐藏点。
     */
    [[nodiscard]] std::unique_ptr<QChart> buildGrowthLineChart(
        const std::vector<data::GrowthSnapshot>& snapshots,
        const std::vector<data::LogEntry>& milestones,
        const std::set<int>& forgivenIds) const;

    /**
     * @brief 导出可视化使用的数据为 CSV 字符串，便于教师留档。
     */
    [[nodiscard]] QString exportCsv(const std::vector<data::GrowthSnapshot>& snapshots) const;

private:
    GrowthVisualizer() = default;
    /**
     * @brief 构建等级（Level）随时间变化的折线序列。
     * @param snapshots 成长快照序列，按时间排序。
     * @return QLineSeries* 指向等级折线序列的指针，需由调用者管理其生命周期。
     */
    QLineSeries* buildLevelSeries(const std::vector<data::GrowthSnapshot>& snapshots) const;

    /**
     * @brief 构建成长值（Growth）随时间变化的折线序列。
     * @param snapshots 成长快照序列，按时间排序。
     * @return QLineSeries* 指向成长值折线序列的指针，需由调用者管理其生命周期。
     */
    QLineSeries* buildGrowthSeries(const std::vector<data::GrowthSnapshot>& snapshots) const;

    /**
     * @brief 构建里程碑与宽恕点的散点序列，用于在成长折线图中标记特殊事件。
     * @param milestones 里程碑日志条目集合。
     * @param snapshots 成长快照序列，按时间排序。
     * @param forgivenIds 被宽恕隐藏的里程碑 ID 集合。
     * @return QScatterSeries* 指向散点序列的指针，需由调用者管理其生命周期。
     */
    QScatterSeries* buildMilestoneSeries(const std::vector<data::LogEntry>& milestones,
                                                   const std::vector<data::GrowthSnapshot>& snapshots,
                                                   const std::set<int>& forgivenIds) const;
};

}  // namespace rove::data

#endif  // GROWTHVISUALIZER_H

