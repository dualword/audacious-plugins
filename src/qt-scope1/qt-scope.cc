/* Audacious-Mod (2024) https://github.com/dualword/Audacious-Mod License:GNU GPL v2*/
/*
 *  Scope plugin for Audacious
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <math.h>
#include <string.h>

#include <QtWidgets>
#include <QImage>
#include <QPainter>
#include <QDebug>
#include <QVector>

#include "libaudqt/colorbutton.h"

#include <libaudcore/i18n.h>
#include <libaudcore/runtime.h>
#include <libaudcore/plugin.h>
#include <libaudcore/preferences.h>

class ScopeWidget : public QWidget {
public:
    ScopeWidget (QWidget * p = nullptr);
    ~ScopeWidget ();
    void resize (int w, int h);
    void clear ();
    void draw_vert_line (int x, int y1, int y2);
    QVector<QPointF> vec;

protected:
    void resizeEvent (QResizeEvent *);
    void paintEvent (QPaintEvent *);

private:

};

static ScopeWidget *s_widget = nullptr;

ScopeWidget::ScopeWidget (QWidget * p) :  QWidget (p), vec(512)
{
    resize (width (), height ());
}

ScopeWidget::~ScopeWidget ()
{
    s_widget = nullptr;
}


void ScopeWidget::paintEvent (QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(0, 0, width(), height(), Qt::black);

    QPen pen( Qt::green, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin );
    p.setPen( pen );
    p.drawLines(vec);
}

void ScopeWidget::resizeEvent (QResizeEvent *)
{
    resize (width (), height ());
}

void ScopeWidget::resize (int w, int h)
{
   update ();
}

void ScopeWidget::clear ()
{
    update ();
}

class ScopeQt : public VisPlugin
{
public:
    static constexpr PluginInfo info = {
        N_("Scope1"),
        PACKAGE,
        nullptr,
        nullptr,
        PluginQtOnly
    };

    constexpr ScopeQt () : VisPlugin (info, Visualizer::MonoPCM) {}

    bool init ();
    void cleanup ();

    void * get_qt_widget ();

    void clear ();
    void render_mono_pcm (const float * pcm);
};

EXPORT ScopeQt aud_plugin_instance;

bool ScopeQt::init ()
{
    return true;
}

void ScopeQt::cleanup ()
{

}

void ScopeQt::clear ()
{
    if (s_widget){
        for(auto& p : s_widget->vec){
            p.rx() = 0;
            p.ry() = 0;
        }
        s_widget->clear ();
    }
}

void ScopeQt::render_mono_pcm (const float * pcm)
{
    float width = s_widget->width ();
    float height = s_widget->height ();
    float x0 = 0, y0 = height *0.5;

    for (int i = 0; i < 512; i+=2  ) {
        s_widget->vec[i].rx() = x0;
        s_widget->vec[i].ry() = y0;
        x0 += width/256;
        float y1 = height * 0.5 + pcm[i]*100;
        s_widget->vec[i+1].rx() = x0;
        s_widget->vec[i+1].ry() = y1;
        y0 = y1;
    }
    s_widget->update ();
}

void * ScopeQt::get_qt_widget ()
{
    if (s_widget) return s_widget;

    s_widget = new ScopeWidget ();
    return s_widget;
}
