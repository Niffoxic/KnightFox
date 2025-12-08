#pragma once

#include "EngineAPI.h"

#include <string>
#include <unordered_map>
#include <sstream>
#include "file_system.h"

class KFE_API JsonLoader
{
public:
    using ObjectType = std::unordered_map<std::string, JsonLoader>;

public:
    JsonLoader() = default;
    ~JsonLoader() = default;

    void Load(const std::string& filePath);
    void Save(const std::string& filePath);

    JsonLoader& operator=(const std::string& value) noexcept;

    const JsonLoader& operator[](const std::string& key) const noexcept;

    JsonLoader& operator[](const std::string& key) noexcept
    {
        return GetOrCreate(key);
    }

    JsonLoader(JsonLoader&&) = default;
    JsonLoader& operator=(JsonLoader&&) = default;

    JsonLoader(const JsonLoader&) = default;
    JsonLoader& operator=(const JsonLoader&) = default;

    JsonLoader& GetOrCreate(const std::string& key);

    auto begin()        noexcept { return m_children.begin(); }
    auto end()          noexcept { return m_children.end(); }
    auto begin()  const noexcept { return m_children.begin(); }
    auto end()    const noexcept { return m_children.end(); }

    const std::string& GetValue() const noexcept { return m_value; }
    void               SetValue(const std::string& val) { m_value = val; }

    [[nodiscard]] bool Contains(const std::string& key) const noexcept;

    std::string ToFormattedString(int indent = 0) const;
    void        FromStream(std::istream& input);

    // Typed access with optional defaults
    [[nodiscard]] float AsFloat(float defaultValue = 0.0f) const noexcept;
    [[nodiscard]] int   AsInt(int   defaultValue = 0)     const noexcept;
    [[nodiscard]] bool  AsBool(bool  defaultValue = false) const noexcept;

    [[nodiscard]] bool IsValid() const noexcept;
    void Clear() noexcept;

    // Object vs leaf convenience
    [[nodiscard]] bool IsObject() const noexcept { return !m_children.empty(); }
    [[nodiscard]] bool IsLeaf()   const noexcept { return m_children.empty(); }

private:
    void        Serialize(std::ostream& output, int indent) const;
    void        ParseObject(std::istream& input);
    static void SkipWhitespace(std::istream& input);
    static bool ConsumeChar(std::istream& input, char expected);
    static std::string ReadQuotedString(std::istream& input);
    static std::string EscapeString(const std::string& s);

private:
    std::string m_value;
    ObjectType  m_children;

    KFEFileSystem m_fileSystem{};
};
