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

#include <pwd.h>
#include <time.h>
#include <unistd.h>

#include <qfileinfo.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qstringlist.h>
#include <KStandardDirs>
#include <kdebug.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <knfsshare.h>
#include <ktemporaryfile.h>
#include <kprocess.h>
#include <kshell.h>
#include "nfsfile.h"

NFSFile::NFSFile(const KUrl & url, bool readonly)
{
  _lines.setAutoDelete(true);
  _entries.setAutoDelete(false);
  _url = url;
  _readonly = readonly;
}

NFSFile::~NFSFile()
{
}

void NFSFile::addEntry(NFSEntry *entry)
{
  _lines.append(entry);
  _entries.append(entry);
}

void NFSFile::removeEntry(NFSEntry *entry)
{
  _entries.remove(entry);
  _lines.remove(entry);
}

bool NFSFile::hasEntry(NFSEntry *entry)
{
  return 0 < _entries.contains(entry);
}


NFSEntry* NFSFile::getEntryByPath(const QString & path)
{
  if( path.isEmpty())
     return 0L;

  QString testPath = path.trimmed();
  if ( !testPath.endsWith('/') )
       testPath += '/';

  for (NFSEntry* entry = _entries.first(); entry; entry = _entries.next())
  {
    if (entry->path()==testPath)
       return entry;
  }

  return 0L;
}

bool NFSFile::removeEntryByPath(const QString & path) {
  NFSEntry* entry = getEntryByPath(path);
  if (!entry)
      return false;

  removeEntry(entry);
  return true;
}

EntryIterator NFSFile::getEntries()
{
  return EntryIterator(_entries);
}




bool NFSFile::load()
{
  QFile f(_url.path());

  if ( !f.open(QIODevice::ReadOnly) ) {
    kError() << "NFSFile::load: Could not open " << _url.path() << endl;
    return false;
  }

  _entries.clear();
  _lines.clear();

  QTextStream s( &f );

  bool continuedLine = false; // is true if the line before ended with a backslash
  QString completeLine;

  while ( !s.atEnd() )
  {
    QString currentLine = s.readLine().trimmed();

    if (continuedLine) {
      completeLine += currentLine;
      continuedLine = false;
    }
    else
      completeLine = currentLine;

    // is the line continued in the next line ?
    if ( completeLine.endsWith('\\') )
    {
      continuedLine = true;
      // remove the ending backslash
      completeLine.truncate( completeLine.length()-1 );
      continue;
    }

    // empty lines
    if (completeLine.isEmpty()) {
      _lines.append(new NFSEmptyLine());
      continue;
    }

    // comments
    if (completeLine.startsWith('#') ) {
      _lines.append(new NFSComment(completeLine));
      continue;
    }

    QString path;
    QString hosts;

    // Handle quotation marks
    if ( completeLine.startsWith('"') ) {
      int i = completeLine.indexOf('"',1);
      if (i == -1) {
        kError() << "NFSFile: Parse error: Missing quotation mark: "
                  << completeLine << endl;
        continue;
      }
      path = completeLine.mid(1,i-1);
      hosts = completeLine.mid(i+1);

    } else { // no quotation marks
      int i = completeLine.indexOf(' ');
      if (i == -1)
          i = completeLine.indexOf('\t');

      if (i == -1)
        path = completeLine;
      else {
        path = completeLine.left(i);
        hosts = completeLine.mid(i+1).trimmed();
      }
    }

    // normalize path
    if ( !path.endsWith('/') )
            path += '/';

    kDebug(5009) << "KNFSShare: Found path: '" << path << "'";
    NFSEntry *entry = new NFSEntry(path);
    QStringList hostList = QStringList::split(' ', hosts);

    if (hostList.isEmpty()) {
        NFSHost* host = new NFSHost("*");
        entry->addHost(host);
    } else {
      QStringList::iterator it;
      for ( it = hostList.begin(); it != hostList.end(); ++it ) {
         NFSHost* host = new NFSHost((*it).trimmed());
         entry->addHost(host);
         kDebug(5009) << "KNFSShare: Found host: " << (*it) << " name='"
                  << host->name << "'" << endl;
      }
    }

    kDebug(5009) << "KNFSShare: Found hosts: " << hosts << "'";
    this->addEntry(entry);
  }

  f.close();


  return true;

}

void NFSFile::saveTo(QTextStream * stream) {
  Q3PtrListIterator<NFSLine> it(_lines);

  NFSLine *line;
  while ( (line = it.current()) != 0 ) {
    ++it;
    *stream << line->toString() << endl;
  }
}

bool NFSFile::saveTo(const QString& fileName) {
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    QTextStream stream(&file);
    saveTo(&stream);
    file.close();
    return true;
}

bool NFSFile::save()
{
  if (QFileInfo(_url.path()).isWritable() ) {
    saveTo(_url.path());
  } else
  {

    KTemporaryFile tempFile;
    tempFile.open();
    saveTo(tempFile.fileName());

    QString command = QString("cp %1 %2")
        .arg(KShell::quoteArg( tempFile.fileName() ),
             KShell::quoteArg( _url.path() ));

    if (restartNFSServer)
      command +=" && exportfs -ra";

    KProcess proc;
    if (!QFileInfo(_url.path()).isWritable() || restartNFSServer)
      proc<<KStandardDirs::findExe("kdesu") << "-d" << "-c"<<command;
    else
      proc.setShellCommand(command);

    if (proc.execute()) {
      return false;
    }


  }
  return true;
}

