"""
"""

__all__ = [
    # public names
    "parseAddonXml",
    "parseAddonElt",
    "createItemListFromXml",
    "ListItemFromXML"
    ]


# Modules general
import sys

from traceback import print_exc

import xml.etree.cElementTree as ET

# Modules custom
try:
    from Item import TYPE_ADDON, TYPE_ADDON_MUSIC, TYPE_ADDON_PICTURES, TYPE_ADDON_PROGRAMS, TYPE_ADDON_VIDEO, TYPE_ADDON_MODULE, TYPE_ADDON_REPO, TYPE_ADDON_SCRIPT
except:
    print_exc()


# Types of Extension
TYPE_EXT_UI_SKIN                = "xbmc.gui.skin"
TYPE_EXT_REPO                   = "xbmc.addon.repository"
TYPE_EXT_SERVICE                = "xbmc.service"
TYPE_EXT_SCRAPER_ALBUMS         = "xbmc.metadata.scraper.albums"
TYPE_EXT_SCRAPER_ARTISTS        = "xbmc.metadata.scraper.artists"
TYPE_EXT_SCRAPER_MOVIES         = "xbmc.metadata.scraper.movies"
TYPE_EXT_SCRAPER_MUSICVIDEOS    = "xbmc.metadata.scraper.musicvideos"
TYPE_EXT_SCRAPER_TVSHOWS        = "xbmc.metadata.scraper.tvshows"
TYPE_EXT_SCRAPER_LIB            = "xbmc.metadata.scraper.library"
TYPE_EXT_UI_SCREENSAVER         = "xbmc.ui.screensaver"
TYPE_EXT_PLAYER_MUSICVIZ        = "xbmc.player.musicviz"
TYPE_EXT_PLUGINSOURCE           = "xbmc.python.pluginsource"
TYPE_EXT_SCRIPT                 = "xbmc.python.script"
TYPE_EXT_SCRIPT_WEATHER         = "xbmc.python.weather"
TYPE_EXT_SCRIPT_SUBTITLE        = "xbmc.python.subtitles"
TYPE_EXT_SCRIPT_LYRICS          = "xbmc.python.lyrics"
TYPE_EXT_SCRIPT_MODULE          = "xbmc.python.module"
TYPE_EXT_SCRIPT_LIB             = "xbmc.python.library"

supportedExtList = [ TYPE_EXT_REPO,
                     TYPE_EXT_PLUGINSOURCE,
                     TYPE_EXT_SCRIPT,
                     TYPE_EXT_SCRIPT_WEATHER,
                     TYPE_EXT_SCRIPT_SUBTITLE,
                     TYPE_EXT_SCRIPT_LYRICS,
                     TYPE_EXT_SCRIPT_LIB,
                     TYPE_EXT_SCRIPT_MODULE,
                   ]

scriptExtList = [ TYPE_EXT_SCRIPT_WEATHER,
                  TYPE_EXT_SCRIPT_SUBTITLE,
                  TYPE_EXT_SCRIPT_LYRICS,
                  TYPE_EXT_SCRIPT_LIB,
                  TYPE_EXT_SCRIPT,
                ]

def parseAddonXml( xmlData, itemInfo ):
    """
    Get Item Info from addon.xml and set itemInfo object
    Look at http://wiki.xbmc.org/index.php?title=Add-ons_for_XBMC_(Developement) for XML format description
    """
    # id
    # name
    # type
    # version
    # author
    # disclaimer
    # summary
    # description
    # icon
    # fanart
    # changelog
    # library: path of python script
    # raw_item_sys_type: file | archive | dir
    # raw_item_path
    # install_path
    # extracted_path
    # provides
    # required_lib

    status = 'OK'
    try:
        if ( xmlData ):
            xmlElt = ET.parse( xmlData ).getroot()
            status = parseAddonElt( xmlElt, itemInfo )
    except:
        status = 'ERROR'
        print_exc()

    return status




def parseAddonElt( addonElt, itemInfo ):
    """
    Get Item Info from addon.xml and set itemInfo object
    """
    # id
    # name
    # type
    # version
    # author
    # disclaimer
    # summary
    # description
    # icon
    # fanart
    # changelog
    # library: path of python script file i.e default.py
    # raw_item_sys_type: file | archive | dir
    # raw_item_path
    # install_path
    # extracted_path
    # provides
    # required_lib

    status = 'OK'
    try:
        if ( addonElt ):
            libPoint = None
            itemInfo [ "id" ]      = addonElt.attrib.get( "id" )
            itemInfo [ "name" ]    = addonElt.attrib.get( "name" )
            itemInfo [ "version" ] = addonElt.attrib.get( "version" )
            itemInfo [ "author" ]  = addonElt.attrib.get( "provider-name" )
            itemInfo [ "type" ]    = TYPE_ADDON # Unsupported type of addon (default value)
            extensions = addonElt.findall("extension")
            if extensions:
                for extension in extensions:
                    point = extension.attrib.get( "point" )
                    if point in supportedExtList:
                        # Map the type
                        itemInfo [ "type" ] = _getType(itemInfo [ "id" ], point)
                        if extension.attrib.get("library"):
                            # Info on lib
                            itemInfo [ "library" ] = extension.attrib.get( "library" )
                            itemInfo [ "provides" ] = extension.findtext( "provides" )
                            libPoint = extension.attrib.get( "point" )

                        # Get repo info in case of repo:
#                        <extension point="xbmc.addon.repository"
#                            name="Passion-XBMC Add-on Repository">
#                            <info compressed="true">http://passion-xbmc.org/addons/addons.php</info>
#                            <checksum>http://passion-xbmc.org/addons/addons.xml.md5</checksum>
#                            <datadir zip="true">http://passion-xbmc.org/addons/Download.php</datadir>
#                        </extension>
                        if point == TYPE_EXT_REPO:
                            itemInfo [ "repo_url" ] = extension.findtext( "info" )
                            datadir = extension.find( "datadir" )
                            itemInfo [ "repo_datadir" ] = datadir.text
                            zip = datadir.attrib.get( "zip" )
                            if zip == "true":
                                itemInfo [ "repo_format" ] = "zip"
                            else:
                                itemInfo [ "repo_format" ] = "dir"

                    elif point == "xbmc.addon.metadata":
                        # Metadata
                        itemInfo [ "platform" ]    = extension.findtext( "platform" )
                        itemInfo [ "nofanart" ]    = extension.findtext( "nofanart" )
                        itemInfo [ "description" ] = extension.findtext( "description" )
                        itemInfo [ "disclaimer" ]  = extension.findtext( "disclaimer" )

                        #TODO: Check case where tag is not present: what is returned?

            requires = addonElt.find("requires")
            requiredModuleList = []
            if requires:
                modules2import = requires.findall("import")
                for module in modules2import:
                    addonId = module.attrib.get( "addon" )
                    if module.attrib.get( "addon" ) != 'xbmc.python': # we ignore default python lib
                        moduleInfo = {}
                        moduleInfo [ "id" ]      = addonId
                        moduleInfo [ "version" ] = module.attrib.get( "version" )
                        requiredModuleList.append( moduleInfo )
            itemInfo [ "required_lib" ] = requiredModuleList



            if itemInfo [ "type" ] == TYPE_ADDON:
                status = 'NOT_SUPPORTED'
        else:
            print "addonElt not defined"
            #status = 'ERROR'

    except:
        #status = 'ERROR'
        print_exc()

    return status

def _getType(id, extension):
    """
    Determine the Type of the addon
    """
    type = TYPE_ADDON # Unsupported type of addon
    if  extension == TYPE_EXT_PLUGINSOURCE:
        # Plugin: we need to check the addons id
        if TYPE_ADDON_MUSIC in id:
            type = TYPE_ADDON_MUSIC
        elif TYPE_ADDON_PICTURES in id:
            type = TYPE_ADDON_PICTURES
        elif TYPE_ADDON_PROGRAMS in id:
            type = TYPE_ADDON_PROGRAMS
        elif TYPE_ADDON_VIDEO in id:
            type = TYPE_ADDON_VIDEO
    elif extension == TYPE_EXT_SCRIPT_MODULE:
        type = TYPE_ADDON_MODULE
    elif extension == TYPE_EXT_REPO:
        type = TYPE_ADDON_REPO
    elif extension in scriptExtList:
        type = TYPE_ADDON_SCRIPT
    return type



def createItemListFromXml( xmlData ):
    """
    Create and return the list of addons from XML data
    Returns list and name of the list
   """
    status = 'OK'
    list = []

    try:
        if ( xmlData ):
            xmlElt = ET.parse( xmlData ).getroot() # root: <addons>
            if ( xmlElt ):
                addons = xmlElt.findall("addon")
                for addon in addons:
                    # dictionary to hold addon info
                    itemInfo = {}
                    status = parseAddonElt( addon, itemInfo )
                    if status == 'OK':
                        list.append(itemInfo)
    except:
        status = 'ERROR'
        print_exc()

    return status, list

class ListItemFromXML:
    currentParseIdx = 0
    addons = []
    def __init__( self, xmlData ):
        try:
            if ( xmlData ):
                rootXmlElt = ET.parse( xmlData ).getroot() # root: <addons>

                if ( rootXmlElt ):
                    self.addons = rootXmlElt.findall("addon")
        except:
            status = 'ItemList::__init__: ERROR'
            print_exc()


    def _parseAddonElement(self, addonElt, itemInfo):
        return parseAddonElt( addonElt, itemInfo )


    def getNextItem(self):
        result = None
        if len(self.addons) > 0 and self.currentParseIdx < len(self.addons):
            itemInfo = {}
            status = self._parseAddonElement( self.addons[self.currentParseIdx], itemInfo )
            self.currentParseIdx = self.currentParseIdx + 1
            result = itemInfo
        return result


