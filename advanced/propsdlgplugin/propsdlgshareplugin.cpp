/*
  Copyright (c) 2004 Jan Schaefer <j_schaef@informatik.uni-kl.de>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#include <qstring.h>
#include <kvbox.h>
#include <qlayout.h>
#include <qlabel.h>
#include <QProcess>

#include <kgenericfactory.h>
#include <kdebug.h>
#include <kpushbutton.h>
#include <kfileshare.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <kdialog.h>
#include <kglobal.h>

#include "propertiespage.h"
#include "propsdlgshareplugin.h"

typedef KGenericFactory<PropsDlgSharePlugin, KPropertiesDialog> PropsDlgSharePluginFactory;

K_EXPORT_COMPONENT_FACTORY( fileshare_propsdlgplugin,
                            PropsDlgSharePluginFactory("fileshare_propsdlgplugin") )

class PropsDlgSharePlugin::Private                            
{
  public:
    PropertiesPage* page; 
};
                            
PropsDlgSharePlugin::PropsDlgSharePlugin( KPropertiesDialog *dlg,
                    const QStringList & )
  : KPropertiesDialogPlugin(dlg), d(0)
{
  KGlobal::locale()->insertCatalog("kfileshare");                            
                            
  if (KFileShare::shareMode() == KFileShare::Simple) {     
      kDebug(5009) << "PropsDlgSharePlugin: Sharing mode is simple. Aborting." << endl;
      return;
  }

  KVBox *vbox = new KVBox();
  properties->addPage(vbox, i18n("&Share"));
  properties->setFileSharingPage(vbox);

  if (KFileShare::authorization() == KFileShare::UserNotAllowed) {
  
        QWidget* widget = new QWidget( vbox );
        QVBoxLayout * vLayout = new QVBoxLayout( widget );
        vLayout->setSpacing( KDialog::spacingHint() );
        vLayout->setMargin( 0 );
        
        
        if (KFileShare::sharingEnabled()) {
          vLayout->addWidget(
              new QLabel( i18n("You need to be authorized to share directories."), 
                          widget ));
        } else {
          vLayout->addWidget(
              new QLabel( i18n("File sharing is disabled."), widget));
        }                    
                    
        KPushButton* btn = new KPushButton( i18n("Configure File Sharing..."), widget );
        connect( btn, SIGNAL( clicked() ), SLOT( slotConfigureFileSharing() ) );
        btn->setDefault(false);
        QHBoxLayout* hBox = new QHBoxLayout( (QWidget *)0L );
        hBox->addWidget( btn, 0, Qt::AlignLeft );
        vLayout->addLayout(hBox);
        vLayout->addStretch( 10 ); // align items on top
        return;
  }                    
       

  d = new Private();
  
  d->page = new PropertiesPage(vbox, properties->items(),false);
  connect(d->page, SIGNAL(changed()), 
          this, SIGNAL(changed()));
  
  kDebug(5009) << "Fileshare properties dialog plugin loaded" << endl;
  
}                            

void PropsDlgSharePlugin::slotConfigureFileSharing()
{
    QStringList lst;
    lst<<"kcmshell" << "fileshare";
    QProcess::startDetached(KStandardDirs::findExe("kdesu"),lst);
}


PropsDlgSharePlugin::~PropsDlgSharePlugin()
{
  delete d;
}

void PropsDlgSharePlugin::applyChanges() 
{
  if (!d->page->save()) {
//    KMessageBox::sorry(d->page,
//                i18n("Saving the changes failed"));

    properties->abortApplying();
  }
}


#include "propsdlgshareplugin.moc"

