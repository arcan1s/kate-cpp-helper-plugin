/**
 * \file
 *
 * \brief Class \c kate::SearchResultsTableModel (implementation)
 *
 * \date Mon Oct 28 09:34:39 MSK 2013 -- Initial design
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
#include <src/search_results_table_model.h>
#include <src/database_manager.h>

// Standard includes
#include <KDE/KLocalizedString>

namespace kate {

SearchResultsTableModel::SearchResultsTableModel(DatabaseManager& db_mgr)
  : QAbstractTableModel(nullptr)
  , m_db_mgr(db_mgr)
{
}

}                                                           // namespace kate
