#include "LogEntry.h"

namespace rove::data {

LogEntry::LogEntry()
    : m_id(-1),
      m_timestamp(QDateTime::currentDateTime()),
      m_type(LogType::Auto),
      m_content(),
      m_relatedId(),
      m_attributeChanges(),
      m_levelChange(0),
      m_specialEvent(),
      m_mood() {}

LogEntry::LogEntry(int id,
                   const QDateTime& timestamp,
                   LogType type,
                   std::string content,
                   std::optional<int> relatedId,
                   std::vector<AttributeChange> attributeChanges,
                   int levelChange,
                   std::string specialEvent,
                   std::optional<MoodTag> mood)
    : m_id(id),
      m_timestamp(timestamp),
      m_type(type),
      m_content(std::move(content)),
      m_relatedId(std::move(relatedId)),
      m_attributeChanges(std::move(attributeChanges)),
      m_levelChange(levelChange),
      m_specialEvent(std::move(specialEvent)),
      m_mood(std::move(mood)) {}

int LogEntry::id() const noexcept { return m_id; }

void LogEntry::setId(int id) noexcept {
    // 仅允许在初次持久化后写入数据库生成的 ID，防止外部修改已存在记录。
    if (m_id < 0) {
        m_id = id;
    }
}

const QDateTime& LogEntry::timestamp() const noexcept { return m_timestamp; }

void LogEntry::setTimestamp(const QDateTime& timestamp) noexcept { m_timestamp = timestamp; }

LogEntry::LogType LogEntry::type() const noexcept { return m_type; }

void LogEntry::setType(LogType type) noexcept { m_type = type; }

const std::string& LogEntry::content() const noexcept { return m_content; }

void LogEntry::setContent(std::string content) { m_content = std::move(content); }

const std::optional<int>& LogEntry::relatedId() const noexcept { return m_relatedId; }

void LogEntry::setRelatedId(const std::optional<int>& relatedId) noexcept { m_relatedId = relatedId; }

const std::vector<LogEntry::AttributeChange>& LogEntry::attributeChanges() const noexcept {
    return m_attributeChanges;
}

void LogEntry::setAttributeChanges(const std::vector<AttributeChange>& changes) { m_attributeChanges = changes; }

int LogEntry::levelChange() const noexcept { return m_levelChange; }

void LogEntry::setLevelChange(int change) noexcept { m_levelChange = change; }

const std::string& LogEntry::specialEvent() const noexcept { return m_specialEvent; }

void LogEntry::setSpecialEvent(std::string specialEvent) { m_specialEvent = std::move(specialEvent); }

const std::optional<LogEntry::MoodTag>& LogEntry::mood() const noexcept { return m_mood; }

void LogEntry::setMood(const std::optional<MoodTag>& mood) noexcept { m_mood = mood; }

std::string LogEntry::moodToEmoji(LogEntry::MoodTag mood) {
    switch (mood) {
        case MoodTag::Happy:
            return "😊";
        case MoodTag::Neutral:
            return "😐";
        case MoodTag::Sad:
            return "😔";
    }
    return "😐";  // 未知值时回退到中立，保持 UI 一致性。
}

std::string LogEntry::typeToString(LogEntry::LogType type) {
    switch (type) {
        case LogType::Auto:
            return "Auto";
        case LogType::Manual:
            return "Manual";
        case LogType::Milestone:
            return "Milestone";
        case LogType::Event:
            return "Event";
    }
    return "Auto";  // 默认回退到自动日志，保持兼容性。
}

LogEntry::LogType LogEntry::typeFromString(const std::string& text) {
    if (text == "Manual") {
        return LogType::Manual;
    }
    if (text == "Milestone") {
        return LogType::Milestone;
    }
    if (text == "Event") {
        return LogType::Event;
    }
    return LogType::Auto;
}

}  // namespace rove::data
