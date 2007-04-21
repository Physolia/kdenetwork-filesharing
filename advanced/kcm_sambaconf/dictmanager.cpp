/***************************************************************************
                          dictmanager.cpp  -  description
                             -------------------
    begin                : Wed Jan 1 2003
    copyright            : (C) 2003 by Jan Schäfer
    email                : janschaefer@users.sourceforge.net
 ***************************************************************************/

/******************************************************************************
 *                                                                            *
 *  This file is part of KSambaPlugin.                                          *
 *                                                                            *
 *  KSambaPlugin is free software; you can redistribute it and/or modify            *
 *  it under the terms of the GNU General Public License as published by      *
 *  the Free Software Foundation; either version 2 of the License, or         *
 *  (at your option) any later version.                                       *
 *                                                                            *
 *  KSambaPlugin is distributed in the hope that it will be useful,                 *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 *  GNU General Public License for more details.                              *
 *                                                                            *
 *  You should have received a copy of the GNU General Public License         *
 *  along with KSambaPlugin; if not, write to the Free Software                     *
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA  *
 *                                                                            *
 ******************************************************************************/
 
 
#include <qlineedit.h>
#include <qcheckbox.h>
#include <qspinbox.h>
#include <qcombobox.h>
#include <qstringlist.h>

#include <kurlrequester.h> 
#include <klocale.h>
 
#include "sambashare.h"
#include "dictmanager.h" 
#include "common.h"

 
DictManager::DictManager(SambaShare* share):
  lineEditDict(40,false),
  checkBoxDict(40,false),
  urlRequesterDict(40,false),
  spinBoxDict(40,false),
  comboBoxDict(20,false),
  comboBoxValuesDict(20,false)
{
  _share = share;
}

DictManager::~DictManager() {
}

void DictManager::handleUnsupportedWidget(const QString & s, QWidget* w) {
  w->setEnabled(false);
  w->setToolTip(i18n("The option <em>%1</em> is not supported by your Samba version", s));
}

void DictManager::add(const QString & key, QLineEdit* lineEdit) {
  if (_share->optionSupported(key)) {
    lineEditDict.insert(key,lineEdit);
    connect(lineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(changedSlot()));
  } else
    handleUnsupportedWidget(key,lineEdit);
}

void DictManager::add(const QString & key, QCheckBox* checkBox){
  if (_share->optionSupported(key)) {
    checkBoxDict.insert(key,checkBox);
    connect(checkBox, SIGNAL(clicked()), this, SLOT(changedSlot()));
  } else
    handleUnsupportedWidget(key,checkBox);
}

void DictManager::add(const QString & key, KUrlRequester* urlRq){
  if (_share->optionSupported(key)) {
    urlRequesterDict.insert(key,urlRq);
    connect(urlRq, SIGNAL(textChanged(const QString &)), this, SLOT(changedSlot()));
  } else
    handleUnsupportedWidget(key,urlRq);
}

void DictManager::add(const QString & key, QSpinBox* spinBox){
  if (_share->optionSupported(key)) {
    spinBoxDict.insert(key,spinBox);
    connect(spinBox, SIGNAL(valueChanged(int)), this, SLOT(changedSlot()));
  } else
    handleUnsupportedWidget(key,spinBox);
}
              
void DictManager::add(const QString & key, QComboBox* comboBox, QStringList* values){
  if (_share->optionSupported(key)) {
    comboBoxDict.insert(key,comboBox);
    comboBoxValuesDict.insert(key,values);
    connect(comboBox, SIGNAL(activated(int)), this, SLOT(changedSlot()));
  } else
    handleUnsupportedWidget(key,comboBox);
}


void DictManager::load(SambaShare* share, bool globalValue, bool defaultValue){
  Q3DictIterator<QCheckBox> checkBoxIt( checkBoxDict ); 
   
  for( ; checkBoxIt.current(); ++checkBoxIt )	{
    checkBoxIt.current()->setChecked(share->getBoolValue(checkBoxIt.currentKey(),globalValue,defaultValue));
  }

  Q3DictIterator<QLineEdit> lineEditIt( lineEditDict ); 
 
  for( ; lineEditIt.current(); ++lineEditIt )	{
    lineEditIt.current()->setText(share->getValue(lineEditIt.currentKey(),globalValue,defaultValue));
  }

  Q3DictIterator<KUrlRequester> urlRequesterIt( urlRequesterDict ); 
 
  for( ; urlRequesterIt.current(); ++urlRequesterIt )	{
    urlRequesterIt.current()->setUrl(share->getValue(urlRequesterIt.currentKey(),globalValue,defaultValue));
  }

  Q3DictIterator<QSpinBox> spinBoxIt( spinBoxDict ); 
   
  for( ; spinBoxIt.current(); ++spinBoxIt )	{
    spinBoxIt.current()->setValue(share->getValue(spinBoxIt.currentKey(),globalValue,defaultValue).toInt());
  }

  loadComboBoxes(share,globalValue,defaultValue);

}

void DictManager::loadComboBoxes(SambaShare* share, bool globalValue, bool defaultValue) {
  Q3DictIterator<QComboBox> comboBoxIt( comboBoxDict ); 
   
  for( ; comboBoxIt.current(); ++comboBoxIt )	{
    QStringList *v = comboBoxValuesDict[comboBoxIt.currentKey()];
    QString value = share->getValue(comboBoxIt.currentKey(),globalValue,defaultValue);

    if (value.isNull())
        continue;
        
    value = value.toLower();

    
    int comboIndex = 0;
                                
    QStringList::iterator it;
    for ( it = v->begin(); it != v->end(); ++it ) {
      QString lower = (*it).toLower();
      if ( lower == "yes" &&
           boolFromText(value))
           break;

      if ( lower == "no" &&
           ! boolFromText(value,false))
           break;

      if ( lower == value ) 
           break;                            
      
      comboIndex++;
    }

    comboBoxIt.current()->setCurrentItem(comboIndex);
  }
}


void DictManager::save(SambaShare* share, bool globalValue, bool defaultValue){
  Q3DictIterator<QCheckBox> checkBoxIt( checkBoxDict ); 
 
  for( ; checkBoxIt.current(); ++checkBoxIt )	{
    share->setValue(checkBoxIt.currentKey(),checkBoxIt.current()->isChecked(), globalValue, defaultValue );
  }

  Q3DictIterator<QLineEdit> lineEditIt( lineEditDict ); 
   
  for( ; lineEditIt.current(); ++lineEditIt )	{
    share->setValue(lineEditIt.currentKey(),lineEditIt.current()->text(), globalValue, defaultValue );
  }

  Q3DictIterator<KUrlRequester> urlRequesterIt( urlRequesterDict ); 
 
  for( ; urlRequesterIt.current(); ++urlRequesterIt )	{
    share->setValue(urlRequesterIt.currentKey(),urlRequesterIt.current()->url().url(), globalValue, defaultValue );
  }

  Q3DictIterator<QSpinBox> spinBoxIt( spinBoxDict ); 
 
  for( ; spinBoxIt.current(); ++spinBoxIt )	{
    share->setValue(spinBoxIt.currentKey(),spinBoxIt.current()->value(), globalValue, defaultValue );
  }

  Q3DictIterator<QComboBox> comboBoxIt( comboBoxDict );
  
  for( ; comboBoxIt.current(); ++comboBoxIt )	{
    QStringList* values = comboBoxValuesDict[comboBoxIt.currentKey()];
    
    int i = comboBoxIt.current()->currentItem();
    share->setValue(comboBoxIt.currentKey(),(*values)[i], globalValue, defaultValue );
  }
    
}

void DictManager::changedSlot() {
  emit changed();
}



#include "dictmanager.moc"


