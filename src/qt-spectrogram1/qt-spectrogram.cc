/* Audacious-Mod (2024) https://github.com/dualword/Audacious-Mod License:GNU GPL v2*/
/*
 *  Spectrogram plugin for Audacious
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

class SpectrogramWidget : public QWidget {
public:
    SpectrogramWidget (QWidget * p = nullptr);
    ~SpectrogramWidget ();
    void resize (int w, int h);
    void clear ();
    QImage image;

protected:
    void resizeEvent (QResizeEvent *);
    void paintEvent (QPaintEvent *);

private:

};

static SpectrogramWidget *s_widget = nullptr;

SpectrogramWidget::SpectrogramWidget (QWidget * p) :  QWidget (p)
{
    resize (width (), height ());
    image = QImage(width()/2, height()/2, QImage::Format_RGB32);
    image.fill(Qt::black);
}

SpectrogramWidget::~SpectrogramWidget ()
{
    s_widget = nullptr;
}


void SpectrogramWidget::paintEvent (QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(0, 0, width(), height(), Qt::black);
    p.drawImage(0,0,image);
}

void SpectrogramWidget::resizeEvent (QResizeEvent *)
{
    resize (width (), height ());
}

void SpectrogramWidget::resize (int w, int h)
{
   update ();
}

void SpectrogramWidget::clear ()
{
    update ();
}

class ScopeQt : public VisPlugin
{
public:
    static constexpr PluginInfo info = {
        N_("Spectrogram"),
        PACKAGE,
        nullptr,
        nullptr,
        PluginQtOnly
    };

    constexpr ScopeQt () : VisPlugin (info, Visualizer::Freq) {}

    bool init ();
    void cleanup ();
    void * get_qt_widget ();
    void clear ();
    void render_freq (const float * pcm);
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
        s_widget->image.fill(Qt::black);
        s_widget->clear ();
    }
}

//TODO
void ScopeQt::render_freq (const float * pcm)
{
    QRgb *line;
    for (int y = 0; y < s_widget->image.height() ; y++) {
        line = reinterpret_cast<QRgb*>(s_widget->image.scanLine(y));
        for (int x = s_widget->image.width()-1; x >=0; x--) {
            line[x] = line[x-1];
        }
    }

    for(int i = 0; i < 256; ++i) {
        int color = abs(10 * log10f(pcm[i]));
        s_widget->image.setPixelColor(0, i, QColor(color, color, color));
    }

    s_widget->update ();
}

void * ScopeQt::get_qt_widget ()
{
    if (s_widget) return s_widget;

    s_widget = new SpectrogramWidget ();
    return s_widget;
}
