#ifndef LOGENTRY_H
#define LOGENTRY_H

#include <QDateTime>

#include <optional>
#include <string>
#include <vector>

namespace rove::data {

/**
 * @class LogEntry
 * @brief 日志数据模型，封装系统或学生手动添加的成长记录。
 * 中文：LogEntry 提供日志类型、时间戳、关联任务/成就、属性变化和心情标签等字段，
 *       方便日志管理器按需过滤与持久化，遵循“不可修改、不可删除”的原则。
 */
class LogEntry {
public:
    /**
     * @enum LogType
     * @brief 枚举日志类型，区分自动记录、手动记录、里程碑与特殊事件。
     */
    enum class LogType { Auto, Manual, Milestone, Event };

    /**
     * @enum MoodTag
     * @brief 心情标签，仅用于手动日志以便后续情绪过滤。
     */
    enum class MoodTag { Happy, Neutral, Sad };

    /**
     * @struct AttributeChange
     * @brief 描述单项属性的变化量，便于可视化展示。
     */
    struct AttributeChange {
        std::string name;
        int delta = 0;
    };

    /**
     * @brief 默认构造函数，保持安全初始状态。
     */
    LogEntry();

    /**
     * @brief 完整构造函数，一次性填充所有核心字段。
     */
    LogEntry(int id,
             const QDateTime& timestamp,
             LogType type,
             std::string content,
             std::optional<int> relatedId,
             std::vector<AttributeChange> attributeChanges,
             int levelChange,
             std::string specialEvent,
             std::optional<MoodTag> mood);

    [[nodiscard]] int id() const noexcept;
    void setId(int id) noexcept;

    [[nodiscard]] const QDateTime& timestamp() const noexcept;
    void setTimestamp(const QDateTime& timestamp) noexcept;

    [[nodiscard]] LogType type() const noexcept;
    void setType(LogType type) noexcept;

    [[nodiscard]] const std::string& content() const noexcept;
    void setContent(std::string content);

    [[nodiscard]] const std::optional<int>& relatedId() const noexcept;
    void setRelatedId(const std::optional<int>& relatedId) noexcept;

    [[nodiscard]] const std::vector<AttributeChange>& attributeChanges() const noexcept;
    void setAttributeChanges(const std::vector<AttributeChange>& changes);

    [[nodiscard]] int levelChange() const noexcept;
    void setLevelChange(int change) noexcept;

    [[nodiscard]] const std::string& specialEvent() const noexcept;
    void setSpecialEvent(std::string specialEvent);

    [[nodiscard]] const std::optional<MoodTag>& mood() const noexcept;
    void setMood(const std::optional<MoodTag>& mood) noexcept;

    /**
     * @brief 将心情枚举映射为 emoji 字符串，方便 UI 直接显示。
     */
    [[nodiscard]] static std::string moodToEmoji(MoodTag mood);

    /**
     * @brief 将日志类型转换为字符串，便于存储和过滤。
     */
    [[nodiscard]] static std::string typeToString(LogType type);

    /**
     * @brief 从字符串解析日志类型，兼容数据库字段。
     */
    [[nodiscard]] static LogType typeFromString(const std::string& text);

private:
    int m_id;
    QDateTime m_timestamp;
    LogType m_type;
    std::string m_content;
    std::optional<int> m_relatedId;
    std::vector<AttributeChange> m_attributeChanges;
    int m_levelChange;
    std::string m_specialEvent;
    std::optional<MoodTag> m_mood;
};

}  // namespace rove::data

#endif  // LOGENTRY_H
