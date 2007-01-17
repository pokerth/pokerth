/***************************************************************************
 *   Copyright (C) 2006 by Felix Hammer   *
 *   f.hammer@web.de   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "configfile.h"

ConfigFile::ConfigFile()
{
}


ConfigFile::~ConfigFile()
{
}


QString ConfigFile::readConfig(QString varName, QString defaultvalue)
{
  
	QDir configDir;
	configDir.setPath(QDir::home().absPath()+"/.pokerth/");
	
	QFile configFile (configDir.absPath()+"/pokerth.conf");
	
	if ( !configFile.exists() ) {
		configFile.open( QIODevice::WriteOnly );   
		QTextStream stream( &configFile );
		stream << "##################################################################################### \n";
		stream << "#                                                                                   # \n";
		stream << "#         This is the config-file for Pokerth - Please do not edit                  # \n";
		stream << "#                                                                                   # \n";
		stream << "##################################################################################### \n";
		stream << "\n";
		stream << "numberofplayers=5\n";
		stream << "startcash=2000\n";
		stream << "smallblind=10\n";
		stream << "gamespeed=4\n";
		stream << "showgamesettingsdialogonnewgame=1\n";
		stream << "myname=Human Player\n";
		stream << "oppponent1name=Player 1\n";
		stream << "oppponent2name=Player 2\n";
		stream << "oppponent3name=Player 3\n";
		stream << "oppponent4name=Player 4\n";
		configFile.close();
	} 
	
	QString tempstring;
	QString line;
	int foundvarname(0);
	
	configFile.open( QIODevice::ReadOnly );  
	QTextStream readStream( &configFile );
	while ( !readStream.atEnd() ) {
		line = readStream.readLine();
		if ( line.section( "=", 0, 0 ) == varName ) {
		tempstring = line.section( "=", 1, 1 );
		foundvarname++;
		}
	}
	configFile.close();
	
	if (foundvarname == 0) {

		QDir configDir;
		configDir.setPath(QDir::home().absPath()+"/.pokerth/config/");
		
		QFile configFile (configDir.absPath()+"/pokerth.conf");
		
		QStringList lines;
		QString line;
		QString listtemp;
	
		if ( configFile.open( QIODevice::ReadOnly ) ) {
		QTextStream stream( &configFile );
		while ( !stream.atEnd() ) {
			line = stream.readLine();
			lines += line;
	
		}
		configFile.close();
		}
	
		if ( configFile.open( QIODevice::WriteOnly ) ) {
			
			QTextStream stream( &configFile );
			
			for ( QStringList::Iterator it = lines.begin(); it != lines.end(); ++it ) {
			stream << *it << "\n";
			}
			stream << varName+"="+defaultvalue+"\n";
			configFile.close();
		}
	
		tempstring = defaultvalue;    

	}

 return tempstring;
    
 }

void ConfigFile::writeConfig(QString setVarName, QString setVarCont)
 {
    QDir configDir;
    configDir.setPath(QDir::home().absPath()+"/.pokerth/");
    QFile configFile (configDir.absPath()+"/pokerth.conf");
 
    QStringList lines;
    QString line;
    QString listtemp;
        
    if ( configFile.open( QIODevice::ReadOnly ) ) {
       QTextStream stream( &configFile );
       while ( !stream.atEnd() ) {
          line = stream.readLine();
          lines += line;
       }
       configFile.close();
    }
    
    listtemp = lines.grep(QRegExp("^"+setVarName)).join("");
    lines.gres(listtemp,setVarName+"="+setVarCont);
    
    if ( configFile.open( QIODevice::WriteOnly ) ) {
        QTextStream stream( &configFile );
        for ( QStringList::Iterator it = lines.begin(); it != lines.end(); ++it ) {
	   stream << *it << "\n";
           }
	configFile.close();
    }
        
}
