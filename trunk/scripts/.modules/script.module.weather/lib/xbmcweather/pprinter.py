## Utility to print window properties set by the weather addon

import os
import xbmcgui
import xbmc
import re


class PPrinter:
    """
        Prints all weather properties set by the weather
        addon, with available descriptions.
    """
    def __init__( self, addon ):
        # set our Addon class
        self.Addon = addon

    def print_properties( self ):
        # get python source, we set properties in xbmcweather/weather.py file
        pySource = open( os.path.join( os.path.dirname( __file__ ), "weather.py" ), "r" ).read()
        # get properties, group and description set by addon
        properties = re.findall( "self\.WINDOW\.setProperty\([ ]{0,1}\"([^\"]+)\".+?#(?:.*?\$GROUP\[(.+?)\])?.*?(.+)", pySource )
        # add window properties header
        text = "\n%s\n%s Window Properties\n%s" % ( "-" * 150, self.Addon.getAddonInfo( "Name" ), "-" * 150, )
        # loop thru and add properties
        for property, group, description in properties:
            # 5 day future outlook
            if ( property.startswith( "Day%s" ) or property.startswith( "Night%s" ) ):
                property = property.replace( "%s", "<0-4>" )
                description = description.replace( "%s", "<0-4>" )
            # format property
            property = "Window(Weather).Property(%s)" % ( property, )
            if ( description != "" ):
                property = property + " "
                property = property.ljust( 60, "." )
            # group or subgroup
            if ( group ):
                group = [ "\n[%s]\n" % ( group, ), "\n" ][ group == "*" ]
            # append formatted line
            text += "%s%s %s\n" % ( group, property, description, )
        # add ending divider
        text += "%s\n" % ( "-" * 150, )
        # log properties for easy copying
        xbmc.log( text, level=xbmc.LOGNOTICE )
        # inform user
        ok = xbmcgui.Dialog().ok( self.Addon.getAddonInfo( "Name" ), self.Addon.getLocalizedString( 30700 ), os.path.join( xbmc.translatePath( "special://home/" ), "xbmc.log" ) )
