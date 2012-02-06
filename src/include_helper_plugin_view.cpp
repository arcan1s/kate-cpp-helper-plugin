/**
 * \file
 *
 * \brief Class \c kate::include_helper_plugin_view (implementation)
 *
 * \date Mon Feb  6 06:17:32 MSK 2012 -- Initial design
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
#include <src/include_helper_plugin_view.h>
#include <src/include_helper_plugin_completion_model.h>
#include <src/include_helper_plugin.h>
#include <src/utils.h>

// Standard includes
#include <kate/mainwindow.h>
#include <KTextEditor/Document>
#include <KActionCollection>
#include <KLocalizedString>                                 /// \todo Where is \c i18n() defiend?
#include <KPassivePopup>
#include <QApplication>
#include <QClipboard>
#include <QFileInfo>
#include <cassert>

namespace kate {
//BEGIN IncludeHelperPluginView
IncludeHelperPluginView::IncludeHelperPluginView(Kate::MainWindow* mw, const KComponentData& data, IncludeHelperPlugin* plugin)
  : Kate::PluginView(mw)
  , Kate::XMLGUIClient(data)
  , m_plugin(plugin)
  , m_open_header(actionCollection()->addAction("file_open_included_header"))
  , m_copy_include(actionCollection()->addAction("edit_copy_include"))
{
    m_open_header->setText(i18n("Open Header Under Cursor"));
    m_open_header->setShortcut(QKeySequence(Qt::Key_F10));
    connect(m_open_header, SIGNAL(triggered(bool)), this, SLOT(openHeader()));

    m_copy_include->setText(i18n("Copy #include to Clipboard"));
    m_copy_include->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_F10));
    connect(m_copy_include, SIGNAL(triggered(bool)), this, SLOT(copyInclude()));

    // On viewCreated we have to registerCompletionModel...
    connect(mainWindow(), SIGNAL(viewCreated(KTextEditor::View*)), this, SLOT(viewCreated(KTextEditor::View*)));

    // We want to enable/disable open header action depending on
    // mime-type of the current document, so we have to subscribe
    // to view changes...
    connect(mainWindow(), SIGNAL(viewChanged()), this, SLOT(viewChanged()));
    mainWindow()->guiFactory()->addClient(this);
}

IncludeHelperPluginView::~IncludeHelperPluginView() {
    mainWindow()->guiFactory()->removeClient(this);
}

void IncludeHelperPluginView::openHeader()
{
    KTextEditor::Range r = currentWord();
    kDebug() << "currentWord() = " << r;
    if (r.isEmpty())
        return;                                             // Nothing todo if range is empty
    // NOTE Non empty range means that main window is valid
    // and active view present as well!
    assert("Main window and active view expected to be valid!" && mainWindow()->activeView());
    QString filename = mainWindow()->activeView()->document()->text(r);
    assert("Getting text on non-empty range should return smth!" && !filename.isEmpty());
    kDebug() << "filename = " << filename;

    QStringList candidates;
    // Try to find full filename to open
    /// \todo Is there any way to make a joint view for both containers?
    // 0) search session dirs list first
    Q_FOREACH(const QString& dir, m_plugin->sessionDirs())
    {
        const QString uri = dir + "/" + filename;
        kDebug() << "Trying " << uri;
        const QFileInfo fi = QFileInfo(uri);
        if (fi.exists() && fi.isFile() && fi.isReadable())
            candidates.append(uri);
    }
    // 1) search global dirs next
    Q_FOREACH(const QString& dir, m_plugin->globalDirs())
    {
        const QString uri = dir + "/" + filename;
        kDebug() << "Trying " << (dir + "/" + filename);
        const QFileInfo fi = QFileInfo(uri);
        if (fi.exists() && fi.isFile() && fi.isReadable())
            candidates.append(uri);
    }
    //
    kDebug() << "Found candidates: " << candidates;

    // If there is no ambiguity, then just emit a signal to open the file
    if (candidates.size() == 1)
        mainWindow()->openUrl(candidates.first());
    else if (candidates.isEmpty())
        KPassivePopup::message(i18n("Header not found"), filename, qobject_cast<QWidget*>(this));
    else
    {
        QStringList selected = ChooseFromListDialog::select(qobject_cast<QWidget*>(this), candidates);
        Q_FOREACH(const QString& dir, selected)
            mainWindow()->openUrl(dir);
    }
}

void IncludeHelperPluginView::copyInclude()
{
    KTextEditor::View* kv = mainWindow()->activeView();
    if (!kv)
    {
        kDebug() << "no KTextEditor::View";
        return;
    }
    const KUrl& uri = kv->document()->url().prettyUrl();
    QString current_dir = uri.directory();
    QString longest_matched;
    QChar open = m_plugin->useLtGt() ? '<' : '"';
    QChar close = m_plugin->useLtGt() ? '>' : '"';
    kDebug() << "Got document name: " << uri;
    // Try to match local (per session) dirs first
    Q_FOREACH(const QString& dir, m_plugin->sessionDirs())
        if (current_dir.startsWith(dir) && longest_matched.length() < dir.length())
            longest_matched = dir;
    if (longest_matched.isEmpty())
    {
        open = '<';
        close = '>';
        // Try to match global dirs next
        Q_FOREACH(const QString& dir, m_plugin->globalDirs())
            if (current_dir.startsWith(dir) && longest_matched.length() < dir.length())
                longest_matched = dir;
    }
    if (!longest_matched.isEmpty())
    {
        QString text;
        if (isCOrPPSource(kv->document()->mimeType()))
            text = QString("#include %1%2%3")
              .arg(open)
              .arg(current_dir.remove(0, longest_matched.length()) + '/' + uri.fileName())
              .arg(close);
        else text = uri.prettyUrl();
        QApplication::clipboard()->setText(text);
    }
}

void IncludeHelperPluginView::viewChanged()
{
    KTextEditor::View* kv = mainWindow()->activeView();
    if (!kv)
    {
        kDebug() << "no KTextEditor::View -- leave `open header' action as is...";
        return;
    }
    const QString& mime_str = kv->document()->mimeType();
    kDebug() << "Current document type: " << mime_str;
    const bool enable_open_header_action = isCOrPPSource(mime_str);
    m_open_header->setEnabled(enable_open_header_action);
    if (enable_open_header_action)
        m_copy_include->setText(i18n("Copy #include to Clipboard"));
    else
        m_copy_include->setText(i18n("Copy File URI to Clipboard"));
}

void IncludeHelperPluginView::viewCreated(KTextEditor::View* view)
{
    if (isCOrPPSource(view->document()->mimeType()))
    {
        kDebug() << "C/C++ source: register completer";
        KTextEditor::CodeCompletionInterface* cc_iface =
            qobject_cast<KTextEditor::CodeCompletionInterface*>(view);
        if (cc_iface)
        {
            // Enable completions
            cc_iface->registerCompletionModel(new IncludeHelperPluginCompletionModel(view, m_plugin));
            cc_iface->setAutomaticInvocationEnabled(true);
        }
    }
}

KTextEditor::Range IncludeHelperPluginView::currentWord() const
{
    KTextEditor::Range result;                              // default range is empty
    KTextEditor::View* kv = mainWindow()->activeView();
    if (!kv)
    {
        kDebug() << "no KTextEditor::View";
        return result;
    }

    // Return selected text if any
    if (kv->selection())
        return kv->selectionRange();

    if (!kv->cursorPosition().isValid())
    {
        kDebug() << "cursor not valid!";
        return result;
    }

    // Obtain a line under cursor
    int line = kv->cursorPosition().line();
    QString line_str = kv->document()->line(line);

    // Check if current line starts w/ #include
    KTextEditor::Range r = parseIncludeDirective(line_str, false);
    if (!r.isEmpty())
    {
        r.setBothLines(line);
        return r;
    }

    // No #include parsed... fallback to the default way:
    // just search a word under cursor...

    // Get cirsor line/column
    int col = kv->cursorPosition().column();

    int start = qMax(qMin(col, line_str.length() - 1), 0);
    int end = start;
    kDebug() << "Arghh... trying w/ word under cursor starting from " << start;

    // Seeking for start of current word
    for (; start > 0; --start)
    {
        if (line_str[start].isSpace() || line_str[start] == '<' || line_str[start] == '"')
        {
            start++;
            break;
        }
    }
    // Seeking for end of current word
    while (end < line_str.length() && !line_str[end].isSpace() && line_str[end] != '>' && line_str[end] != '"')
    {
        ++end;
    }

    return KTextEditor::Range(line, start, line, end);
}
//END IncludeHelperPluginView

//BEGIN ChooseFromListDialog
ChooseFromListDialog::ChooseFromListDialog(QWidget* parent)
  : KDialog(parent)
{
    setModal(true);
    setButtons(KDialog::Ok | KDialog::Cancel);
    showButtonSeparator(true);
    setCaption(i18n("Choose Header File to Open"));

    m_list = new KListWidget(this);
    setMainWidget(m_list);

    connect(m_list, SIGNAL(executed(QListWidgetItem*)), this, SLOT(accept()));
}

QStringList ChooseFromListDialog::select(QWidget* parent, const QStringList& strings)
{
    KConfigGroup gcg(KGlobal::config(), "IncludeHelperChooserDialog");
    ChooseFromListDialog dialog(parent);
    dialog.m_list->addItems(strings);                       // append gien items to the list
    dialog.restoreDialogSize(gcg);                          // restore dialog geometry from config

    QStringList result;
    if (dialog.exec() == QDialog::Accepted)
    {
        Q_FOREACH(QListWidgetItem* item, dialog.m_list->selectedItems())
        {
            result.append(item->text());
        }
    }
    dialog.saveDialogSize(gcg);                             // write dialog geometry to config
    gcg.sync();
    return result;
}
//END ChooseFromListDialog
}                                                           // namespace kate