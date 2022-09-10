//===========================================
//  Lumina Desktop Source Code
//  Copyright (c) 2016, Ken Moore
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "getPage.h"

//Add any sub-pages here
#include "page_main.h"
#include "wallpaper/page_wallpaper.h"
#include "autostart/page_autostart.h"
#include "defaultapps/page_defaultapps.h"
#include "desktop/page_interface_desktop.h"
#include "panels/page_interface_panels.h"
#include "menu/page_interface_menu.h"
#include "locale/page_session_locale.h"
#include "options/page_session_options.h"
#include "soundtheme/page_soundtheme.h"

//Simplification function for creating a PAGEINFO structure
PAGEINFO Pages::PageInfo(QString ID, QString i_name, QString i_title, QString i_icon, QString i_comment, QString i_cat, QStringList i_sys, QStringList i_tags){
  PAGEINFO page;
  page.id = ID; page.name = i_name; page.title = i_title; page.icon = i_icon;
  page.comment = i_comment; page.category = i_cat; page.req_systems = i_sys;
  page.search_tags = i_tags;
  return page;
}

//List all the known pages
// **** Add new page entries here ****
QList<PAGEINFO> Pages::KnownPages(){
  // Valid Groups: ["appearance", "interface", "session", "user"]
  QList<PAGEINFO> list;
  //Reminder: <ID>, <name>, <title>, <icon>, <comment>, <category>, <server subsytem list>, <search tags>
  list << Pages::PageInfo("wallpaper", QObject::tr("Wallpaper"), QObject::tr("Wallpaper Settings"), "preferences-desktop-wallpaper",QObject::tr("Change background image(s)"), "appearance", QStringList(), QStringList() << "background" << "wallpaper" << "color" << "image");
  list << Pages::PageInfo("autostart", QObject::tr("Autostart"), QObject::tr("Startup Settings"), "preferences-system-session-services",QObject::tr("Automatically start applications or services"), "session", QStringList(), QStringList() << "apps" << "autostart" << "services" << "xdg" << "startup" << "session");
  list << Pages::PageInfo("defaultapps", QObject::tr("Applications"), QObject::tr("Mimetype Settings"), "preferences-desktop-default-applications",QObject::tr("Change default applications"), "session", QStringList(), QStringList() << "apps" << "default" << "services" << "xdg" << "session");
  list << Pages::PageInfo("interface-desktop", QObject::tr("Desktop"), QObject::tr("Desktop Plugins"), "preferences-desktop-icons",QObject::tr("Change what icons or tools are embedded on the desktop"), "interface", QStringList(), QStringList() << "desktop" << "plugins" << "embed" << "icons" << "utilities");
  list << Pages::PageInfo("interface-panel", QObject::tr("Panels"), QObject::tr("Panels and Plugins"), "emblem-generic",QObject::tr("Change any floating panels and what they show"), "interface", QStringList(), QStringList() << "desktop" << "toolbar" << "panel" << "floating" << "plugins");
  list << Pages::PageInfo("interface-menu", QObject::tr("Menu"), QObject::tr("Menu Plugins"), "emblem-generic",QObject::tr("Change what options are shown on the desktop context menu"), "interface", QStringList(), QStringList() << "desktop" << "menu" << "plugins" << "shortcuts");
  list << Pages::PageInfo("session-locale", QObject::tr("Localization"), QObject::tr("Locale Settings"), "preferences-desktop-locale",QObject::tr("Change the default locale settings for this user"), "user", QStringList(), QStringList() << "user"<<"locale"<<"language"<<"translations");
  list << Pages::PageInfo("session-options", QObject::tr("General Options"), QObject::tr("User Settings"), "emblem-system",QObject::tr("Change basic user settings such as time/date formats"), "user", QStringList(), QStringList() << "user"<<"settings"<<"time"<<"date"<<"icon"<<"reset"<<"numlock"<<"clock");
  list << Pages::PageInfo("soundtheme", QObject::tr("Sounds"), QObject::tr("Theme"), "audio-x-generic",QObject::tr("Change basic sound settings"), "session", QStringList(), QStringList() << "session"<<"settings"<<"sound"<<"theme");

  return list;
}

PageWidget* Pages::GetNewPage(QString id, QWidget *parent){
  //Find the page that matches this "id"
  PageWidget* page = 0;
  if(id=="wallpaper"){ page = new page_wallpaper(parent); }
  else if(id=="autostart"){ page = new page_autostart(parent); }
  else if(id=="defaultapps"){ page = new page_defaultapps(parent); }
  else if(id=="interface-desktop"){ page = new page_interface_desktop(parent); }
  else if(id=="interface-panel"){ page = new page_interface_panels(parent); }
  else if(id=="interface-menu"){ page = new page_interface_menu(parent); }
  else if(id=="session-locale"){ page = new page_session_locale(parent); }
  else if(id=="session-options"){ page = new page_session_options(parent); }
  else if(id=="soundtheme"){ page = new page_soundtheme(parent); }
  //Return the main control_panel page as the fallback/default
  if(page==0){ id.clear(); page = new page_main(parent); }
  page->setWhatsThis(id);
  return page;
}
