/**
 * \file
 *
 * \brief Class \c kate::utils (implementation)
 *
 * \date Mon Feb  6 04:12:51 MSK 2012 -- Initial design
 */
/*
 * KateIncludeHelperPlugin is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KateIncludeHelperPlugin is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// Project specific includes
#include <src/utils.h>

// Standard includes
#include <KDebug>
#include <cassert>

namespace kate {
/**
 * Return a range w/ filename if given line contains a valid \c #include
 * directive, empty range otherwise.
 *
 * \param line a string w/ line to parse
 * \param strict return failure if closing \c '>' or \c '"' missed
 *
 * \return instance of \c IncludeParseResult
 *
 * \warning Line would always 0 in a range! U have to set it after call this function!
 */
IncludeParseResult parseIncludeDirective(const QString& line, const bool strict)
{
    kDebug() << "text2parse=" << line << ", strict=" << strict;

    enum State {
        skipInitialSpaces
      , foundHash
      , checkInclude
      , skipSpaces
      , foundOpenChar
      , findCloseChar
      , stop
    };

    // Perpare 'default' result
    IncludeParseResult result;
    result.m_type = IncludeStyle::unknown;
    result.m_is_complete = false;

    int start = 0;
    int end = 0;
    int tmp = 0;
    QChar close = 0;
    State state = skipInitialSpaces;
    for (int pos = 0; pos < line.length() && state != stop; ++pos)
    {
        switch (state)
        {
            case skipInitialSpaces:
                if (line[pos] != ' ' && line[pos] != '\t')
                {
                    if (line[pos] == '#')
                    {
                        state = foundHash;
                        continue;
                    }
                    return result;                          // Error: smth other than '#' first char in a line
                }
                break;
            case foundHash:
                if (line[pos] == ' ' || line[pos] == '\t')
                    continue;
                else
                    state = checkInclude;
                // NOTE No `break' here!
            case checkInclude:
                if ("include"[tmp++] != line[pos])
                    return result;                          // Error: not 'include' after '#'
                if (tmp == 7)
                    state = skipSpaces;
                break;
            case skipSpaces:
                if (line[pos] == ' ' || line[pos] == '\t')
                    continue;
                // Check open char type
                if (line[pos] == '<')
                {
                    close = '>';
                    result.m_type = IncludeStyle::global;
                }
                else if (line[pos] == '"')
                {
                    close = '"';
                    result.m_type = IncludeStyle::local;
                }
                else return result;
                state = foundOpenChar;
                break;                                      // NOTE We have to move to next char (if remain smth)
            case foundOpenChar:
                state = findCloseChar;
                start = pos;
                end = pos;
                // NOTE No `break' here!
            case findCloseChar:
                if (line[pos] == close)
                {
                    result.m_is_complete = true;            // Found close char! #include complete...
                    state = stop;
                    end = pos;
                }
                else if (line[pos] == ' ' || line[pos] == '\t')
                {
                    if (strict)
                        return result;                      // in strict mode return false for incomplete #include
                    state = stop;                           // otherwise, it is Ok to have incomplete string...
                    end = pos;
                }
                break;
            case stop:
            default:
                assert(!"Parsing FSM broken!");
        }
    }
    // Check state after EOL occurs
    switch (state)
    {
        case foundOpenChar:
            if (!strict)
                result.m_range = KTextEditor::Range(0, line.length(), 0, line.length() + 1);
            break;
        case findCloseChar:
            if (!strict)
                result.m_range = KTextEditor::Range(0, start, 0, line.length());
            break;
        case stop:
            result.m_range = KTextEditor::Range(0, start, 0, end);
            break;
        case skipInitialSpaces:
        case foundHash:
        case checkInclude:
        case skipSpaces:
            break;
        default:
            assert(!"Parsing FSM broken!");
    }
    return result;
}

}                                                           // namespace kate
