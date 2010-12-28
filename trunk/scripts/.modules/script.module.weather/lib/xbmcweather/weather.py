## Main module for fetching and parsing weather info and setting window properties
# -*- coding: UTF-8 -*-

import os
import xbmcgui
import xbmc
import urllib2
import re
import time


class Weather:
    """
        Fetches weather info from weather.com and
        sets window properties for skinners to use.
    """
    # weather.com partner id
    PARTNER_ID = "1004124588"
    # weather.com partner key
    PARTNER_KEY = "079f24145f208494"
    # base url
    BASE_URLS = {
        "weather": "http://xoap.weather.com/weather/local/%s?cc=*&unit=e&dayf=5&prod=xoap&link=xoap&par=%s&key=%s"
    }
    # set headers
    HEADERS = {
        "User-Agent": "XBMC/%s" % ( xbmc.getInfoLabel( "System.BuildVersion" ), ),
        "Accept": "application/xml; charset=UTF-8"
    }
    # weather window for setting weather properties
    WINDOW = xbmcgui.Window( 12600 )
    # set refresh time in minutes
    REFRESH_TIME = 20

    def __init__( self, addon, index, refresh, localize ):
        # set our Addon class
        self.Addon = addon
        # get weather.com code, used to fetch proper weather info
        self.location_id = self.Addon.getSetting( "id%s" % ( index, ) )
        # if ip based geo location, fetch location
        self.geolocation = self.location_id == "*"
        if ( self.geolocation ):
            self._get_geo_location_id()
        # set force refresh
        self.refresh = refresh
        # set localize functions
        self.localize_unit = localize.localize_unit
        self.localize_text = localize.localize_text
        self.localize_text_special = localize.localize_text_special
        # set base path for source
        self.base_path = os.path.join( self.Addon.getAddonInfo( "Profile" ), "source", "weather-%s.xml" % self.location_id )
        # set addon info
        self._set_addon_info()

    def _get_geo_location_id( self ):
        # FIXME: Remove if block and always search for mobile application (eg traveling) maybe have a setting since everytime you change
        # locations it has to fetch ip, if not mobile leave if block.
        # get geo location id (ip based geo location-only once per session) for mobile based we need to always search if ip has changed
        if ( self.WINDOW.getProperty( "Location.IP" ) ):
            self.location_id = self.Addon.getSetting( "id_geo" )
        else:
            # search for town by ip
            from xbmcweather import search
            self.location_id, ip = search.TownSearch( addon=self.Addon, index="_geo" ).get_geo_location()
            # set window property (ip based geo location)
            self.WINDOW.setProperty( "Location.IP", ip )

    def _set_addon_info( self ):
        # set addon's logo, id and name
        self.WINDOW.setProperty( "Addon.Logo", self.Addon.getAddonInfo( "Icon" ) )# $GROUP[Addon Info] Addon's Icon path
        self.WINDOW.setProperty( "Addon.Id", self.Addon.getAddonInfo( "Id" ) )# Addon's Id (useful for customized logo's)
        self.WINDOW.setProperty( "Addon.Name", self.Addon.getAddonInfo( "Name" ) )# Addon's name

    def fetch_weather( self ):
        try:
            # set success message
            msg = "true"
            # get the source files date if it exists
            try:
                date = time.mktime( time.gmtime( os.path.getmtime( self.base_path ) ) )
            except:
                date = 0
            # set default expiration date
            expires = date + ( self.REFRESH_TIME * 60 )
            # see if necessary to refresh source
            refresh = ( ( time.mktime( time.gmtime() ) ) > expires ) or self.refresh
            # do we need to fetch new source?
            if ( refresh ):
                # request base url
                request = urllib2.Request( self.BASE_URLS[ "weather" ] % ( self.location_id, self.PARTNER_ID, self.PARTNER_KEY, ), headers=self.HEADERS )
                # open requested url and fetch source
                xml = urllib2.urlopen( request ).read()
                # cache source
                self._cache_source( xml )
            else:
                # open cached file
                xml = open( self.base_path, "r" ).read()
            # parse info
            info = self._parse_source( xml )
        except:
            # set error message and clear info
            msg = "error"
            info = self._clear_info()
        # set properties
        self._set_properties( info )
        # set fetched property (success)
        self.WINDOW.setProperty( "Weather.IsFetched", msg )# $GROUP[Status] ('true'=successfully fetched weather, 'false'=currently fetching weather, 'error'=fetching weather failed)
        # return message for other weather addons
        return msg

    def _cache_source( self, xml ):
        # create path to cache
        if ( not os.path.isdir( os.path.dirname( self.base_path ) ) ):
            os.makedirs( os.path.dirname( self.base_path ) )
        # save source
        open( self.base_path, "w" ).write( xml )

    def _parse_source( self, xml ):
        # regex's
        regex_location = re.compile( "<loc.+?id=\"([^\"]+)\">.+?<dnam>([^<]+)</dnam>.+?<tm>([^<]+)</tm>.+?<lat>([^<]+)</lat>.+?<lon>([^<]+)</lon>.+?<sunr>([^<]+)</sunr>.+?<suns>([^<]+)</suns>.+?<zone>([^<]+)</zone>.+?</loc>", re.DOTALL + re.IGNORECASE )
        regex_current = re.compile( "<cc>.+?<lsup>([^<]+)</lsup>.+?<obst>([^<]+)</obst>.+?<tmp>([^<]+)</tmp>.+?<flik>([^<]+)</flik>.+?<t>([^<]+)</t>.+?<icon>([^<]+)</icon>.+?<bar>.+?<r>([^<]+)</r>.+?<d>([^<]+)</d>.+?</bar>.+?<wind>.+?<s>([^<]+)</s>.+?<gust>([^<]+)</gust>.+?<d>([^<]+)</d>.+?<t>([^<]+)</t>.+?</wind>.+?<hmid>([^<]+)</hmid>.+?<vis>([^<]+)</vis>.+?<uv>.+?<i>([^<]+)</i>.+?<t>([^<]+)</t>.+?</uv>.+?<dewp>([^<]+)</dewp>.+?<moon>.+?<icon>([^<]+)</icon>.+?<t>([^<]+)</t>.+?</moon>.+?</cc>", re.DOTALL + re.IGNORECASE )
        regex_days_updated = re.compile( "<dayf>.+?<lsup>([^<]+)</lsup>", re.DOTALL + re.IGNORECASE )
        regex_days = re.compile( "<day.+?d=\"([^\"]+)\".+?t=\"([^\"]+)\".+?dt=\"([^\"]+)\">.+?<hi>([^<]+)</hi>.+?<low>([^<]+)</low>.+?<sunr>([^<]+)</sunr>.+?<suns>([^<]+)</suns>.+?<part.+?p=\"d\">.+?<icon>([^<]+)</icon>.+?<t>([^<]+)</t>.+?<wind>.+?<s>([^<]+)</s>.+?<gust>([^<]+)</gust>.+?<d>([^<]+)</d>.+?<t>([^<]+)</t>.+?</wind>.+?<bt>([^<]+)</bt>.+?<ppcp>([^<]+)</ppcp>.+?<hmid>([^<]+)</hmid>.+?</part>.+?<part.+?p=\"n\">.+?<icon>([^<]+)</icon>.+?<t>([^<]+)</t>.+?<wind>.+?<s>([^<]+)</s>.+?<gust>([^<]+)</gust>.+?<d>([^<]+)</d>.+?<t>([^<]+)</t>.+?</wind>.+?<bt>([^<]+)</bt>.+?<ppcp>([^<]+)</ppcp>.+?<hmid>([^<]+)</hmid>.+?</part>.+?</day>", re.DOTALL + re.IGNORECASE )
        # parse info
        location = regex_location.search( xml ).groups()
        cc = regex_current.search( xml ).groups()
        days_updated = regex_days_updated.search( xml ).group( 1 )
        days = regex_days.findall( xml )
        # return results
        return { "location": location, "cc": cc, "days_updated": days_updated, "days": days }

    def _clear_info( self ):
        # set dummy info (all blanks)
        return {
            "location": [ "" ] * 8,
            "cc": [ "" ] * 19,
            "days_updated": "",
            "days": [ [ count ] + [ "" ] * 24 for count in range( 5 ) ]
        }

    def _set_properties( self, info ):
        # updated
        self.WINDOW.setProperty( "Updated", self.localize_unit( info[ "cc" ][ 0 ], "datetime" ) )# Last time weather.com updated current weather (eg. 10/01/2010 10:00 AM EDT)
        self.WINDOW.setProperty( "Updated.5Day", self.localize_unit( info[ "days_updated" ], "datetime" ) )# Last time weather.com updated 5-day forecast (eg. 10/01/2010 10:00 AM EDT)
        # set location if ip based geo location
        if ( self.geolocation ):
            self.WINDOW.setProperty( "Location", "*%s" % ( info[ "location" ][ 1 ], ) )# $GROUP[Location] Town (eg. New York, NY)
        self.WINDOW.setProperty( "Location.Id", self.location_id )# Town's weather.com id (eg. USNY0996)
        #self.WINDOW.setProperty( "Location.Index", sys.argv[ 1 ] )# Town's setting index (starts at 1)
        self.WINDOW.setProperty( "Location.Latitude", self.localize_unit( info[ "location" ][ 3 ], "latitude", format=self.Addon.getSetting( "coordinate_format" ) ) )# Town's latitude (*see settings - eg. N 40°42'36")
        self.WINDOW.setProperty( "Location.Longitude", self.localize_unit( info[ "location" ][ 4 ], "longitude", format=self.Addon.getSetting( "coordinate_format" ) ) )# Town's longitude (*see settings - eg. W 74°00'36")
        self.WINDOW.setProperty( "Location.Time", self.localize_unit( info[ "location" ][ 2 ], "time" ) )# Town's local time when weather was fetched (eg. 7:39 AM)
        # current conditions
        self.WINDOW.setProperty( "Current.Condition", " ".join( [ self.localize_text.get( word, word ) for word in info[ "cc" ][ 4 ].split( " " ) if ( word ) ] ) )# $GROUP[Current Conditions] Current condition description (eg. Fair)
        self.WINDOW.setProperty( "Current.ConditionIcon", os.path.join( self.Addon.getSetting( "icon_path_weather" ), info[ "cc" ][ 5 ] + ".png" ) )# Current condition icon path (*see settings)
        self.WINDOW.setProperty( "Current.DewPoint", self.localize_unit( info[ "cc" ][ 16 ] ) )# Current dew point (eg. 46)
        self.WINDOW.setProperty( "Current.FanartCode", info[ "cc" ][ 5 ] )# Current condition icon code number (eg. 34)
        self.WINDOW.setProperty( "Current.FanartPath", [ "", os.path.join( self.Addon.getSetting( "fanart_path" ), [ info[ "cc" ][ 5 ], self.location_id ][ int( self.Addon.getSetting( "fanart_type" ) ) ] ) ][ self.Addon.getSetting( "fanart_path" ) != "" ] )# Current condition or location fanart path (*see settings)
        self.WINDOW.setProperty( "Current.FeelsLike", self.localize_unit( info[ "cc" ][ 3 ] ) )# Current feels like temperature (eg. 62)
        self.WINDOW.setProperty( "Current.Humidity", "%s" % ( info[ "cc" ][ 12 ], ) )# Current humidity (eg. 49)
        self.WINDOW.setProperty( "Current.Moon", " ".join( [ self.localize_text.get( word, word ) for word in info[ "cc" ][ 18 ].split( " " ) if ( word ) ] ) )# Current moon phase description (eg. New)
        self.WINDOW.setProperty( "Current.MoonIcon", os.path.join( self.Addon.getSetting( "icon_path_moon" ), info[ "cc" ][ 17 ] + ".png" ) )# Current moon phase icon path (*see settings)
        self.WINDOW.setProperty( "Current.Pressure", self.localize_unit( info[ "cc" ][ 6 ], "pressure", status=info[ "cc" ][ 7 ] ) )# Current barometric pressure (eg. 30.25 in. ↑)
        self.WINDOW.setProperty( "Current.Sunrise", self.localize_unit( info[ "location" ][ 5 ], "time" ) )# Current sunrise (eg. 7:39 AM)
        self.WINDOW.setProperty( "Current.Sunset", self.localize_unit( info[ "location" ][ 6 ], "time" ) )# Current sunset (eg. 7:02 PM)
        self.WINDOW.setProperty( "Current.Temperature", self.localize_unit( info[ "cc" ][ 2 ] ) )# Current temperature (eg. 65)
        self.WINDOW.setProperty( "Current.UVIndex", [ "%s %s" % ( info[ "cc" ][ 14 ], self.localize_text.get( info[ "cc" ][ 15 ], info[ "cc" ][ 15 ] ), ), info[ "cc" ][ 14 ] ][ info[ "cc" ][ 14 ] == "N/A" ] )# Current uv index (eg. 0 Low)
        self.WINDOW.setProperty( "Current.Visibility", self.localize_unit( info[ "cc" ][ 13 ], "distance" ) )# Current visibility (eg. 10.0 miles)
        self.WINDOW.setProperty( "Current.Wind", self.localize_unit( info[ "cc" ][ 8 ], "wind", direction=info[ "cc" ][ 11 ] ) )# Current wind (eg. From WNW at 10 mph)
        self.WINDOW.setProperty( "Current.WindLong", self.localize_unit( info[ "cc" ][ 8 ], "wind", direction=info[ "cc" ][ 11 ], long=True ) )# Current wind (eg. From the West Northwest at 10 mph)
        self.WINDOW.setProperty( "Current.WindDegrees", "%s°" % ( info[ "cc" ][ 10 ], ) )# Current wind direction in degrees (eg. 300°)
        self.WINDOW.setProperty( "Current.WindDirection", self.localize_text_special.get( info[ "cc" ][ 11 ], [ info[ "cc" ][ 11 ] ] )[ 0 ] )# Current wind direction (abbreviated) (eg. WNW)
        self.WINDOW.setProperty( "Current.WindDirectionLong", self.localize_text_special.get( info[ "cc" ][ 11 ], [ info[ "cc" ][ 11 ] ] )[ -1 ] )# Current wind direction (eg. West Northwest)
        self.WINDOW.setProperty( "Current.WindSpeed", self.localize_unit( info[ "cc" ][ 8 ], "speed" ) )# Current wind speed (eg. 10 mph)
        self.WINDOW.setProperty( "Current.WindGust", self.localize_unit( info[ "cc" ][ 9 ], "wind", NA=True ) )# Current wind gust (eg. Gust to 20 mph)
        self.WINDOW.setProperty( "Current.WindGustSpeed", self.localize_unit( info[ "cc" ][ 9 ], "speed", NA=True ) )# Current wind gust speed (eg. 20 mph)
        # 5-day forecast - loop thru and set each day
        for day in info[ "days" ]:
            # days info
            self.WINDOW.setProperty( "Day%s.Title" % ( day[ 0 ], ), self.localize_text_special.get( day[ 1 ], [ day[ 1 ] ] )[ -1 ] )# $GROUP[5 Day Forecast] Days name (eg. Monday)
            self.WINDOW.setProperty( "Day%s.Day" % ( day[ 0 ], ), self.localize_text_special.get( day[ 1 ], [ day[ 1 ] ] )[ -1 ] )# Days name (eg. Monday)
            self.WINDOW.setProperty( "Day%s.DayShort" % ( day[ 0 ], ), self.localize_text_special.get( day[ 1 ], [ day[ 1 ] ] )[ 0 ] )# Days abbreviated name (eg. Mon)
            self.WINDOW.setProperty( "Day%s.Date" % ( day[ 0 ], ), self.localize_unit( day[ 2 ], "monthdate", long=True ) )# Days date (eg. October 1)
            self.WINDOW.setProperty( "Day%s.DateShort" % ( day[ 0 ], ), self.localize_unit( day[ 2 ], "monthdate" ) )# Days abbreviated date (eg. Oct 1)
            self.WINDOW.setProperty( "Day%s.LowTemperature" % ( day[ 0 ], ), self.localize_unit( day[ 4 ] ) )# Days low temperature (eg. 50)
            self.WINDOW.setProperty( "Day%s.HighTemperature" % ( day[ 0 ], ), self.localize_unit( day[ 3 ] ) )# Days high temperature (eg. 70)
            self.WINDOW.setProperty( "Day%s.Sunrise" % ( day[ 0 ], ), self.localize_unit( day[ 5 ], "time" ) )# Days sunrise (eg. 7:39 AM)
            self.WINDOW.setProperty( "Day%s.Sunset" % ( day[ 0 ], ), self.localize_unit( day[ 6 ], "time" ) )# Days sunset (eg. 7:02 PM)
            # daytime forecast
            self.WINDOW.setProperty( "Day%s.FanartCode" % ( day[ 0 ], ), day[ 7 ] )# $GROUP[*] Daytime outlook icon code number (eg. 34)
            self.WINDOW.setProperty( "Day%s.Humidity" % ( day[ 0 ], ), "%s" % ( day[ 15 ], ) )# Daytime humidity (eg. 49)
            self.WINDOW.setProperty( "Day%s.Outlook" % ( day[ 0 ], ), " ".join( [ self.localize_text.get( word, word ) for word in day[ 8 ].split( " " ) if ( word ) ] ) )# Daytime outlook description (eg. Mostly Sunny)
            self.WINDOW.setProperty( "Day%s.OutlookBrief" % ( day[ 0 ], ), " ".join( [ self.localize_text.get( word, word ) for word in day[ 13 ].split( " " ) if ( word ) ] ) )# Daytime outlook brief description (eg. M Sunny) (Does NOT localize properly)
            self.WINDOW.setProperty( "Day%s.OutlookIcon" % ( day[ 0 ], ), os.path.join( self.Addon.getSetting( "icon_path_weather" ), day[ 7 ] + ".png" ) )# Daytime outlook icon path (*see settings)
            self.WINDOW.setProperty( "Day%s.Precipitation" % ( day[ 0 ], ), day[ 14 ] )# Daytime chance of precipitation (eg. 60)
            self.WINDOW.setProperty( "Day%s.Wind" % ( day[ 0 ], ), self.localize_unit( day[ 9 ], "wind", direction=day[ 12 ] ) )# Daytime wind (eg. From WNW at 10 mph)
            self.WINDOW.setProperty( "Day%s.WindLong" % ( day[ 0 ], ), self.localize_unit( day[ 9 ], "wind", direction=day[ 12 ], long=True ) )# Daytime wind (eg. From the West Northwest at 10 mph)
            self.WINDOW.setProperty( "Day%s.WindDegrees" % ( day[ 0 ], ), "%s°" % ( day[ 11 ], ) )# Daytime wind direction in degrees (eg. 300°)
            self.WINDOW.setProperty( "Day%s.WindDirection" % ( day[ 0 ], ), self.localize_text_special.get( day[ 12 ], [ day[ 12 ] ] )[ 0 ] )# Daytime wind direction (abbreviated) (eg. WNW)
            self.WINDOW.setProperty( "Day%s.WindDirectionLong" % ( day[ 0 ], ), self.localize_text_special.get( day[ 12 ], [ day[ 12 ] ] )[ -1 ] )# Daytime wind direction (eg. West Northwest)
            self.WINDOW.setProperty( "Day%s.WindSpeed" % ( day[ 0 ], ), self.localize_unit( day[ 9 ], "speed" ) )# Daytime wind speed (eg. 10 mph)
            self.WINDOW.setProperty( "Day%s.WindGust" % ( day[ 0 ], ), self.localize_unit( day[ 10 ], "wind", NA=True ) )# Daytime wind gust (eg. Gust to 20 mph)
            self.WINDOW.setProperty( "Day%s.WindGustSpeed" % ( day[ 0 ], ), self.localize_unit( day[ 10 ], "speed", NA=True ) )# Daytime wind gust speed (eg. 20 mph)
            # nighttime forecast
            self.WINDOW.setProperty( "Night%s.FanartCode" % ( day[ 0 ], ), day[ 16 ] )# $GROUP[*] Nighttime outlook icon code number (eg. 33)
            self.WINDOW.setProperty( "Night%s.Humidity" % ( day[ 0 ], ), "%s" % ( day[ 24 ], ) )# Nighttime humidity (eg. 49)
            self.WINDOW.setProperty( "Night%s.Outlook" % ( day[ 0 ], ), " ".join( [ self.localize_text.get( word, word ) for word in day[ 17 ].split( " " ) if ( word ) ] ) )# Nighttime outlook description (eg. Partly Cloudy)
            self.WINDOW.setProperty( "Night%s.OutlookBrief" % ( day[ 0 ], ), " ".join( [ self.localize_text.get( word, word ) for word in day[ 22 ].split( " " ) if ( word ) ] ) )# Nighttime outlook brief description (eg. P Cloudy) (Does NOT localize properly)
            self.WINDOW.setProperty( "Night%s.OutlookIcon" % ( day[ 0 ], ), os.path.join( self.Addon.getSetting( "icon_path_weather" ), day[ 16 ] + ".png" ) )# Nighttime outlook icon path (*see settings)
            self.WINDOW.setProperty( "Night%s.Precipitation" % ( day[ 0 ], ), day[ 23 ] )# Nighttime chance of precipitation (eg. 60)
            self.WINDOW.setProperty( "Night%s.Wind" % ( day[ 0 ], ), self.localize_unit( day[ 18 ], "wind", direction=day[ 21 ] ) )# Nighttime wind (eg. From WNW at 5 mph)
            self.WINDOW.setProperty( "Night%s.WindLong" % ( day[ 0 ], ), self.localize_unit( day[ 18 ], "wind", direction=day[ 21 ], long=True ) )# Nighttime wind (eg. From the West Northwest at 5 mph)
            self.WINDOW.setProperty( "Night%s.WindDegrees" % ( day[ 0 ], ), "%s°" % ( day[ 20 ], ) )# Nighttime wind direction in degrees (eg. 300°)
            self.WINDOW.setProperty( "Night%s.WindDirection" % ( day[ 0 ], ), self.localize_text_special.get( day[ 21 ], [ day[ 21 ] ] )[ 0 ] )# Nighttime wind direction (abbreviated) (eg. WNW)
            self.WINDOW.setProperty( "Night%s.WindDirectionLong" % ( day[ 0 ], ), self.localize_text_special.get( day[ 21 ], [ day[ 21 ] ] )[ -1 ] )# Nighttime wind direction (eg. West Northwest)
            self.WINDOW.setProperty( "Night%s.WindSpeed" % ( day[ 0 ], ), self.localize_unit( day[ 18 ], "speed" ) )# Nighttime wind speed (eg. 5 mph)
            self.WINDOW.setProperty( "Night%s.WindGust" % ( day[ 0 ], ), self.localize_unit( day[ 19 ], "wind", NA=True ) )# Nighttime wind gust (eg. Gust to 10 mph)
            self.WINDOW.setProperty( "Night%s.WindGustSpeed" % ( day[ 0 ], ), self.localize_unit( day[ 19 ], "speed", NA=True ) )# Nighttime wind gust speed (eg. 10 mph)
