## Utility to search weather.com for town codes

import os
import xbmcgui
import xbmc
import urllib2
import re


class TownSearch:
    """
        Searches for a town from weather.com and
        sets the weather.com town code in settings.
    """
    # weather.com partner id
    PARTNER_ID = "1004124588"
    # weather.com partner key
    PARTNER_KEY = "079f24145f208494"
    # base urls
    BASE_URLS = {
        "geo": "http://www.dnsstuff.com/tools/ipall/",
        "weather": "http://xoap.weather.com/weather/local/%s?cc=*&dayf=5&link=xoap&prod=xoap&unit=s&par=%s&key=%s",
        "search": "http://xoap.weather.com/search/search?where=%s"
    }
    # set headers
    HEADERS = {
        "User-Agent": "XBMC/%s" % ( xbmc.getInfoLabel( "System.BuildVersion" ), ),
        "Accept": "application/xml; charset=UTF-8"
    }
    # set regex's
    regex_geo = re.compile( "Your IP Address: <strong>(.+?)</strong><br />\s.+?Located near: <strong>([^\(<]+).*?</strong>", re.IGNORECASE )
    regex_town = re.compile( "<dnam>([^<]+)</dnam>" )
    regex_location_list = re.compile( "<loc.+?id=\"([^\"]+)\".+?type=\"([^\"]+)\">([^<]+)</loc>" )

    def __init__( self, addon, index ):
        # set our Addon class
        self.Addon = addon
        # setting index id
        self.index = index

    def get_geo_location( self ):
        try:
            # fetch location source
            html = self._fetch_source()
            # get location name
            result = self.regex_geo.search( html ).groups( 1 )
            # raise an error if city not in database
            if ( result[ 1 ].startswith( "-" ) ):
                raise
            # if ip has not changed return old values
            if ( result[ 0 ] == self.Addon.getSetting( "ip_geo" ) ):
                return self.Addon.getSetting( "id_geo" ), result[ 0 ]
            # set new ip
            self.Addon.setSetting( "ip_geo", result[ 0 ] )
            # get new town
            return self.get_town( text=result[ 1 ].strip() )[ 1 ], result[ 0 ]
        except:
            # use our fallback for any errors
            return self.Addon.getSetting( "id_geo_fallback" ), "None"

    def get_town( self, text=None ):
        try:
            # get search text
            if ( text is None ):
                text = self._get_search_text( heading=self.Addon.getLocalizedString( 30731 ) )
            # skip if user cancels keyboard
            if ( text is not None ):
                # fetch source
                xml = self._fetch_source( text )
                # select town
                town = self._select_town( xml, text.upper() )
                # raise an error if no results were found
                if ( town is None ): raise
                # only set if user selected a town
                if ( town ):
                    self.Addon.setSetting( "town%s" % ( self.index, ), town[ 0 ] )
                    self.Addon.setSetting( "id%s" % ( self.index, ), town[ 1 ] )
        except Exception, e:
            # inform user for non ip based geo location searches only, ip based geo searches have a fallback
            if ( self.index != "_geo" ):
                ok = xbmcgui.Dialog().ok( self.Addon.getAddonInfo( "Name" ), self.Addon.getLocalizedString( 30732 ) )
        # we need to return town if this is an ip based geo location search
        if ( self.index == "_geo" ):
            return town
        # open settings for non ip based geo location search
        # FIXME: uncomment when settings navigation bug is fixed
        ##self.Addon.openSettings()

    def _get_search_text( self, default="", heading="", hidden=False ):
        """ shows a keyboard and returns a value """
        # show keyboard for input
        keyboard = xbmc.Keyboard( default, heading, hidden )
        keyboard.doModal()
        # return user input unless canceled
        if ( keyboard.isConfirmed() and keyboard.getText() != "" ):
            return keyboard.getText()
        # user cancelled or entered blank
        return None

    def _fetch_source( self, text=None ):
        # return text if ip based geo location
        if ( text == "*" ): return text
        # ip based geo location search
        if ( text is None ):
            url = self.BASE_URLS[ "geo" ]
        # check for a valid weather.com code and set appropriate url
        elif ( len( text ) == 8 and text[ : 4 ].isalpha() and text[ 4 : ].isdigit() ):
            # valid id's are uppercase
            url = self.BASE_URLS[ "weather" ] % ( text.upper(), self.PARTNER_ID, self.PARTNER_KEY, )
        else:
            # search
            url = self.BASE_URLS[ "search" ] % ( text.replace( " ", "+" ), )
        # request base url
        request = urllib2.Request( url, headers=self.HEADERS )
        # return source
        return urllib2.urlopen( request ).read()

    def _select_town( self, xml, text ):
        # return if ip based geo location
        if ( text == "*" ): return xml, text
        # parse for a town name if valid weather.com id was entered
        if ( len( text ) == 8 and text[ : 4 ].isalpha() and text[ 4 : ].isdigit() ):
            # parse town name and add id
            town = [ self.regex_town.search( xml ).group( 1 ), text ]
            # valid weather source, may as well cache it
            base_path = os.path.join( self.Addon.getAddonInfo( "Profile" ), "source", "weather-%s.xml" % ( text, ) )
            # create path to cache
            if ( not os.path.isdir( os.path.dirname( base_path ) ) ):
                os.makedirs( os.path.dirname( base_path ) )
            # save source
            open( base_path, "w" ).write( xml )
        else:
            # parse town list
            towns = dict( [ [ unicode( town, "UTF-8" ), [ unicode( town, "UTF-8" ), id, type ] ] for id, type, town in self.regex_location_list.findall( xml ) ] )
            # inform user if no list found
            if ( not towns ):
                return None
            # set titles for select dialog
            titles = towns.keys()
            # if only one result return it
            if ( len( titles ) == 1 ):
                choice = 0
            else:
                # initialize to blank in case no selection
                town = ""
                # sort titles (dict() scrambles them)
                titles.sort()
                # get user selection
                choice = xbmcgui.Dialog().select( self.Addon.getLocalizedString( 30730 ), titles )
            # return selection
            if ( choice >= 0 ):
                town = towns[ titles[ choice ] ]
        # return result
        return town
