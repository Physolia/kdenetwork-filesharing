/***************************************************************************
                          common.cpp  -  description
                             -------------------
    begin                : Tue June 6 2002
    copyright            : (C) 2002 by Jan Schäfer
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

#include <qstring.h>
#include <kcombobox.h>
#include <q3listbox.h>

#include "common.h"

void setComboToString(QComboBox* combo,const QString & s)
{
  int i = /*combo->model()->index(*/combo->findText( s )/*)*/;//model()->findItem(s,Qt::ExactMatch));
  combo->setCurrentItem(i);
}

bool boolFromText(const QString & value, bool testTrue)
{
  QString lower = value.toLower();
  
  if (testTrue) {
    if (lower=="yes" ||
        lower=="1" ||
        lower=="true" ||
        lower=="on")
      return true;
    else
      return false;
  } else {
    if (lower=="no" ||
        lower=="0" ||
        lower=="false" ||
        lower=="off")
      return false;
    else
      return true;
  }
}

QString textFromBool(bool value)
{
  if (value)
    return "yes";
  else
    return "no";
}


