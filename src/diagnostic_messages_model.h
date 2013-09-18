/**
 * \file
 *
 * \brief Class \c kate::DiagnosticMessagesModel (interface)
 *
 * \date Sun Aug 11 04:42:02 MSK 2013 -- Initial design
 */
/*
 * Copyright (C) 2011-2013 Alex Turbov, all rights reserved.
 * This is free software. It is licensed for use, modification and
 * redistribution under the terms of the GNU General Public License,
 * version 3 or later <http://gnu.org/licenses/gpl.html>
 *
 * KateCppHelperPlugin is free software: you can redistribute it and/or modify it
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

#ifndef __SRC__DIAGNOSTIC_MESSAGES_MODEL_HH__
# define __SRC__DIAGNOSTIC_MESSAGES_MODEL_HH__

// Project specific includes

// Standard includes
# include <KUrl>
# include <QAbstractListModel>
# include <deque>
# include <tuple>

namespace kate {

/**
 * \brief A mode class to hold diagnostic messages
 *
 * Data from this class will be displayed in the <em>Diagnostic Messages</em>
 * tab of the plugin sidebar.
 *
 */
class DiagnosticMessagesModel : public QAbstractListModel
{
    Q_OBJECT

public:
    /// Structure to hold info about diagnostic message
    struct Record
    {
        enum class type
        {
            debug
          , info
          , warning
          , error
          , cutset
        };
        QString m_text;                                     ///< Diagnostic message text
        /// \name Location in source code
        //@{
        QString m_file;                                     ///< Name of a source file
        unsigned m_line;                                    ///< Line number
        unsigned m_column;                                  ///< Position on the line
        //@}
        type m_type;                                        ///< Type of the record

        /// \c record class must be default constructible
        Record() = default;
        /// Make a \c record from parts
        Record(QString&&, unsigned, unsigned, QString&&, type) noexcept;
        /// Move ctor
        Record(Record&&) noexcept;
        /// Move-assign operator
        Record& operator=(Record&&) noexcept;
    };

    /// Default constructor
    DiagnosticMessagesModel() = default;

    std::tuple<KUrl, unsigned, unsigned> getLocationByIndex(const QModelIndex&) const;

    /// \name QAbstractTableModel interface
    //@{
    /// Get rows count
    int rowCount(const QModelIndex& = QModelIndex()) const;
    /// Get columns count
    int columnCount(const QModelIndex& = QModelIndex()) const;
    /// Get data for given index and role
    QVariant data(const QModelIndex&, int = Qt::DisplayRole) const;
    //@}

    /// \name Records manipulation
    //@{
    /// Append a diagnostic record to the model
    void append(QString&&, unsigned, unsigned, QString&&, Record::type);
    /// Append a diagnostic record to the model (bulk version)
    template <typename Iter>
    void append(Iter, Iter);

public Q_SLOTS:
    void clear()
    {
        m_records.clear();
    }
    //@}

private:
    std::deque<Record> m_records;                           ///< Stored records
};

inline DiagnosticMessagesModel::Record::Record(
    QString&& file
  , const unsigned line
  , const unsigned column
  , QString&& text
  , const Record::type type
  ) noexcept
  : m_line(line)
  , m_column(column)
  , m_type(type)
{
    m_text.swap(text);
    m_file.swap(file);
}

inline DiagnosticMessagesModel::Record::Record(Record&& other) noexcept
  : m_line(other.m_line)
  , m_column(other.m_column)
  , m_type(other.m_type)
{
    if (&other != this)
    {
        m_text.swap(other.m_text);
        m_file.swap(other.m_file);
    }
}

inline auto DiagnosticMessagesModel::Record::operator=(Record&& other) noexcept -> Record&
{
    m_line = other.m_line;
    m_column = other.m_column;
    m_type = other.m_type;
    if (&other != this)
    {
        m_text.swap(other.m_text);
        m_file.swap(other.m_file);
    }
    return *this;
}

inline void DiagnosticMessagesModel::append(
    QString&& file
  , const unsigned line
  , const unsigned column
  , QString&& text
  , const Record::type type
  )
{
    beginInsertRows(QModelIndex(), m_records.size(), m_records.size());
    m_records.emplace_back(std::move(file), line, column, std::move(text), type);
    endInsertRows();
}

template <typename Iter>
inline void DiagnosticMessagesModel::append(Iter first, Iter last)
{
    beginInsertRows(
        QModelIndex()
      , m_records.size()
      , m_records.size() + std::distance(first, last) - 1
      );
    m_records.insert(end(m_records), first, last);
    endInsertRows();
}

}                                                           // namespace kate
#endif                                                      // __SRC__DIAGNOSTIC_MESSAGES_MODEL_HH__