/***************************************************************************
                          sharedlgimpl.cpp  -  description
                             -------------------
    begin                : Tue June 6 2002
    copyright            : (C) 2002 by Jan Sch�fer
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA  *
 *                                                                            *
 ******************************************************************************/


/**
 * @author Jan Sch�fer
 **/

#include <qcheckbox.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <qgroupbox.h>
#include <qlayout.h>
#include <qtabwidget.h>
#include <qregexp.h>
#include <qstringlist.h>
#include <qgrid.h>
#include <qcursor.h>
#include <qtable.h>
#include <qlistbox.h>

#include <klineedit.h>
#include <kurlrequester.h>
#include <knuminput.h>
#include <kdebug.h>
#include <kcombobox.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kfiledetailview.h>
#include <kdirlister.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <kaction.h>

#include <assert.h>

#include "passwd.h"
#include "smbpasswdfile.h"
#include "sambafile.h"
#include "sharedlgimpl.h"
#include "common.h"

#define COL_NAME 0
#define COL_HIDDEN 1
#define COL_VETO 2
#define COL_SIZE 3
#define COL_DATE 4
#define COL_PERM 5
#define COL_OWNER 6
#define COL_GROUP 7

#define HIDDENTABINDEX 5

HiddenListViewItem::HiddenListViewItem( QListView *parent, KFileItem *fi, bool hidden, bool veto )
  : KListViewItem( parent )
{
  setPixmap( COL_NAME, fi->pixmap(KIcon::SizeSmall));

  setText( COL_NAME, fi->text() );
  setText( COL_SIZE, KGlobal::locale()->formatNumber( fi->size(), 0));
  setText( COL_DATE,  fi->timeString() );
  setText( COL_PERM,  fi->permissionsString() );
  setText( COL_OWNER, fi->user() );
  setText( COL_GROUP, fi->group() );

  setHidden(hidden);
  setVeto(veto);
}

HiddenListViewItem::~HiddenListViewItem()
{
}

void HiddenListViewItem::setVeto(bool b)
{
  _veto = b;
  setText( COL_VETO, _veto ? i18n("Yes") : QString(""));
}

bool HiddenListViewItem::isVeto()
{
  return _veto;
}

void HiddenListViewItem::setHidden(bool b)
{
  _hidden = b;
  setText( COL_HIDDEN, _hidden ? i18n("Yes") : QString("") );
}

bool HiddenListViewItem::isHidden()
{
  return _hidden;
}

void HiddenListViewItem::paintCell(QPainter *p, const QColorGroup &cg, int column, int width, int alignment)
{
  QColorGroup _cg = cg;

  if (isVeto())
     _cg.setColor(QColorGroup::Base,lightGray);

  if (isHidden())
     _cg.setColor(QColorGroup::Text,gray);

  QListViewItem::paintCell(p, _cg, column, width, alignment);
}




HiddenFileView::HiddenFileView(ShareDlgImpl* shareDlg, SambaShare* share)
{
  _share = share;
  _dlg = shareDlg;

  _hiddenActn = new KToggleAction(i18n("&Hide"));
  _vetoActn = new KToggleAction(i18n("&Veto"));

  initListView();

  _dlg->hiddenChk->setTristate(true);
  _dlg->vetoChk->setTristate(true);

  connect( _dlg->hiddenChk, SIGNAL(toggled(bool)), this, SLOT( hiddenChkClicked(bool) ));
  connect( _dlg->vetoChk, SIGNAL(toggled(bool)), this, SLOT( vetoChkClicked(bool) ));

  _dlg->hiddenEdit->setText( _share->getValue("hide files") );
  connect( _dlg->hiddenEdit, SIGNAL(textChanged(const QString &)), this, SLOT(updateView()));

  _dlg->vetoEdit->setText( _share->getValue("veto files") );
  connect( _dlg->vetoEdit, SIGNAL(textChanged(const QString &)), this, SLOT(updateView()));


//  new QLabel(i18n("Hint")+" : ",grid);
//  new QLabel(i18n("You have to separate the entries with a '/'. You can use the wildcards '*' and '?'"),grid);
//  new QLabel(i18n("Example")+" : ",grid);
//  new QLabel(i18n("*.tmp/*SECRET*/.*/file?.*/"),grid);

  _dir = new KDirLister(true);

  connect( _dir, SIGNAL(newItems(const KFileItemList &)),
           this, SLOT(insertNewFiles(const KFileItemList &)));

  connect( _hiddenActn, SIGNAL(toggled(bool)), this, SLOT(hiddenChkClicked(bool)));
  connect( _vetoActn, SIGNAL(toggled(bool)), this, SLOT(vetoChkClicked(bool)));
}

void HiddenFileView::initListView()
{
  _dlg->hiddenListView->setMultiSelection(true);
  _dlg->hiddenListView->setSelectionMode(QListView::Extended);
  _dlg->hiddenListView->setAllColumnsShowFocus(true);

  _hiddenList = createRegExpList(_share->getValue("hide files"));
  _vetoList = createRegExpList(_share->getValue("veto files"));

  _popup = new KPopupMenu(_dlg->hiddenListView);
  _hiddenActn->plug(_popup);
  _vetoActn->plug(_popup);

  connect( _dlg->hiddenListView, SIGNAL(selectionChanged()), this, SLOT(selectionChanged()));
  connect( _dlg->hiddenListView, SIGNAL(contextMenu(KListView*,QListViewItem*,const QPoint&)),
           this, SLOT(showContextMenu(QListViewItem*,const QPoint&)));

}

HiddenFileView::~HiddenFileView()
{
}

void HiddenFileView::load()
{
  _dir->openURL( KURL(_share->getValue("path")) );
}

void HiddenFileView::save()
{
  QString s = _dlg->hiddenEdit->text().stripWhiteSpace();
  // its important that the string ends with an '/'
  // as Samba won't recognize the last entry
  if (s != "" && s.right(1)!="/")
      s+="/";
  _share->setValue("hide files", s);

  s = _dlg->vetoEdit->text().stripWhiteSpace();
  // its important that the string ends with an '/'
  // as Samba won't recognize the last entry
  if (s != "" && s.right(1)!="/")
      s+="/";
  _share->setValue("veto files", s);
}

void HiddenFileView::insertNewFiles(const KFileItemList &newone)
{
  if ( newone.isEmpty() )
     return;

  KFileItem *tmp;

  int j=0;

  for (KFileItemListIterator it(newone); (tmp = it.current()); ++it)
  {
    j++;

    bool hidden = matchHidden(tmp->text());
    bool veto = matchVeto(tmp->text());

    HiddenListViewItem *item = new HiddenListViewItem( _dlg->hiddenListView, tmp, hidden, veto );
  }
}

void HiddenFileView::showContextMenu(QListViewItem* l,const QPoint & p)
{
  _popup->exec(QCursor::pos());
}


void HiddenFileView::selectionChanged()
{
  bool veto = false;
  bool noVeto = false;
  bool hide = false;
  bool noHide = false;

  int n = 0;

  HiddenListViewItem* item;
  for (item = static_cast<HiddenListViewItem*>(_dlg->hiddenListView->firstChild());item;
       item = static_cast<HiddenListViewItem*>(item->nextSibling()))
  {
    if (!item->isSelected())
       continue;

    n++;

    if (item->isVeto())
       veto = true;
    else
       noVeto = true;

    if (item->isHidden())
       hide = true;
    else
       noHide = true;
  }


  _dlg->selGrpBx->setEnabled(n>0);

  if (veto && noVeto)
  {
    _dlg->vetoChk->setTristate(true);
    _dlg->vetoChk->setNoChange();
    _dlg->vetoChk->update();
  }
  else
  {
    _dlg->vetoChk->setTristate(false);
    _dlg->vetoChk->setChecked(veto);
  }

  if (hide && noHide)
  {
    _dlg->hiddenChk->setTristate(true);
    _dlg->hiddenChk->setNoChange();
    _dlg->hiddenChk->update();
  }
  else
  {
    _dlg->hiddenChk->setTristate(false);
    _dlg->hiddenChk->setChecked(hide);
  }
}

void HiddenFileView::hiddenChkClicked(bool b)
{
  // We don't save the old state so
  // disable the tristate mode
  _dlg->hiddenChk->setTristate(false);
  _hiddenActn->setChecked(b);
  _dlg->hiddenChk->setChecked(b);

  HiddenListViewItem* item;
  for (item = static_cast<HiddenListViewItem*>(_dlg->hiddenListView->firstChild());item;
       item = static_cast<HiddenListViewItem*>(item->nextSibling()))
  {
    if (item->isSelected())
    {
      // If we remove a file from the list
      // perhaps it was a wildcard string
      if (!b && item->isHidden())
      {
        QRegExp* rx = getHiddenMatch(item->text(0));
        assert(rx);

        QString p = rx->pattern();
        if ( p.find("*") > -1 ||
             p.find("?") > -1 )
        {
          int result = KMessageBox::questionYesNo(_dlg,i18n(
            "<b></b>Some files you have selected are matched by the wildcarded string <b>'%1'</b> ! "
            "Do you want to unhide all files matching <b>'%1'</b> ? <br>"
            "(If you say no, no file matching '%1' will be unhidden)").arg(rx->pattern()).arg(rx->pattern()).arg(rx->pattern()),
            i18n("Wildcarded string"));

          QPtrList<HiddenListViewItem> lst = getMatchingItems( *rx );

          if (result == KMessageBox::No)
          {
            deselect(lst);
            continue;
          }
          else
          {
            setHidden(lst,false);
          }
        }

        _hiddenList.remove(rx);
      }
      else
      if (b && !item->isHidden())
      {
        _hiddenList.append( new QRegExp(item->text(0)) );
      }

      item->setHidden(b);
    }
  }

  updateEdit(_dlg->hiddenEdit, _hiddenList);
  _dlg->hiddenListView->update();
}

void HiddenFileView::vetoChkClicked(bool b)
{
  // We don't save the old state so
  // disable the tristate mode
  _dlg->vetoChk->setTristate(false);
  _vetoActn->setChecked(b);
  _dlg->vetoChk->setChecked(b);

  HiddenListViewItem* item;
  for (item = static_cast<HiddenListViewItem*>(_dlg->hiddenListView->firstChild());item;
       item = static_cast<HiddenListViewItem*>(item->nextSibling()))
  {
    if (item->isSelected())
    {
      // If we remove a file from the list
      // perhaps it was a wildcard string
      if (!b && item->isVeto())
      {
        QRegExp* rx = getVetoMatch(item->text(0));
        assert(rx);

        QString p = rx->pattern();
        if ( p.find("*") > -1 ||
             p.find("?") > -1 )
        {
          int result = KMessageBox::questionYesNo(_dlg,i18n(
            "<b></b>Some files you have selected are matched by the wildcarded string <b>'%1'</b> ! "
            "Do you want to unveto all files matching <b>'%1'</b> ? <br>"
            "(If you say no, no file matching '%1' will be unvetoed)").arg(rx->pattern()).arg(rx->pattern()).arg(rx->pattern()),
            i18n("Wildcarded string"));

          QPtrList<HiddenListViewItem> lst = getMatchingItems( *rx );

          if (result == KMessageBox::No)
          {
            deselect(lst);
            continue;
          }
          else
          {
            setVeto(lst,false);
          }
        }

        _vetoList.remove(rx);
      }
      else
      if (b && !item->isVeto())
      {
        _vetoList.append( new QRegExp(item->text(0)) );
      }

      item->setVeto(b);
    }


  }

  updateEdit(_dlg->vetoEdit, _vetoList);
  _dlg->hiddenListView->update();
}

/**
 * Sets the text of the QLineEdit edit to the entries of the passed QRegExp-List
 **/
void HiddenFileView::updateEdit(QLineEdit* edit, QPtrList<QRegExp> & lst)
{
  QString s="";

  QRegExp* rx;
  for(rx = static_cast<QRegExp*>(lst.first()); rx;
      rx = static_cast<QRegExp*>(lst.next()) )
  {
    s+= rx->pattern()+QString("/");
  }

  edit->setText(s);
}

void HiddenFileView::setVeto(QPtrList<HiddenListViewItem> & lst, bool b)
{
  HiddenListViewItem* item;
  for(item = static_cast<HiddenListViewItem*>(lst.first()); item;
      item = static_cast<HiddenListViewItem*>(lst.next()) )
  {
    item->setVeto(b);
  }
}

void HiddenFileView::setHidden(QPtrList<HiddenListViewItem> & lst, bool b)
{
  HiddenListViewItem* item;
  for(item = static_cast<HiddenListViewItem*>(lst.first()); item;
      item = static_cast<HiddenListViewItem*>(lst.next()) )
  {
    item->setHidden(b);
  }
}

void HiddenFileView::deselect(QPtrList<HiddenListViewItem> & lst)
{
  HiddenListViewItem* item;
  for(item = static_cast<HiddenListViewItem*>(lst.first()); item;
      item = static_cast<HiddenListViewItem*>(lst.next()) )
  {
    item->setSelected(false);
  }
}


QPtrList<HiddenListViewItem> HiddenFileView::getMatchingItems(const QRegExp & rx)
{
  QPtrList<HiddenListViewItem> lst;

  HiddenListViewItem* item;
  for (item = static_cast<HiddenListViewItem*>(_dlg->hiddenListView->firstChild());item;
       item = static_cast<HiddenListViewItem*>(item->nextSibling()))
  {
    if (rx.exactMatch(item->text(0)))
       lst.append(item);
  }

  return lst;
}

void HiddenFileView::updateView()
{
  _hiddenList = createRegExpList(_dlg->hiddenEdit->text());
  _vetoList = createRegExpList(_dlg->vetoEdit->text());

  HiddenListViewItem* item;
  for (item = static_cast<HiddenListViewItem*>(_dlg->hiddenListView->firstChild());item;
       item = static_cast<HiddenListViewItem*>(item->nextSibling()))
  {
    item->setHidden(matchHidden(item->text(0)));
    item->setVeto(matchVeto(item->text(0)));
  }

  _dlg->hiddenListView->repaint();
}


QPtrList<QRegExp> HiddenFileView::createRegExpList(const QString & s)
{
  QPtrList<QRegExp> lst;
  bool cs = _share->getBoolValue("case sensitive");

  if (s != "")
  {
    QStringList l = QStringList::split("/",s);

    for ( QStringList::Iterator it = l.begin(); it != l.end(); ++it ) {
        lst.append( new QRegExp(*it,cs,true) );
    }
  }

  return lst;
}

bool HiddenFileView::matchHidden(const QString & s)
{
  return matchRegExpList(s,_hiddenList);
}

bool HiddenFileView::matchVeto(const QString & s)
{
  return matchRegExpList(s,_vetoList);
}

bool HiddenFileView::matchRegExpList(const QString & s, QPtrList<QRegExp> & lst)
{
  if (getRegExpListMatch(s,lst))
     return true;
  else
     return false;
}


QRegExp* HiddenFileView::getHiddenMatch(const QString & s)
{
  return getRegExpListMatch(s,_hiddenList);
}

QRegExp* HiddenFileView::getVetoMatch(const QString & s)
{
  return getRegExpListMatch(s,_vetoList);
}

QRegExp* HiddenFileView::getRegExpListMatch(const QString & s, QPtrList<QRegExp> & lst)
{
  QRegExp* rx;

  for ( rx = lst.first(); rx; rx = lst.next() )
  {
    if (rx->exactMatch(s))
       return rx;
  }

  return 0L;
}



ShareDlgImpl::ShareDlgImpl(QWidget* parent, SambaShare* share)
	: KcmShareDlg(parent,"sharedlgimpl")
{

  _share = share;
	assert(_share);
  initDialog();
}

void ShareDlgImpl::initDialog()
{
	// Base settings

	assert(_share);
  
  if (!_share)
     return;

	pathUrlRq->setMode(2+8+16);

  homeChk->setChecked( _share->getName() == "homes" );
  pathUrlRq->setURL( _share->getValue("path") );

  shareNameEdit->setText( _share->getName() );

	commentEdit->setText( _share->getValue("comment") );

  availableBaseChk->setChecked( _share->getBoolValue("available") );
  browseableBaseChk->setChecked( _share->getBoolValue("browseable") );
  readOnlyBaseChk->setChecked( ! _share->getBoolValue("writeable") );
  publicBaseChk->setChecked( _share->getBoolValue("public") );

  // User settings

  invalidUsersEdit->setText( _share->getValue("invalid users") );

  loadUserTable();

  // Filename settings
  
  defaultCaseCombo->setCurrentText( _share->getValue("default case") );
  caseSensitiveChk->setChecked( _share->getBoolValue("case sensitive") );
  preserveCaseChk->setChecked( _share->getBoolValue("preserve case") );
  shortPreserveCaseChk->setChecked( _share->getBoolValue("short preserve case") );
  mangledNamesChk->setChecked( _share->getBoolValue("mangled names") );
  mangleCaseChk->setChecked( _share->getBoolValue("mangle case") );
  manglingCharEdit->setText( _share->getValue("mangling char") );

  hideDotFilesChk->setChecked( _share->getBoolValue("hide dot files") );
  hideTrailingDotChk->setChecked( _share->getBoolValue("strip dot") );
  hideUnreadableChk->setChecked( _share->getBoolValue("hide unreadable") );
  dosFilemodeChk->setChecked( _share->getBoolValue("dos filemode") );
  dosFiletimesChk->setChecked( _share->getBoolValue("dos filetimes") );
  dosFiletimeResolutionChk->setChecked( _share->getBoolValue("dos filetime resolution") );
  deleteReadonlyChk->setChecked( _share->getBoolValue("delete readonly") );


//-  readOnlyChk->setChecked( _share->getBoolValue("read only") );
//-  guestOkChk->setChecked( _share->getBoolValue("guest ok") );
  guestOnlyChk->setChecked( _share->getBoolValue("guest only") );
  userOnlyChk->setChecked( _share->getBoolValue("user only") );
  hostsAllowEdit->setText( _share->getValue("hosts allow") );
  guestAccountEdit->setText( _share->getValue("guest account") );
  hostsDenyEdit->setText( _share->getValue("hosts deny") );
  forceDirectorySecurityModeEdit->setText( _share->getValue("force directory security mode") );
  forceDirectoryModeEdit->setText( _share->getValue("force directory mode") );
  forceSecurityModeEdit->setText( _share->getValue("force security mode") );
  
  forceCreateModeEdit->setText( _share->getValue("force create mode") );
  directorySecurityMaskEdit->setText( _share->getValue("directory security mask") );
  directoryMaskEdit->setText( _share->getValue("directory mask") );
  securityMaskEdit->setText( _share->getValue("security mask") );
  createMaskEdit->setText( _share->getValue("create mask") );
  inheritPermissionsChk->setChecked( _share->getBoolValue("inherit permissions") );
  
  // Advanced

  blockingLocksChk->setChecked( _share->getBoolValue("blocking locks") );
  fakeOplocksChk->setChecked( _share->getBoolValue("fake oplocks") );
  lockingChk->setChecked( _share->getBoolValue("locking") );
  level2OplocksChk->setChecked( _share->getBoolValue("level2 oplocks") );
  posixLockingChk->setChecked( _share->getBoolValue("posix locking") );
  strictLockingChk->setChecked( _share->getBoolValue("strict locking") );
  shareModesChk->setChecked( _share->getBoolValue("share modes") );
  oplocksChk->setChecked( _share->getBoolValue("oplocks") );
  
  oplockContentionLimitInput->setValue( _share->getValue("oplock contention limit").toInt() );
  strictSyncChk->setChecked( _share->getBoolValue("strict sync") );

  maxConnectionsInput->setValue( _share->getValue("max connections").toInt() );
  writeCacheSizeInput->setValue( _share->getValue("write cache size").toInt() );

  syncAlwaysChk->setChecked( _share->getBoolValue("sync always") );
  statusChk->setChecked( _share->getBoolValue("status") );

  _fileView = 0L;

  connect( tabs, SIGNAL(currentChanged(QWidget*)), this, SLOT(tabChangedSlot(QWidget*)));
}

ShareDlgImpl::~ShareDlgImpl()
{
}

void ShareDlgImpl::loadUserTable()
{
  forceUserCombo->insertItem("");
  forceGroupCombo->insertItem("");

  forceUserCombo->insertStringList( getUnixUsers() );
  forceGroupCombo->insertStringList( getUnixGroups() );

  setComboToString(forceUserCombo, _share->getValue("force user"));
  setComboToString(forceGroupCombo, _share->getValue("force group"));


  possibleUserListView->setSelectionMode(QListView::Extended);

  QStringList validUsers = QStringList::split(QRegExp("[,\\s]+"),_share->getValue("valid users"));
  QStringList readList = QStringList::split(QRegExp("[,\\s]+"),_share->getValue("read list"));
  QStringList writeList = QStringList::split(QRegExp("[,\\s]+"),_share->getValue("write list"));
  QStringList adminUsers = QStringList::split(QRegExp("[,\\s]+"),_share->getValue("admin users"));

  QStringList added;

  QStringList accessRights;
  accessRights << "Share" << "Read only" << "Writeable" << "Admin";

  int row=0;

  userTable->setLeftMargin(0);

  SmbPasswdFile passwd( KURL(_share->getValue("smb passwd file",true,true)) );
  SambaUserList sambaList = passwd.getSambaUserList();

  // if the valid users list contains no entries
  // then all users are allowed !
  if (validUsers.empty())
     validUsers = sambaList.getUserNames();

  QStringList all = validUsers+readList+writeList+adminUsers;


  for ( QStringList::Iterator it = all.begin(); it != all.end(); ++it )
  {
    if (added.find(*it)!=added.end())
       continue;

    added.append(*it);

    userTable->setNumRows(row+1);

    QTableItem* item = new QTableItem( userTable,QTableItem::Never, *it );
    userTable->setItem(row,0,item);
    QComboTableItem* comboItem = new QComboTableItem( userTable,accessRights);
    userTable->setItem(row,1,comboItem);

    QStringList::Iterator it2;

    it2=readList.find(*it);
    if (it2 != readList.end())
    {
      comboItem->setCurrentItem(1);
      readList.remove(it2);
    }

    it2=writeList.find(*it);
    if (it2 != writeList.end())
    {
      comboItem->setCurrentItem(2);
      writeList.remove(it2);
    }

    it2=adminUsers.find(*it);
    if (it2 != adminUsers.end())
    {
      comboItem->setCurrentItem(3);
      adminUsers.remove(it2);
    }

    row++;
  }

  SambaUser *user;
  for ( user = sambaList.first(); user; user = sambaList.next() )
  {
    QStringList::Iterator it2;

    it2=added.find(user->name);
    if (it2 == added.end())
        new KListViewItem(possibleUserListView, user->name);
  }


}

void ShareDlgImpl::addAllowedUserBtnClicked()
{
  QPtrList<QListViewItem> list = possibleUserListView->selectedItems();
  QStringList accessRights;
  accessRights << "Share" << "Read only" << "Writeable" << "Admin";

  QListViewItem* item;
  for ( item = list.first(); item; item = list.first() )
  {
    int row = userTable->numRows();
    userTable->setNumRows(row+1);

    QTableItem* tableItem = new QTableItem( userTable,QTableItem::Never, item->text(0) );
    userTable->setItem(row,0,tableItem);
    QComboTableItem* comboItem = new QComboTableItem( userTable,accessRights);
    userTable->setItem(row,1,comboItem);

    list.remove(item);
    delete item;
  }
}

void ShareDlgImpl::removeAllowedUserBtnClicked()
{
  QMemArray<int>rows;

  int j=0;

  for (int i=0; i<userTable->numRows(); i++)
  {
    if (userTable->isRowSelected(i))
    {
      QTableItem* item = userTable->item(i,0);
      new KListViewItem(possibleUserListView, item->text());
      rows.resize(j+1);
      rows[j] = i;
      j++;
    }
  }

  userTable->removeRows(rows);
}

void ShareDlgImpl::saveUserTable()
{
  QStringList validUsers;
  QStringList writeList;
  QStringList readList;
  QStringList adminUsers;

  for (int i=0; i<userTable->numRows(); i++)
  {
    QTableItem* item = userTable->item(i,0);
    QComboTableItem* comboItem = static_cast<QComboTableItem*>(userTable->item(i,1));

    validUsers.append(item->text());

    switch (comboItem->currentItem())
    {
      case 1 : readList.append(item->text());break;
      case 2 : writeList.append(item->text());break;
      case 3 : adminUsers.append(item->text());break;
      default : break;
    }
  }

  _share->setValue("valid users", validUsers.join(","));
  _share->setValue("read list", readList.join(","));
  _share->setValue("write list", writeList.join(","));
  _share->setValue("admin users", adminUsers.join(","));


}

void ShareDlgImpl::tabChangedSlot(QWidget* w)
{

  // We are only interrested in the Hidden files tab
  if (HIDDENTABINDEX == tabs->indexOf(w))
     loadHiddenFilesView();

}

void ShareDlgImpl::loadHiddenFilesView()
{
  if (_fileView)
     return;


  _fileView = new HiddenFileView( this, _share );
  _fileView->load();


}

void ShareDlgImpl::accept()
{
	// Base settings

	assert(_share);
  
  if (!_share)
     return;

  if (homeChk->isChecked())
  	 _share->setName("homes");
	else
    _share->setName(shareNameEdit->text());


  _share->setValue("path",pathUrlRq->url() );


	_share->setValue("comment",commentEdit->text( ) );

  _share->setValue("available",availableBaseChk->isChecked( ) );
  _share->setValue("browseable",browseableBaseChk->isChecked( ) );
  _share->setValue("writeable", ! readOnlyBaseChk->isChecked( ) );
  _share->setValue("public",publicBaseChk->isChecked( ) );

  // User settings

  _share->setValue("invalid users",invalidUsersEdit->text( ) );
  _share->setValue("force user",forceUserCombo->currentText( ) );
  _share->setValue("force group",forceGroupCombo->currentText( ) );

  saveUserTable();

  // Filename settings
  
  _share->setValue("default case",defaultCaseCombo->currentText( ) );
  _share->setValue("case sensitive",caseSensitiveChk->isChecked( ) );
  _share->setValue("preserve case",preserveCaseChk->isChecked( ) );
  _share->setValue("short preserve case",shortPreserveCaseChk->isChecked( ) );
  _share->setValue("mangled names",mangledNamesChk->isChecked( ) );
  _share->setValue("mangle case",mangleCaseChk->isChecked( ) );
  _share->setValue("mangling char",manglingCharEdit->text( ) );

  _share->setValue("hide dot files",hideDotFilesChk->isChecked( ) );
  _share->setValue("strip dot",hideTrailingDotChk->isChecked( ) );
  _share->setValue("hide unreadable",hideUnreadableChk->isChecked( ) );
  _share->setValue("dos filemode",dosFilemodeChk->isChecked( ) );
  _share->setValue("dos filetimes",dosFiletimesChk->isChecked( ) );
  _share->setValue("dos filetime resolution",dosFiletimeResolutionChk->isChecked( ) );
  _share->setValue("delete readonly",deleteReadonlyChk->isChecked( ) );


//-  _share->setValue("read only",readOnlyChk->isChecked( ) );
//-  _share->setValue("guest ok",guestOkChk->isChecked( ) );
  _share->setValue("guest only",guestOnlyChk->isChecked( ) );
  _share->setValue("user only",userOnlyChk->isChecked( ) );
  _share->setValue("hosts allow",hostsAllowEdit->text( ) );
  _share->setValue("guest account",guestAccountEdit->text( ) );
  _share->setValue("hosts deny",hostsDenyEdit->text( ) );
  _share->setValue("force directory security mode",forceDirectorySecurityModeEdit->text( ) );
  _share->setValue("force directory mode",forceDirectoryModeEdit->text( ) );
  _share->setValue("force security mode",forceSecurityModeEdit->text( ) );

  _share->setValue("force create mode",forceCreateModeEdit->text( ) );
  _share->setValue("directory security mask",directorySecurityMaskEdit->text( ) );
  _share->setValue("directory mask",directoryMaskEdit->text( ) );
  _share->setValue("security mask",securityMaskEdit->text( ) );
  _share->setValue("create mask",createMaskEdit->text( ) );
  _share->setValue("inherit permissions",inheritPermissionsChk->isChecked( ) );
  
  // Advanced

  _share->setValue("blocking locks",blockingLocksChk->isChecked( ) );
  _share->setValue("fake oplocks",fakeOplocksChk->isChecked( ) );
  _share->setValue("locking",lockingChk->isChecked( ) );
  _share->setValue("level2 oplocks",level2OplocksChk->isChecked( ) );
  _share->setValue("posix locking",posixLockingChk->isChecked( ) );
  _share->setValue("strict locking",strictLockingChk->isChecked( ) );
  _share->setValue("share modes",shareModesChk->isChecked( ) );
  _share->setValue("oplocks",oplocksChk->isChecked( ) );

  _share->setValue("oplock contention limit",QString::number(oplockContentionLimitInput->value()));
  _share->setValue("strict sync",strictSyncChk->isChecked( ) );

  _share->setValue("max connections",QString::number(maxConnectionsInput->value()) );
  _share->setValue("write cache size",QString::number(writeCacheSizeInput->value()) );

  _share->setValue("sync always",syncAlwaysChk->isChecked( ) );
  _share->setValue("status",statusChk->isChecked( ) );

  // Hidden files
  if (_fileView)
      _fileView->save();

	KcmShareDlg::accept();
}

void ShareDlgImpl::homeChkToggled(bool b)
{
  shareNameEdit->setDisabled(b);
	pathUrlRq->setDisabled(b);

	if (b)
  {
  	shareNameEdit->setText("homes");
    pathUrlRq->setURL("");
    directoryPixLbl->setPixmap(DesktopIcon("folder_home",48));
		
  }
  else
  {
  	shareNameEdit->setText( _share->getName() );
	  pathUrlRq->setURL( _share->getValue("path") );
    directoryPixLbl->setPixmap(DesktopIcon("folder"));
  }
}


#include "sharedlgimpl.moc"
