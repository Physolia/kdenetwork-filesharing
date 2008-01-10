/***************************************************************************
                          qextendedchecklistitem.h  -  description
                             -------------------
    begin                : Sun Jan 26 2003
    copyright            : (C) 2003 by Jan Schäfer
    email                : janschaefer@users.sourceforge.net
 ***************************************************************************/

/******************************************************************************
 *                                                                            *
 *  This file is part of KSambaPlugin.                                        *
 *                                                                            *
 *  KSambaPlugin is free software; you can redistribute it and/or modify      *
 *  it under the terms of the GNU General Public License as published by      *
 *  the Free Software Foundation; either version 2 of the License, or         *
 *  (at your option) any later version.                                       *
 *                                                                            *
 *  KSambaPlugin is distributed in the hope that it will be useful,           *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 *  GNU General Public License for more details.                              *
 *                                                                            *
 *  You should have received a copy of the GNU General Public License         *
 *  along with KSambaPlugin; if not, write to the Free Software               *
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA  *
 *                                                                            *
 ******************************************************************************/
  
#ifndef _QMULTICHECKLISTITEM_H_
#define _QMULTICHECKLISTITEM_H_

#include <q3listview.h>

#include <qbitarray.h>

class QMultiCheckListItem : public QObject, public Q3ListViewItem {
Q_OBJECT

  public:
    QMultiCheckListItem( Q3ListView *parent=0);
    ~QMultiCheckListItem() {}
    
    virtual bool isOn(int column);
    virtual bool isDisabled(int column);
      
    
  protected:
    /* reimp */
    virtual void paintCell(QPainter *, const QColorGroup &, int, int, int);
    
  public slots:
    virtual void setOn(int, bool);
    virtual void toggle(int);  
    virtual void setDisabled(int, bool);
    
  signals:
    void stateChanged(int, bool);    
  private:
    QBitArray checkStates;
    QBitArray checkBoxColumns;
    QBitArray disableStates;
    void resize(int column);
};

#endif
