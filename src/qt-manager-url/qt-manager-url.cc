/* Audacious-Mod (2024) https://github.com/dualword/Audacious-Mod License:GNU GPL v2*/
/*
 * playlist-manager-qt.cc
 * Copyright 2015 John Lindgren
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

#include <QtWidgets>

#include <libaudcore/hook.h>
#include <libaudcore/i18n.h>
#include <libaudcore/playlist.h>
#include <libaudcore/plugin.h>
#include <libaudqt/libaudqt.h>
#include <libaudqt/treeview.h>
#include <libaudcore/runtime.h>
#include <libaudcore/drct.h>

#include "../ui-common/qt-compat.h"

class UrlManagerQt : public GeneralPlugin
{
public:
    static constexpr PluginInfo info = {N_("URL Manager"), PACKAGE,
                                        nullptr, // about
                                        nullptr, // prefs
                                        PluginQtOnly};

    constexpr UrlManagerQt() : GeneralPlugin(info, false) {}

    void * get_qt_widget();
    int take_message(const char * code, const void *, int);
};

EXPORT UrlManagerQt aud_plugin_instance;

class PlaylistsModel : public QAbstractListModel
{
public:
    enum
    {
        ColumnTitle,
        ColumnEntries,
        NColumns
    };

    PlaylistsModel()
        : m_rows(Playlist::n_playlists()), m_playing(Playlist::playing_playlist().index())
    {
        QFile file( QString(aud_get_path(AudPath::UserDir)).append(QDir::separator())
                   .append("url.txt"));
        if (file.open(QFile::ReadOnly)) {
          QTextStream stream(&file);
          QString line;
          while (stream.readLineInto(&line)) {
              line = line.trimmed();
              list.append(line);
          }
        }
        file.close();
    }

    void setFont(const QFont & font)
    {
        m_bold = font;
        m_bold.setBold(true);

        if (m_playing >= 0)
            update_rows(m_playing, 1);
    }

    void update(Playlist::UpdateLevel level);

protected:
    int rowCount(const QModelIndex & parent) const
    {
        return list.size();
    }

    int columnCount(const QModelIndex & parent) const { return 1; }

    Qt::DropActions supportedDropActions() const { return Qt::MoveAction; }

    Qt::ItemFlags flags(const QModelIndex & index) const
    {
        if (index.isValid())
            return Qt::ItemIsSelectable | Qt::ItemIsDragEnabled |
                   Qt::ItemIsEnabled;
        else
            return Qt::ItemIsSelectable | Qt::ItemIsDropEnabled |
                   Qt::ItemIsEnabled;
    }

    QVariant data(const QModelIndex & index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role) const;

private:
    void update_rows(int row, int count);
    void update_playing();

//    const HookReceiver<PlaylistsModel> activate_hook{
//        "playlist set playing", this, &PlaylistsModel::update_playing};

    int m_rows, m_playing;
    QFont m_bold;
    QStringList list;
};

class PlaylistsView : public audqt::TreeView
{
public:
    PlaylistsView();
    void refresh(){auto tmp = m_model; auto o = new PlaylistsModel(); m_model = o; setModel(m_model); delete tmp; };

protected:
    void changeEvent(QEvent * event) override
    {
        if (event->type() == QEvent::FontChange)
            m_model->setFont(font());

        audqt::TreeView::changeEvent(event);
    }

    void currentChanged(const QModelIndex & current,
                        const QModelIndex & previous) override;
    void dropEvent(QDropEvent * event) override;

private:
    PlaylistsModel *m_model=new PlaylistsModel();

    void update(Playlist::UpdateLevel level);
    void update_sel();

//    const HookReceiver<PlaylistsView, Playlist::UpdateLevel> //
//        update_hook{"playlist update", this, &PlaylistsView::update};
//    const HookReceiver<PlaylistsView> //
//        activate_hook{"playlist activate", this, &PlaylistsView::update_sel};

    int m_in_update = 0;
};

QVariant PlaylistsModel::data(const QModelIndex & index, int role) const
{
    switch (role)
    {
    case Qt::DisplayRole:
    {
        switch (index.column())
        {
        case ColumnTitle:
            return list[index.row()];
        }
    }
    break;

    case Qt::FontRole:
        if (index.row() == m_playing)
            return m_bold;
        break;

    case Qt::TextAlignmentRole:
        if (index.column() == ColumnEntries)
            return Qt::AlignRight;
        break;
    }

    return QVariant();
}

QVariant PlaylistsModel::headerData(int section, Qt::Orientation orientation,
                                    int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch (section)
        {
        case ColumnTitle:
            return QString(_("Title"));
        case ColumnEntries:
            return QString(_("Entries"));
        }
    }

    return QVariant();
}

void PlaylistsModel::update_rows(int row, int count)
{
    if (count < 1)
        return;

    auto topLeft = createIndex(row, 0);
    auto bottomRight = createIndex(row + count - 1, NColumns - 1);
    emit dataChanged(topLeft, bottomRight);
}

void PlaylistsModel::update_playing()
{
    int playing = Playlist::playing_playlist().index();

    if (playing != m_playing)
    {
        if (m_playing >= 0)
            update_rows(m_playing, 1);
        if (playing >= 0)
            update_rows(playing, 1);

        m_playing = playing;
    }
}

void PlaylistsModel::update(const Playlist::UpdateLevel level)
{
    int rows = Playlist::n_playlists();

    if (level == Playlist::Structure)
    {
        if (rows < m_rows)
        {
            beginRemoveRows(QModelIndex(), rows, m_rows - 1);
            m_rows = rows;
            endRemoveRows();
        }
        else if (rows > m_rows)
        {
            beginInsertRows(QModelIndex(), m_rows, rows - 1);
            m_rows = rows;
            endInsertRows();
        }
    }

    if (level >= Playlist::Metadata)
    {
        update_rows(0, m_rows);
        m_playing = Playlist::playing_playlist().index();
    }
    else
        update_playing();
}

PlaylistsView::PlaylistsView()
{
    setModel(m_model);
    m_model->setFont(font());
    setHeaderHidden(true);

    m_in_update++;
    update_sel();
    m_in_update--;

    auto hdr = header();
    hdr->setStretchLastSection(false);
    hdr->setSectionResizeMode(PlaylistsModel::ColumnTitle,
                              QHeaderView::Stretch);
    hdr->setSectionResizeMode(PlaylistsModel::ColumnEntries,
                              QHeaderView::Interactive);
    hdr->resizeSection(PlaylistsModel::ColumnEntries, audqt::to_native_dpi(64));

    setAllColumnsShowFocus(true);
    setDragDropMode(InternalMove);
    setFrameShape(QFrame::NoFrame);
    setIndentation(0);

    connect(this, &QTreeView::activated, [](const QModelIndex & index) {
        if (index.isValid())
            aud_drct_pl_open(index.data().toString().toStdString().c_str());
            //Playlist::by_index(index.row()).start_playback();
    });
}

void PlaylistsView::currentChanged(const QModelIndex & current,
                                   const QModelIndex & previous)
{
    audqt::TreeView::currentChanged(current, previous);
//    if (!m_in_update)
//        Playlist::by_index(current.row()).activate();
}

void PlaylistsView::dropEvent(QDropEvent * event)
{
    if (event->source() != this || event->proposedAction() != Qt::MoveAction)
        return;

    int from = currentIndex().row();
    if (from < 0)
        return;

    int to;
    switch (dropIndicatorPosition())
    {
    case AboveItem:
        to = indexAt(QtCompat::pos(event)).row();
        break;
    case BelowItem:
        to = indexAt(QtCompat::pos(event)).row() + 1;
        break;
    case OnViewport:
        to = Playlist::n_playlists();
        break;
    default:
        return;
    }

    Playlist::reorder_playlists(from, (to > from) ? to - 1 : to, 1);
    event->acceptProposedAction();
}

void PlaylistsView::update(Playlist::UpdateLevel level)
{
    m_in_update++;
    m_model->update(level);
    update_sel();
    m_in_update--;
}

void PlaylistsView::update_sel()
{
    m_in_update++;
    auto sel = selectionModel();
    auto current = m_model->index(Playlist::active_playlist().index(), 0);
    sel->setCurrentIndex(current, sel->ClearAndSelect | sel->Rows);
    m_in_update--;
}

static QPointer<PlaylistsView> s_playlists_view;

static QToolButton * new_tool_button(const char * text, const char * icon)
{
    auto button = new QToolButton;
    button->setIcon(QIcon::fromTheme(icon));
    button->setText(audqt::translate_str(text));
    button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    return button;
}

void * UrlManagerQt::get_qt_widget()
{
    s_playlists_view = new PlaylistsView;

    QLineEdit* txt = new QLineEdit(s_playlists_view);
    auto new_button = new_tool_button(N_("_Add"), "document-new");
    QObject::connect(new_button, &QToolButton::clicked, [=]() {
        QFile file( QString(aud_get_path(AudPath::UserDir)).append(QDir::separator())
                   .append("url.txt"));
        if (file.open(QIODevice::Append)) {
            QTextStream stream(&file);
            stream << txt->text() << "\n";
            file.close();
        }
        s_playlists_view->refresh();
        txt->clear();
    });

    auto hbox = audqt::make_hbox(nullptr);
    hbox->setContentsMargins(audqt::margins.TwoPt);
    hbox->addWidget(txt);
    hbox->addWidget(new_button);

    auto widget = new QWidget;
    auto vbox = audqt::make_vbox(widget, 0);

    vbox->addLayout(hbox);
    vbox->addWidget(s_playlists_view, 1);
    return widget;
}

int UrlManagerQt::take_message(const char * code, const void *, int)
{
    if (!strcmp(code, "grab focus") && s_playlists_view)
    {
        s_playlists_view->setFocus(Qt::OtherFocusReason);
        return 0;
    }

    return -1;
}
