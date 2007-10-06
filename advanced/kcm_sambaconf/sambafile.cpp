/*
  Copyright (c) 2002-2004 Jan Schaefer <j_schaef@informatik.uni-kl.de>

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

#include <qfile.h>
#include <q3process.h>
#include <qfileinfo.h>
//Added by qt3to4:
#include <QTextStream>

#include <kconfig.h>
#include <kconfiggroup.h>
#include <kdebug.h>
#include <kio/job.h>
#include <k3process.h>
#include <kprocess.h>
#include <kshell.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <ktemporaryfile.h>
#include <ksambashare.h>

#include <pwd.h>
#include <time.h>
#include <unistd.h>

#include "sambafile.h"

#define FILESHARE_DEBUG 5009

SambaConfigFile::SambaConfigFile(SambaFile* sambaFile)
{
  Q3Dict<QString>(10,false);
  setAutoDelete(true);
  _sambaFile = sambaFile;
}

QString SambaConfigFile::getDefaultValue(const QString & name)
{
  SambaShare* defaults = _sambaFile->getTestParmValues();
  QString s = defaults->getValue(name,false,false);

  return s;
}

SambaShare* SambaConfigFile::addShare(const QString & name)
{
  SambaShare* newShare = new SambaShare(name,this);
  addShare(name,newShare);
  return newShare;
}


void SambaConfigFile::addShare(const QString & name, SambaShare* share)
{
  insert(name,share),
  _shareList.append(name);
}

void SambaConfigFile::removeShare(const QString & name)
{
  remove(name);
  _shareList.remove(name);
}


QStringList SambaConfigFile::getShareList()
{
  return _shareList;
}

SambaFile::SambaFile(const QString & _path, bool _readonly)
  : readonly(_readonly),
    changed(false),
    path(_path),
    localPath(_path),
    _sambaConfig(0),
    _testParmValues(0),
    _sambaVersion(-1),
    _tempFile(0)
{
}

SambaFile::~SambaFile()
{
  delete _sambaConfig;
  delete _testParmValues;
  delete _tempFile;

}

bool SambaFile::isRemoteFile() {
  return ! KUrl(path).isLocalFile();
}

/** No descriptions */
QString SambaFile::findShareByPath(const QString & path) const
{
  Q3DictIterator<SambaShare> it(*_sambaConfig);
  KUrl url(path);
  url.adjustPath(KUrl::RemoveTrailingSlash);

  for (  ; it.current(); ++it )
  {
    SambaShare* share = it.current();

    QString *s = share->find("path");
    if (s) {
        KUrl curUrl(*s);
        curUrl.adjustPath(KUrl::RemoveTrailingSlash);

        kDebug(5009) << url.path() << " =? " << curUrl.path();

        if (url.path() == curUrl.path())
            return it.currentKey();
    }
  }

  return QString();
}

bool SambaFile::save() {
  return slotApply();
}


bool SambaFile::slotApply()
{
  if (readonly) {
      kDebug(FILESHARE_DEBUG) << "SambaFile::slotApply: readonly=true";
      return false;
  }

  // If we have write access to the smb.conf
  // we simply save the values to it
  // if not we have to save the results in
  // a temporary file and copy it afterwards
  // over the smb.conf file with kdesu.
  if (QFileInfo(path).isWritable())
  {
    saveTo(path);
    changed = false;
    return true;
  }

  // Create a temporary smb.conf file
   delete _tempFile;
  _tempFile = new KTemporaryFile();

  if (!_tempFile->open() || !saveTo(_tempFile->fileName())) {
    kDebug(5009) << "SambaFile::slotApply: Could not save to temporary file";
    delete _tempFile;
    _tempFile = 0;
    return false;
  }

  QFileInfo fi(path);
  KUrl url(path);

  if (KUrl(path).isLocalFile()) {
    KProcess proc;
    kDebug(5009) << "SambaFile::slotApply: is local file!";

    QString suCommand=QString("cp %1 %2; rm %3")
              .arg(KShell::quoteArg(_tempFile->fileName()),
                   KShell::quoteArg(path),
                   KShell::quoteArg(_tempFile->fileName()));
    proc << "kdesu" << "-d" << suCommand;

    if (proc.execute()) {
        kDebug(5009) << "SambaFile::slotApply: saving to " << path << " failed!";
        //KMessageBox::sorry(0,i18n("Saving the results to %1 failed.",path));
        delete _tempFile;
        _tempFile = 0;
        return false;
    }
    else {
        changed = false;
        delete _tempFile;
        _tempFile = 0;
        kDebug(5009) << "SambaFile::slotApply: changes successfully saved!";
        return true;
    }
  } else {
    kDebug(5009) << "SambaFile::slotApply: is remote file!";
    KUrl srcURL;
    srcURL.setPath( _tempFile->fileName() );

    KIO::FileCopyJob * job =  KIO::file_copy( srcURL, url, -1, KIO::Overwrite  );
    connect( job, SIGNAL( result( KJob * ) ),
             this, SLOT( slotSaveJobFinished ( KJob * ) ) );
    return (job->error()==0);
  }

  return true;
}

  /**
  * Returns a name which isn't already used for a share
  **/
QString SambaFile::getUnusedName(const QString alreadyUsedName) const
{

  QString init = i18n("Unnamed");
  if (alreadyUsedName != QString())
    init = alreadyUsedName;

  QString s = init;

  int i = 2;

  while (_sambaConfig->find(s))
  {
    s = init+QString::number(i);
    i++;
  }

  return s;
}



SambaShare* SambaFile::newShare(const QString & name)
{
  if (_sambaConfig->find(name))
    return 0L;

  SambaShare* share = new SambaShare(name,_sambaConfig);
  _sambaConfig->addShare(name,share);

  changed = true;

  return share;

}

SambaShare* SambaFile::newShare(const QString & name, const QString & path)
{
  SambaShare* share = newShare(name);
  if (share)
  {
    share->setValue("path",path);
  }

  return share;
}

SambaShare* SambaFile::newPrinter(const QString & name, const QString & printer)
{
  SambaShare* share = newShare(name);

  if (share)
  {
    share->setValue("printable",true);
    share->setValue("printer name",printer);
  }

  return share;
}


/** No descriptions */
void SambaFile::removeShare(const QString & share)
{
  changed = true;

  _sambaConfig->removeShare(share);
}

void SambaFile::removeShare(SambaShare* share)
{
  removeShare(share->getName());
}

void SambaFile::removeShareByPath(const QString & path) {
  QString share = findShareByPath(path);
  removeShare(share);
}

/** No descriptions */
SambaShare* SambaFile::getShare(const QString & share) const
{
  SambaShare *s = _sambaConfig->find(share);

  return s;
}

/**
* Returns a list of all shared directories
**/
SambaShareList* SambaFile::getSharedDirs() const
{
  SambaShareList* list = new SambaShareList();

  Q3DictIterator<SambaShare> it(*_sambaConfig);

  for( ; it.current(); ++it )
  {
    if (!it.current()->isPrinter() &&
        it.current()->getName() != "global")
    {
      list->append(it.current());
    }
  }

  return list;
}

/**
* Returns a list of all shared printers
**/
SambaShareList* SambaFile::getSharedPrinters() const
{
  SambaShareList* list = new SambaShareList();

  Q3DictIterator<SambaShare> it(*_sambaConfig);

  for( ; it.current(); ++it )
  {
    if (it.current()->isPrinter())
      list->append(it.current());
  }

  return list;
}

int SambaFile::getSambaVersion() {
  if (_sambaVersion > -1)
    return _sambaVersion;

  K3Process testParam;
  testParam << "testparm";
  testParam << "-V";
  _parmOutput.clear();
  _sambaVersion = 2;

  connect( &testParam, SIGNAL(receivedStdout(K3Process*,char*,int)),
          this, SLOT(testParmStdOutReceived(K3Process*,char*,int)));



  if (testParam.start(K3Process::Block,K3Process::Stdout)) {
    if (_parmOutput.contains('3') )
      _sambaVersion = 3;
  }

  kDebug(5009) << "Samba version = " << _sambaVersion;

  return _sambaVersion;
}


SambaShare* SambaFile::getTestParmValues(bool reload)
{
  if (_testParmValues && !reload)
    return _testParmValues;


  K3Process testParam;
  testParam << "testparm";
  testParam << "-s";

  if (getSambaVersion() == 3)
     testParam << "-v";


  testParam << "/dev/null";
  _parmOutput.clear();

  connect( &testParam, SIGNAL(receivedStdout(K3Process*,char*,int)),
          this, SLOT(testParmStdOutReceived(K3Process*,char*,int)));

  if (testParam.start(K3Process::Block,K3Process::Stdout))
  {
    parseParmStdOutput();
  } else
    _testParmValues = new SambaShare(_sambaConfig);

  return _testParmValues;
}

void SambaFile::testParmStdOutReceived(K3Process *, char *buffer, int buflen)
{
  _parmOutput+=QString::fromLatin1(buffer,buflen);
}

void SambaFile::parseParmStdOutput()
{

  QTextIStream s(&_parmOutput);

  delete _testParmValues;
  _testParmValues = new SambaShare(_sambaConfig);

  QString section="";

  while (!s.atEnd())
  {
    QString line = s.readLine().trimmed();

    // empty lines
    if (line.isEmpty())
      continue;

    // comments
    if ('#' == line[0])
      continue;

    // sections
    if ('[' == line[0])
    {
      // get the name of the section
      section = line.mid(1,line.length()-2).toLower();
      continue;
    }

    // we are only interested in the global section
    if (section != KGlobal::staticQString("global"))
      continue;

    // parameter
    // parameter
    int i = line.indexOf('=');

    if (i>-1) {
      QString name = line.left(i).trimmed();
      QString value = line.mid(i+1).trimmed();
      _testParmValues->setValue(name,value,false,false);
    }

  }



}

/**
* Try to find the samba config file position
* First tries the config file, then checks
* several common positions
* If nothing is found returns QString()
**/
QString SambaFile::findSambaConf()
{
#ifdef __GNUC__
#warning FIXME
#endif
//     return KSambaShare::componentData().smbConfPath();
    return QString();
}

void SambaFile::slotSaveJobFinished( KJob * job ) {
  delete _tempFile;
  _tempFile = 0;
}

void SambaFile::slotJobFinished( KJob * job )
{
  if (job->error())
    emit canceled( job->errorString() );
  else
  {
    openFile();
    emit completed();
  }
}

bool SambaFile::load()
{
  if (path.isNull() || path.isEmpty())
      return false;

  kDebug(FILESHARE_DEBUG) << "SambaFile::load: path=" << path;
  KUrl url(path);

  if (!url.isLocalFile()) {
    KTemporaryFile tempFile;
    tempFile.open();
    localPath = tempFile.fileName();
    KUrl destURL;
    destURL.setPath( localPath );
    KIO::FileCopyJob * job =  KIO::file_copy( url, destURL, 0600, KIO::Overwrite );
//    emit started( d->m_job );
    connect( job, SIGNAL( result( KJob * ) ), this, SLOT( slotJobFinished ( KJob * ) ) );
    return true;
  } else {
    localPath = path;
    bool ret = openFile();
    if (ret)
        emit completed();
    return ret;
  }
}

bool SambaFile::openFile() {

  QFile f(localPath);

  if (!f.open(QIODevice::ReadOnly)) {
    //throw SambaFileLoadException(QString("<qt>Could not open file <em>%1</em> for reading.</qt>").arg(path));
    return false;
  }

  QTextStream s(&f);

  delete _sambaConfig;

  _sambaConfig = new SambaConfigFile(this);

  SambaShare *currentShare = 0L;
  bool continuedLine = false; // is true if the line before ended with a backslash
  QString completeLine;
  QStringList comments;
  while (!s.atEnd())
  {
    QString currentLine = s.readLine().trimmed();
    if (continuedLine)
    {
      completeLine += currentLine;
      continuedLine = false;
    } else
      completeLine = currentLine;
    // is the line continued in the next line ?
    if ( !completeLine.isEmpty() && completeLine[completeLine.length()-1] == '\\' )
    {
      continuedLine = true;
      // remove the ending backslash
      completeLine.truncate( completeLine.length()-1 );
      continue;
    }

    // comments or empty lines
    if (completeLine.isEmpty() ||
        '#' == completeLine[0] ||
        ';' == completeLine[0])
    {
      comments.append(completeLine);
      continue;
    }


    // sections
    if (!completeLine.isEmpty() && '[' == completeLine[0])
    {
      // get the name of the section
      QString section = completeLine.mid(1,completeLine.length()-2);
      currentShare = _sambaConfig->addShare(section);
      currentShare->setComments(comments);
      comments.clear();

      continue;
    }

    // parameter
    int i = completeLine.indexOf('=');

    //completeLine is not empty
    if (i>-1)
    {
      QString name = completeLine.left(i).trimmed();
      QString value = completeLine.mid(i+1).trimmed();

      if (currentShare)
      {
        currentShare->setComments(name,comments);
        currentShare->setValue(name,value,true,true);

        comments.clear();
      }
    }
  }

  f.close();

  // Make sure there is a global share
  if (!getShare("global")) {
     _sambaConfig->addShare("global");
  }

  return true;
}

bool SambaFile::saveTo(const QString & path)
{
  QFile f(path);

  if (!f.open(QIODevice::WriteOnly))
    return false;

  QTextStream s(&f);

  QStringList shareList = _sambaConfig->getShareList();

  for ( QStringList::Iterator it = shareList.begin(); it != shareList.end(); ++it )
  {
    SambaShare* share = _sambaConfig->find(*it);

    // First add all comments of the share to the file
    QStringList comments = share->getComments();
    for ( QStringList::Iterator cmtIt = comments.begin(); cmtIt != comments.end(); ++cmtIt )
    {
      s << *cmtIt << endl;

      kDebug(5009) << *cmtIt;
    }

    // If there are no lines before the section add
    // a blank line
    if (comments.isEmpty())
      s << endl;

    // Add the name of the share / section
    s << "[" << share->getName() << "]" << endl;

    // Add all options of the share
    QStringList optionList = share->getOptionList();

    for ( QStringList::Iterator optionIt = optionList.begin(); optionIt != optionList.end(); ++optionIt )
    {

      // Add the comments of the option
      comments = share->getComments(*optionIt);
      for ( QStringList::Iterator cmtIt = comments.begin(); cmtIt != comments.end(); ++cmtIt )
      {
        s << *cmtIt << endl;
      }

      // Add the option
      s << *optionIt << " = " << *share->find(*optionIt) << endl;
    }


  }

  f.close();

  return true;
}


SambaConfigFile* SambaFile::getSambaConfigFile(KConfig* config)
{
  QStringList groups = config->groupList();

  SambaConfigFile* samba = new SambaConfigFile(this);

  for ( QStringList::Iterator it = groups.begin(); it != groups.end(); ++it )
  {
    QMap<QString,QString> entries = config->entryMap(*it);

    SambaShare *share = new SambaShare(*it,samba);
    samba->insert(*it,share);

    for (QMap<QString,QString>::Iterator it2 = entries.begin(); it2 != entries.end(); ++it2 )
    {
      if (!it2.value().isEmpty())
          share->setValue(it2.key(),QString(it2.value()),false,false);
    }

  }

  return samba;

}

KConfig* SambaFile::getSimpleConfig(SambaConfigFile* sambaConfig, const QString & path)
{
  KConfig *config = new KConfig(path, KConfig::OnlyLocal);

  Q3DictIterator<SambaShare> it(*sambaConfig);

  for ( ; it.current(); ++it )
  {
    SambaShare* share = it.current();

    KConfigGroup cg(config, it.currentKey());

    Q3DictIterator<QString> it2(*share);

    for (; it2.current(); ++it2 )
    {
      cg.writeEntry(it2.currentKey(), *it2.current());
    }

  }

  return config;
}

#include "sambafile.moc"
