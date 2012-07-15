## xbmcaddon emulator module for xbmc4xbox

__all__ = [ "Addon" ]
__author__ = "nuka1195"

import sys
# set this here, hopefully sys.argv will be more accurate the faster we have it
sysargv = sys.argv
import os
import xbmc
from xml.dom.minidom import parseString


class Addon:
    """
        Addon(id) -- Creates a new Addon class.

        id          : string - id of the addon.

        example:
         - self.Addon = xbmcaddon.Addon(id="script.xbmc.lyrics")
    """
    # dictionary to hold addon info
    _info = {}
    
    def __init__( self, id ):
        """
            Initializer for passing the addon's id and setting addon info.
        """
        # get root dir
        cwd = self._get_root_dir( sysargv[0] )
        xbmc.log( "xbmcaddon: trying " + cwd, xbmc.LOGDEBUG )
        # parse addon.xml and set all addon info
        self._set_addon_info( xbmc.translatePath( cwd ), id )

        #check to make sure we are in the right plugin dir
        if self._info.get( 'id', '' ) != id:
            #try using id to find correct dir
            parts = id.split( '.' )
            cwd = "plugin://%s/%s" % ( parts[ 1 ], parts[ 2 ] )
            xbmc.log( "xbmcaddon: trying " + cwd, xbmc.LOGDEBUG )
            # parse addon.xml and reset all addon info
            self._set_addon_info( xbmc.translatePath( cwd ), id )

        #have we got the right dir now?
        if self._info.get( 'id', '' ) != id:
            #walk plugin directories to try to find correct one
            base_path = os.path.dirname( xbmc.translatePath( cwd ) ) + os.sep
            for directory in os.listdir(base_path):
                cwd = "plugin://%s/%s" % ( parts[ 1 ], directory )
                xbmc.log( "xbmcaddon: trying " + cwd, xbmc.LOGDEBUG )
                # parse addon.xml and reset all addon info
                self._set_addon_info( xbmc.translatePath( cwd ), id )
                #surely we must have found it by now
                if self._info.get( 'id', '' ) == id:
                    break
                    
        # get settings and language methods
        self._get_methods( cwd )
        xbmc.log( "xbmcaddon: using " + cwd, xbmc.LOGDEBUG )

    def _get_root_dir( self, path ):
        # get current working directory, we need to reset sys.argv[ 0 ] to a plugin for weather plugins as they aren't run as plugins, but they are categorized as plugins
        cwd = os.path.dirname( xbmc.validatePath( path.replace( "Q:\\plugins\\weather\\", "plugin://weather/" ) ) )
        # check if we're at root folder of addon
        if ( not os.path.isfile( os.path.join( xbmc.translatePath( cwd ), "addon.xml" ) ) ):
            # we're not at root, assume resources/lib/
            cwd = os.path.dirname( os.path.dirname( cwd ) )
        # return result
        return cwd

    def _get_methods( self, cwd ):
        # language module
        self._language_ = xbmc.Language( cwd ).getLocalizedString
        # settings module, try catch necessary as not all scripts have settings
        try:
            self._settings_ = xbmc.Settings( cwd )
        except:
            self._settings_ = None

    def _set_addon_info( self, cwd, id ):
        # get source
        try:
            xml_file = os.path.join( cwd, "addon.xml" )
            xml = open( xml_file, "r" ).read()
            # parse source
            dom = parseString( xml )
            # get main element
            item = dom.getElementsByTagName( "addon" )[ 0 ]
            # clear out old info
            self._info = {}
            # set info
            self._info[ "id" ] = item.getAttribute( "id" )
            self._info[ "name" ] = item.getAttribute( "name" )
            self._info[ "version" ] = item.getAttribute( "version" )
            self._info[ "author" ] = item.getAttribute( "provider-name" )
            for extension in dom.getElementsByTagName( "extension" ):
                # get library and type
                if ( extension.hasAttribute( "library" ) ):
                    self._info[ "library" ] = extension.getAttribute( "library" )
                    self._info[ "type" ] = extension.getAttribute( "point" )
                # get any meta data
                if ( extension.getAttribute( "point" ) == "xbmc.addon.metadata" ):
                    locale = xbmc.getRegion( "locale" )
                    for metatype in [ "disclaimer", "summary", "description" ]:
                        metadict = { "en": "" }
                        for metadata in extension.getElementsByTagName( metatype ):
                            if ( metadata.getAttribute( "lang" ) != "" ):
                                try:
                                    metadict[ metadata.getAttribute( "lang" ) ] = metadata.firstChild.data
                                except:
                                    metadict[ metadata.getAttribute( "lang" ) ] = ""
                            else:
                                try:
                                    metadict[ "en" ] = metadata.firstChild.data
                                except:
                                    metadict[ "en" ] = ""
                        if ( metadict.has_key( locale ) ):
                            self._info[ metatype ] = metadict[ locale ]
                        else:
                            self._info[ metatype ] = metadict[ "en" ]
            # reset this to default.py as that's what xbox uses
            self._info[ "library" ] = "default.py"
            self._info[ "path" ] = cwd
            self._info[ "libpath" ] = os.path.join( cwd, self._info[ "library" ] )
            self._info[ "icon" ] = os.path.join( cwd, "default.tbn" )
            self._info[ "fanart" ] = os.path.join( cwd, "fanart.jpg" )
            self._info[ "changelog" ] = os.path.join( cwd, "changelog.txt" )
            if ( cwd.startswith( "Q:\\plugins" ) ):
                self._info[ "profile" ] = "special://profile/plugin_data/%s/%s" % ( os.path.basename( os.path.dirname( cwd ) ), os.path.basename( cwd ), )
            else:
                self._info[ "profile" ] = "special://profile/script_data/%s" % ( os.path.basename( cwd ), )
            # cleanup
            dom.unlink()
        except:
            xbmc.log( "xbmcaddon: could not open " + xml_file, xbmc.LOGDEBUG )

    def getAddonInfo( self, id ):
        """
            getAddonInfo(id) -- Returns the value of an addon property as a string.

            id        : string - id of the property you want returned.

            *values for id: author, changelog, description, disclaimer, fanart. icon, id, libpath,
                            library, name, path, profile, stars, summary, type, version

            example:
              - profile_path = self.Addon.getAddonInfo(id="profile")
        """
        return self._info[ id.lower() ]

    def getLocalizedString( self, id ):
        """
            getLocalizedString(id) -- Returns the localized string as a unicode object.

            id             : integer - id# of the string you want to localize.

            example:
              - locstr = self.Addon.getLocalizedString(id=30000)
        """
        return self._language_( id )

    def getSetting( self, id ):
        """
            getSetting(id) -- Returns the value of a setting as a unicode object.

            id        : string - id of the setting you want returned.

            example:
              - username = self.Addon.getSetting(id="username")
        """
        return self._settings_.getSetting( id )

    def setSetting( self, id, value ):
        """
            setSetting(id, value) -- Sets a setting for this addon.

            id        : string - id of the setting you want to set.
            value     : string or unicode - value of the setting.

            example:
              - self.Addon.setSetting(id="username", value="nuka1195")
        """
        self._settings_.setSetting( id, value )

    def openSettings( self ):
        """
            openSettings() -- Opens this addons settings dialog.

            example:
              - self.Addon.openSettings()
        """
        self._settings_.openSettings()
