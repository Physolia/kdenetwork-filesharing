/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename slots use Qt Designer which will
** update this file, preserving your code. Create an init() slot in place of
** a constructor, and a destroy() slot in place of a destructor.
*****************************************************************************/

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
 *  along with KSambaPlugin; if not, write to the Free Software                     *
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA  *
 *                                                                            *
 ******************************************************************************/

void KcmInterface::init()
{
    addShareBtn->setIconSet(SmallIcon("filenew"));
    editShareBtn->setIconSet(SmallIcon("edit"));
    removeShareBtn->setIconSet(SmallIcon("editdelete"));
    editDefaultShareBtn->setIconSet(SmallIcon("queue"));
    
    addPrinterBtn->setIconSet(SmallIcon("filenew"));
    editPrinterBtn->setIconSet(SmallIcon("edit"));
    removePrinterBtn->setIconSet(SmallIcon("editdelete"));
    editDefaultPrinterBtn->setIconSet(SmallIcon("print_class"));
    
}



void KcmInterface::changedSlot()
{
    emit changed();
}








void KcmInterface::securityLevelCombo_activated( int i )
{
   passwordServerEdit->setDisabled(i<2);
   allowGuestLoginsChk->setDisabled(i==0);
   
    
}


void KcmInterface::shareRadio_clicked()
{
    securityLevelHelpLbl->setText("Use the <i>share</i> security level if you have a home network "
			         "or a small office network.<br> It allows everyone to read the list "
		                     "of all your shared directories and printers before a login is required.");
}


void KcmInterface::userRadio_clicked()
{
    securityLevelHelpLbl->setText("Use the <i>user</i> security level if you have a bigger network "
			         "and you don't want to allow everyone to read your list of shared "
		                     "direcories and printers without a login.<p>"
			"If you want to run your Samba server as a <b>Primary Domain controller</b> (PDC) "
			"you also have to set this option.");
}

void KcmInterface::serverRadio_clicked()
{
    securityLevelHelpLbl->setText("Use the <i>server</i> security level if you have a big network "
			         "and the samba server should validate the username/password "
			         "by passing it to another SMB server, such as an NT box.");
}


void KcmInterface::domainRadio_clicked()
{
 securityLevelHelpLbl->setText("Use the <i>domain</i> security level if you have a big network "
			      "and the samba server should validate the username/password "
			      "by passing it to a Windows NT Primary or Backup Domain Controller.");
}
			      
			



void KcmInterface::KURLLabel1_leftClickedURL()
{
    KProcess* p = new KProcess();
    
    *p << "konqueror";
    *p << "man:smb.conf";
    p->start();
    
    
    
}




void KcmInterface::lmAnnounceCombo_activated( int i)
{
    lmIntervalSpin->setEnabled(i==0);
}




void KcmInterface::allowGuestLoginsChk_toggled( bool b)
{
    int i = 0;
    if (b) 
	i = 1;
    
    mapToGuestCombo->setCurrentItem(i);
}


void KcmInterface::mapToGuestCombo_activated( int i)
{
    allowGuestLoginsChk->setChecked(i>0);
}



