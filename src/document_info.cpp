/**
 * \file
 *
 * \brief Class \c kate::DocumentInfo (implementation)
 *
 * \date Sun Feb 12 06:05:45 MSK 2012 -- Initial design
 */
/*
 *  * KateCppHelperPlugin is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KateCppHelperPlugin is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// Project specific includes
#include <src/document_info.h>
#include <src/cpp_helper_plugin.h>
#include <src/utils.h>

// Standard includes
#include <KDE/KDebug>
#include <KDE/KLocalizedString>
#include <KDE/KTextEditor/MarkInterface>
#include <QtCore/QFileInfo>

namespace kate {

DocumentInfo::DocumentInfo(CppHelperPlugin* p)
  : m_plugin(p)
{
    // Subscribe self to configuration changes
    connect(&m_plugin->config(), SIGNAL(sessionDirsChanged()), this, SLOT(updateStatus()));
    connect(&m_plugin->config(), SIGNAL(systemDirsChanged()), this, SLOT(updateStatus()));
}

/**
 * \bug Why it crashed sometime on exit???
 */
DocumentInfo::~DocumentInfo()
{
    kDebug(DEBUG_AREA) << "Removing " << m_ranges.size() << " ranges...";
    for (const auto& s : m_ranges)
        s.m_range->setFeedback(0);
}

void DocumentInfo::addRange(KTextEditor::MovingRange* range)
{
    assert("Sanity check" && !range->isEmpty() && range->onSingleLine());
    assert("Sanity check" && findRange(range) == m_ranges.end());
    //
    m_ranges.emplace_back(
        std::unique_ptr<KTextEditor::MovingRange>(range)
      , static_cast<KTextEditor::MovingRangeFeedback* const>(this)
      );
    // Subscribe self to range invalidate
    //
    updateStatus(m_ranges.back());
    kDebug(DEBUG_AREA) << "MovingRange registered: " << range;
}

void DocumentInfo::updateStatus()
{
    for (
        auto it = begin(m_ranges)
      , last = end(m_ranges)
      ; it != last
      ; updateStatus(*it++)
      );
}

/**
 * \todo Good idea is to check \c #include directive again and realize what kind of
 * open/close chars are used... depending on this do search for files in all configured
 * paths or session's only...
 */
void DocumentInfo::updateStatus(State& s)
{
    kDebug(DEBUG_AREA) << "Update status for range: " << s.m_range.get();
    if (!s.m_range->isEmpty())
    {
        auto* doc = s.m_range->document();
        auto filename = doc->text(s.m_range->toRange());
        // NOTE After editing it is possible that opening '<' or '"' could
        // appear as a start symbol of the range... just try to exclude it!
        if (filename.startsWith('>') || filename.startsWith('"'))
        {
            filename.remove(0, 1);
            auto shrinked = s.m_range->toRange();
            shrinked.end().setColumn(shrinked.start().column() + 1);
            s.m_range->setRange(shrinked);
        }
        // NOTE after autocompletion it is possible that closing '>' or '"' could
        // appear as the last symbol of the range... just try to exclude it!
        if (filename.endsWith('>') || filename.endsWith('"'))
        {
            filename.resize(filename.size() - 1);
            auto shrinked = s.m_range->toRange();
            shrinked.end().setColumn(shrinked.end().column() - 1);
            s.m_range->setRange(shrinked);
        }
        // Reset status
        s.m_status = Status::Dunno;

        // Check if given header available
        // 0) check CWD first if allowed
        if (m_plugin->config().useCwd())
        {
            const auto& uri = doc->url();
            const auto cur2check = QString{uri.directory() + '/' + filename};
            kDebug(DEBUG_AREA) << "check current dir 4: " << cur2check;
            s.m_status = (QFileInfo{cur2check}.exists()) ? Status::Ok : Status::NotFound;
        }
        // 1) Try configured dirs then
        auto paths = findHeader(
            filename
          , m_plugin->config().sessionDirs()
          , m_plugin->config().systemDirs()
          );
        if (paths.empty())
            s.m_status = (s.m_status == Status::Ok) ? Status::Ok : Status::NotFound;
        else if (paths.size() == 1)
            s.m_status = (s.m_status == Status::Ok) ? Status::MultipleMatches : Status::Ok;
        else
            s.m_status = Status::MultipleMatches;
        kDebug(DEBUG_AREA) << "#include filename=" << filename << ", status=" << int(s.m_status) << ", r=" << s.m_range.get();

        auto* iface = qobject_cast<KTextEditor::MarkInterface*>(doc);
        const auto line = s.m_range->start().line();
        switch (s.m_status)
        {
            case Status::Ok:
                iface->removeMark(
                    line
                  , KTextEditor::MarkInterface::Error | KTextEditor::MarkInterface::Warning
                  );
                break;
            case Status::NotFound:
                iface->removeMark(
                    line
                  , KTextEditor::MarkInterface::Error | KTextEditor::MarkInterface::Warning
                  );
                iface->setMarkPixmap(
                    KTextEditor::MarkInterface::Error
                  , KIcon("task-reject").pixmap(QSize(16, 16))
                  );
                iface->setMarkDescription(KTextEditor::MarkInterface::Error, i18n("File not found"));
                iface->addMark(line, KTextEditor::MarkInterface::Error);
                break;
            case Status::MultipleMatches:
                iface->removeMark(
                    line
                  , KTextEditor::MarkInterface::Error | KTextEditor::MarkInterface::Warning
                  );
                iface->setMarkPixmap(
                    KTextEditor::MarkInterface::Warning
                  , KIcon("task-attention").pixmap(QSize(16, 16))
                  );
                iface->setMarkDescription(KTextEditor::MarkInterface::Error, i18n("Multiple files matched"));
                iface->addMark(line, KTextEditor::MarkInterface::Warning);
                break;
            default:
                assert(!"Impossible");
        }
    }
}

/**
 * \attention Find range by given address
 */
DocumentInfo::registered_ranges_type::iterator DocumentInfo::findRange(KTextEditor::MovingRange* range)
{
    // std::find_if + lambda!
    const auto last = end(m_ranges);
    for (
        auto it = begin(m_ranges)
      ; it != last
      ; ++it
      ) if (range == it->m_range.get()) return it;
    return last;
}

void DocumentInfo::caretExitedRange(KTextEditor::MovingRange* range, KTextEditor::View*)
{
    auto it = findRange(range);
    if (it != end(m_ranges))
        updateStatus(*it);
}

void DocumentInfo::rangeEmpty(KTextEditor::MovingRange* range)
{
    assert(
        "Range must be valid (possible empty, but valid)"
      && range->start().line() != -1 && range->end().line() != -1
      && range->start().column() != -1 && range->end().column() != -1
      );
    // Remove possible mark on a line
    auto* iface = qobject_cast<KTextEditor::MarkInterface*>(range->document());
    iface->clearMark(range->start().line());
    // Erase internal data
    auto it = findRange(range);
    if (it != m_ranges.end())
    {
        kDebug(DEBUG_AREA) << "MovingRange: empty range deleted: " << range;
        it->m_range->setFeedback(0);
        m_ranges.erase(it);
    }
}

/**
 * \note Range invalidation event may occur only if document gets relaoded
 * cuz we've made it w/ \c EmptyAllow option
 */
void DocumentInfo::rangeInvalid(KTextEditor::MovingRange* range)
{
    kDebug(DEBUG_AREA) << "It seems document reloaded... cleanup ranges???";
    // Erase internal data
    auto it = findRange(range);
    if (it != m_ranges.end())
    {
        kDebug(DEBUG_AREA) << "MovingRange: invalid range deleted: " << range;
        it->m_range->setFeedback(0);
        m_ranges.erase(it);
    }
}

std::vector<unsigned> DocumentInfo::getListOfIncludedBy(const unsigned id) const
{
    std::vector<unsigned> result;
    auto p = m_includes.get<include_idx>().equal_range(id);
    if (p.first != m_includes.get<include_idx>().end())
    {
        result.reserve(std::distance(p.first, p.second));
        std::transform(
            p.first
          , p.second
          , std::back_inserter(result)
          , [](const IncludeLocationData& item)
            {
                return item.m_included_by_id;
            }
          );
    }
#if 0
    kDebug(DEBUG_AREA) << "got" << result.size() << "items for header ID" << id;
#endif
    return result;
}

auto DocumentInfo::getListOfIncludedBy2(const unsigned id) const -> std::vector<IncludeLocationData>
{
    std::vector<IncludeLocationData> result;
    auto p = m_includes.get<include_idx>().equal_range(id);
    if (p.first != m_includes.get<include_idx>().end())
    {
        result.reserve(std::distance(p.first, p.second));
        std::copy(p.first, p.second, std::back_inserter(result));
    }
#if 0
    kDebug(DEBUG_AREA) << "got" << result.size() << "items for header ID" << id;
#endif
    return result;
}

std::vector<unsigned> DocumentInfo::getIncludedHeaders(const unsigned id) const
{
    std::vector<unsigned> result;
    auto p = m_includes.get<included_by_idx>().equal_range(id);
    if (p.first != m_includes.get<included_by_idx>().end())
    {
        result.reserve(std::distance(p.first, p.second));
        std::transform(
            p.first
          , p.second
          , std::back_inserter(result)
          , [](const IncludeLocationData& item)
            {
                return item.m_header_id;
            }
          );
    }
#if 0
    kDebug(DEBUG_AREA) << "got" << result.size() << "items for header ID" << id;
#endif
    return result;
}

}                                                           // namespace kate
// kate: hl C++11/Qt4;
