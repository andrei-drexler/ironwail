#include "quakedef.h"
#include "sidebar.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

class CSidebar
{
public:
    CSidebar();
    ~CSidebar();

    void Setup();
    void Shutdown();
    void Clear();
    void AddCount(const char *path, int count);
    const char *GetFile(const char *path) const;
    bool HasItems() const;
    const sidebar_menu_t *GetMenu() const;

    void Sidebar_Shutdown();
    void Sidebar_Setup();
    void Sidebar_Clear();
    void Sidebar_AddCount(const char *path, int count);
    const char *Sidebar_GetFile(const char *path) const;
    bool Sidebar_HasItems() const;
    const sidebar_menu_t *Sidebar_GetMenu() const;

private:
    struct entry_t
    {
        std::string key;
        int count = 0;
    };

    void MarkMenuDirty() const;

    mutable sidebar_menu_t menu_;
    mutable std::vector<const char *> labels_;
    mutable std::vector<int> counts_;
    mutable bool menu_dirty_ = true;
    mutable bool entries_dirty_ = true;

    std::vector<entry_t> entries_;
    mutable std::unordered_map<std::string, size_t> index_;
};

CSidebar::CSidebar()
{
    menu_.count = 0;
    menu_.labels = nullptr;
    menu_.counts = nullptr;
}

CSidebar::~CSidebar() = default;

void CSidebar::MarkMenuDirty() const
{
    menu_dirty_ = true;
}

void CSidebar::Setup()
{
    if (!entries_dirty_)
        return;

    if (entries_.empty())
    {
        index_.clear();
        labels_.clear();
        counts_.clear();
        menu_.count = 0;
        menu_.labels = nullptr;
        menu_.counts = nullptr;
        menu_dirty_ = false;
        entries_dirty_ = false;
        return;
    }

    std::stable_sort(entries_.begin(), entries_.end(), [](const entry_t &lhs, const entry_t &rhs) {
        if (lhs.count != rhs.count)
            return lhs.count > rhs.count;
        return q_strcasecmp(lhs.key.c_str(), rhs.key.c_str()) < 0;
    });

    index_.clear();
    index_.reserve(entries_.size());
    for (size_t i = 0; i < entries_.size(); ++i)
        index_[entries_[i].key] = i;

    entries_dirty_ = false;
    MarkMenuDirty();
}

void CSidebar::Shutdown()
{
    Clear();
}

void CSidebar::Clear()
{
    entries_.clear();
    index_.clear();
    labels_.clear();
    counts_.clear();
    menu_.count = 0;
    menu_.labels = nullptr;
    menu_.counts = nullptr;
    menu_dirty_ = false;
    entries_dirty_ = false;
}

void CSidebar::AddCount(const char *path, int count)
{
    if (!path || !*path || count <= 0)
        return;

    auto it = index_.find(path);
    if (it != index_.end())
    {
        entries_[it->second].count += count;
    }
    else
    {
        entries_.push_back({std::string(path), count});
        index_[entries_.back().key] = entries_.size() - 1;
    }

    entries_dirty_ = true;
    MarkMenuDirty();
}

const char *CSidebar::GetFile(const char *path) const
{
    if (!path || !*path)
        return nullptr;

    auto it = index_.find(path);
    if (it == index_.end())
        return nullptr;

    const entry_t &entry = entries_[it->second];
    return entry.key.c_str();
}

bool CSidebar::HasItems() const
{
    return !entries_.empty();
}

const sidebar_menu_t *CSidebar::GetMenu() const
{
    if (entries_dirty_)
        const_cast<CSidebar *>(this)->Setup();

    if (!menu_dirty_)
        return &menu_;

    labels_.clear();
    counts_.clear();
    labels_.reserve(entries_.size());
    counts_.reserve(entries_.size());

    for (const entry_t &entry : entries_)
    {
        labels_.push_back(entry.key.c_str());
        counts_.push_back(entry.count);
    }

    menu_.count = labels_.size();
    menu_.labels = labels_.empty() ? nullptr : labels_.data();
    menu_.counts = counts_.empty() ? nullptr : counts_.data();
    menu_dirty_ = false;
    return &menu_;
}

void CSidebar::Sidebar_Shutdown()
{
    Shutdown();
}

void CSidebar::Sidebar_Setup()
{
    Setup();
}

void CSidebar::Sidebar_Clear()
{
    Clear();
}

void CSidebar::Sidebar_AddCount(const char *path, int count)
{
    AddCount(path, count);
}

const char *CSidebar::Sidebar_GetFile(const char *path) const
{
    return GetFile(path);
}

bool CSidebar::Sidebar_HasItems() const
{
    return HasItems();
}

const sidebar_menu_t *CSidebar::Sidebar_GetMenu() const
{
    return GetMenu();
}

static CSidebar g_sidebar;

extern "C" {

void Sidebar_Init(void)
{
    g_sidebar.Clear();
}

void Sidebar_Shutdown(void)
{
    g_sidebar.Shutdown();
}

void Sidebar_Setup(void)
{
    g_sidebar.Setup();
}

void Sidebar_Clear(void)
{
    g_sidebar.Clear();
}

void Sidebar_AddCount(const char *path, int count)
{
    g_sidebar.AddCount(path, count);
}

const char *Sidebar_GetFile(const char *path)
{
    return g_sidebar.GetFile(path);
}

qboolean Sidebar_HasItems(void)
{
    return g_sidebar.HasItems() ? 1 : 0;
}

const sidebar_menu_t *Sidebar_GetMenu(void)
{
    return g_sidebar.GetMenu();
}

}
