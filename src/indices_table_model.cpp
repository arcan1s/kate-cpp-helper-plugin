/**
 * \file
 *
 * \brief Class \c kate::IndicesTableModel (implementation)
 *
 * \date Tue Oct 22 16:08:11 MSK 2013 -- Initial design
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

// Project specific includes
#include <src/indices_table_model.h>
#include <src/database_manager.h>

// Standard includes
#include <KDE/KLocalizedString>
#include <cassert>

namespace kate {

IndicesTableModel::IndicesTableModel(const std::weak_ptr<DatabaseManager>& db_mgr)
  : QAbstractTableModel(nullptr)
  , m_db_mgr(db_mgr)
{
}

int IndicesTableModel::rowCount(const QModelIndex&) const
{
    const auto& dm = m_db_mgr.lock();
    assert("Sanity check" && dm);
    return dm->m_collections.size();
}

int IndicesTableModel::columnCount(const QModelIndex&) const
{
    return column::last__;
}

QVariant IndicesTableModel::data(const QModelIndex& index, const int role) const
{
    const auto& dm = m_db_mgr.lock();
    assert("Sanity check" && dm);
    switch (role)
    {
        case Qt::DisplayRole:
            switch (index.column())
            {
                case column::NAME:
                    return dm->m_collections[index.row()].m_options->name();
                default:
                    break;
            }
        case Qt::CheckStateRole:
            switch (index.column())
            {
                case column::NAME:
                    return dm->isEnabled(index.row()) ? Qt::Checked : Qt::Unchecked;
                default:
                    break;
            }
        default:
            break;
    }
    return QVariant{};
}

#if 0
QVariant IndicesTableModel::headerData(
    const int section
  , const Qt::Orientation orientation
  , const int role
  ) const
{
    if (role == Qt::DisplayRole)
    {
        if (orientation == Qt::Horizontal)
        {
            switch (section)
            {
                case column::NAME:
                    return QString{i18n("Name")};
                default:
                    break;
            }
        }
    }
    return QVariant{};
}
#endif

Qt::ItemFlags IndicesTableModel::flags(const QModelIndex& index) const
{
    auto result = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    if (index.column() == column::NAME)
        result |= Qt::ItemIsEditable | Qt::ItemIsUserCheckable;
    return result;
}

bool IndicesTableModel::setData(const QModelIndex& index, const QVariant& value, const int role)
{
    const auto& dm = m_db_mgr.lock();
    assert("Sanity check" && dm);
    if (role == Qt::EditRole)
    {
        // Save value from editor
        auto name = value.toString();
        if (name.isEmpty())
            name = "<No name>";
        dm->m_collections[index.row()].m_options->setName(name);
    }
    else if (role == Qt::CheckStateRole)
    {
        auto flag = value.toBool();
        dm->enable(index.row(), flag);
    }
    return true;
}

}                                                           // namespace kate
