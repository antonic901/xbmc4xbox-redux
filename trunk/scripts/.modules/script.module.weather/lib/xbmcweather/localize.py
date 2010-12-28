## Utilities for localizing text and values
# -*- coding: UTF-8 -*-

import xbmc
import urllib
import re
import time


class Localize:
    """
        Class for localizing text and values.
    """

    def __init__( self ):
        # set our localized strings
        self.set_localized_strings()

    def set_localized_strings( self ):
        # localized strings
        LOCALIZED_STRINGS = range( 370, 395 + 1 )
        LOCALIZED_STRINGS += range( 1396, 1449 + 1 )
        LOCALIZED_STRINGS += range( 33001, 33099 + 1 )
        LOCALIZED_STRINGS_SPECIAL = range( 11, 62 + 1 )
        # get English strings, that's what weather.com returns
        xml = open( "special://xbmc/language/English/strings.xml", "r" ).read()
        # parse strings
        strings = re.findall( "<string.+?id=\"([^\"]+)\">([^<]+)</string>", xml )
        # create dictionary
        self.localize_text = dict( [ [ value, xbmc.getLocalizedString( int( key ) ) ] for key, value in strings if ( int( key ) in LOCALIZED_STRINGS ) ] )
        # handle speicalized strings, these are [<abbreviations>, <fulltext>] lists
        self.localize_text_special = dict( [ [ value, [ xbmc.getLocalizedString( int( key ) + ( 30 * ( int( key ) < 33 ) ) ), xbmc.getLocalizedString( int( key ) - ( 30 * ( int( key ) > 40 ) ) ) ] ] for key, value in strings if ( int( key ) in LOCALIZED_STRINGS_SPECIAL ) ] )
        # we need to handle these special as they do not correlate properly
        self.localize_text_special.update( {
            "Today": ( xbmc.getLocalizedString( 33006 ), xbmc.getLocalizedString( 33006 ), ),
            "Tonight": ( xbmc.getLocalizedString( 33018 ), xbmc.getLocalizedString( 33018 ), ),
            "N": ( xbmc.getLocalizedString( 71 ), xbmc.getLocalizedString( 88 ), ),
            "NNE": ( xbmc.getLocalizedString( 72 ), "%s %s" % ( xbmc.getLocalizedString( 88 ), xbmc.getLocalizedString( 89 ), ) ),
            "NE": ( xbmc.getLocalizedString( 73 ), xbmc.getLocalizedString( 89 ) ),
            "ENE": ( xbmc.getLocalizedString( 74 ), "%s %s" % ( xbmc.getLocalizedString( 90 ), xbmc.getLocalizedString( 89 ), ) ),
            "E": ( xbmc.getLocalizedString( 75 ), xbmc.getLocalizedString( 90 ) ),
            "ESE": ( xbmc.getLocalizedString( 76 ), "%s %s" % ( xbmc.getLocalizedString( 90 ), xbmc.getLocalizedString( 91 ), ) ),
            "SE": ( xbmc.getLocalizedString( 77 ), xbmc.getLocalizedString( 91 ) ),
            "SSE": ( xbmc.getLocalizedString( 78 ), "%s %s" % ( xbmc.getLocalizedString( 92 ), xbmc.getLocalizedString( 91 ), ) ),
            "S": ( xbmc.getLocalizedString( 79 ), xbmc.getLocalizedString( 92 ), ),
            "SSW": ( xbmc.getLocalizedString( 80 ), "%s %s" % ( xbmc.getLocalizedString( 92 ), xbmc.getLocalizedString( 93 ), ) ),
            "SW": ( xbmc.getLocalizedString( 81 ), xbmc.getLocalizedString( 93 ) ),
            "WSW": ( xbmc.getLocalizedString( 82 ), "%s %s" % ( xbmc.getLocalizedString( 94 ), xbmc.getLocalizedString( 93 ), ) ),
            "W": ( xbmc.getLocalizedString( 83 ), xbmc.getLocalizedString( 94 ), ),
            "WNW": ( xbmc.getLocalizedString( 84 ), "%s %s" % ( xbmc.getLocalizedString( 94 ), xbmc.getLocalizedString( 95 ), ) ),
            "NW": ( xbmc.getLocalizedString( 85 ), xbmc.getLocalizedString( 95 ), ),
            "NNW": ( xbmc.getLocalizedString( 86 ), "%s %s" % ( xbmc.getLocalizedString( 88 ), xbmc.getLocalizedString( 95 ), ) ),
            "VAR": ( xbmc.getLocalizedString( 96 ), xbmc.getLocalizedString( 96 ) ),
            "North": ( xbmc.getLocalizedString( 71 ), xbmc.getLocalizedString( 88 ), ),
            "North Northeast": ( xbmc.getLocalizedString( 72 ), "%s %s" % ( xbmc.getLocalizedString( 88 ), xbmc.getLocalizedString( 89 ), ) ),
            "Northeast": ( xbmc.getLocalizedString( 73 ), xbmc.getLocalizedString( 89 ) ),
            "East Northeast": ( xbmc.getLocalizedString( 74 ), "%s %s" % ( xbmc.getLocalizedString( 90 ), xbmc.getLocalizedString( 89 ), ) ),
            "East": ( xbmc.getLocalizedString( 75 ), xbmc.getLocalizedString( 90 ) ),
            "East Southeast": ( xbmc.getLocalizedString( 76 ), "%s %s" % ( xbmc.getLocalizedString( 90 ), xbmc.getLocalizedString( 91 ), ) ),
            "Southeast": ( xbmc.getLocalizedString( 77 ), xbmc.getLocalizedString( 91 ) ),
            "South Southeast": ( xbmc.getLocalizedString( 78 ), "%s %s" % ( xbmc.getLocalizedString( 92 ), xbmc.getLocalizedString( 91 ), ) ),
            "South": ( xbmc.getLocalizedString( 79 ), xbmc.getLocalizedString( 92 ), ),
            "South Southwest": ( xbmc.getLocalizedString( 80 ), "%s %s" % ( xbmc.getLocalizedString( 92 ), xbmc.getLocalizedString( 93 ), ) ),
            "Southwest": ( xbmc.getLocalizedString( 81 ), xbmc.getLocalizedString( 93 ) ),
            "West Southwest": ( xbmc.getLocalizedString( 82 ), "%s %s" % ( xbmc.getLocalizedString( 94 ), xbmc.getLocalizedString( 93 ), ) ),
            "West": ( xbmc.getLocalizedString( 83 ), xbmc.getLocalizedString( 94 ), ),
            "West Northwest": ( xbmc.getLocalizedString( 84 ), "%s %s" % ( xbmc.getLocalizedString( 94 ), xbmc.getLocalizedString( 95 ), ) ),
            "Northwest": ( xbmc.getLocalizedString( 85 ), xbmc.getLocalizedString( 95 ), ),
            "North Northwest": ( xbmc.getLocalizedString( 86 ), "%s %s" % ( xbmc.getLocalizedString( 88 ), xbmc.getLocalizedString( 95 ), ) ),
            "Variable": ( xbmc.getLocalizedString( 96 ), xbmc.getLocalizedString( 96 ) ),
        } )

    def localize_unit( self, value, unit="temp", **kwargs ):
        # replace any invalid characters
        if ( kwargs.get( "NA", False ) ):
            value = value.replace( "N/A", "" )
        # do not convert invalid values
        if ( not value or value.startswith( "N/A" ) ):
            return value
        # date/time conversion
        if ( unit in [ "time", "datetime" ] ):
            # format time properly
            if ( ":" not in value ):
                value = "%s %s" % ( value.split()[ 0 ] + ":00", value.split()[ 1 ], )
            # set our default format
            if ( unit == "datetime" ):
                format = "%s %s %s" % ( xbmc.getRegion( id="dateshort" ), re.sub( ".%S", "", xbmc.getRegion( id="time" ) ), " ".join( value.split()[ 3 : ] ), )
                dvalue, tvalue, meridiem = time.strftime( format, time.strptime( " ".join( value.split()[ : 3 ] ), "%m/%d/%y %I:%M %p" ) ).split( " ", 2 )
            else:
                format = [ "%H:%M", re.sub( ".%S", "", xbmc.getRegion( id="time" ) ) ][ unit == "time" ]
                if ( kwargs.get( "hours", False ) ):
                    format = re.sub( ".%M", "", format )
                tvalue = time.strftime( format, time.strptime( value, "%I:%M %p" ) )
                dvalue = meridiem = ""
            # remove leading 0 as python does not
            if ( tvalue.startswith( "0" ) ):
                tvalue = tvalue[ 1 : ]
            ##dvalue = dvalue.lstrip( "0" )
            # format final value
            value = "%s %s %s" % ( dvalue, tvalue, meridiem, )
            # localize AM/PM
            am, pm = xbmc.getRegion( id="meridiem" ).split( "/" )
            # return localized date/time
            return value.strip().replace( "AM", am ).replace( "PM", pm )
        # month day conversion
        elif ( unit == "monthdate" ):
            # split month and day
            month, day = value.split()
            # localize month
            date = [ self.localize_text_special.get( month, [ month ] )[ -kwargs.get( "long", False ) ], day, ]
            # do we need to reverse
            if ( xbmc.getRegion( id="datelong" ).find( "%d" ) < xbmc.getRegion( id="datelong" ).find( "%B" ) ):
                date.reverse()
            # return result
            return "%s %s" % ( date[ 0 ], date[ 1 ], )
        # wind conversion
        elif ( unit == "wind" ):
            # if speed isn't a valid integer, we localize the text (eg. calm)
            if ( value.isdigit() ):
                # split speed and unit for formatting string
                speed, unit = self.localize_unit( value, "speed" ).split( " " )
                # format wind
                if ( kwargs.get( "direction", None ) is None ):
                    wind = xbmc.getLocalizedString( 436 ) % ( int( speed ), unit, )
                #FIXME: hack for day forecasts wind, weather.com put CALM in the direction with a speed of 0 (maybe a one time mistake?)
                elif ( kwargs.get( "direction", "" ).lower() == "calm" ):
                    wind = self.localize_text.get( "Calm" )
                else:
                    # get correct string
                    string = xbmc.getLocalizedString( 434 + kwargs.get( "long", False ) )
                    # if variable wind, we don't want "from"
                    if ( kwargs[ "direction" ].lower() == "var" ):
                        string = "%s" + string.split( "%s", 1 )[ 1 ]
                    if ( kwargs.get( "split", False ) ):
                        string = string.replace( "%s ", "%s[CR]" )
                    # format wind
                    wind = string % ( self.localize_text_special.get( kwargs[ "direction" ], [ kwargs[ "direction" ] ] )[ -kwargs.get( "long", False ) ], int( speed ), unit, )
            else:
                wind = [ self.localize_text.get( value.title(), value ), "" ][ kwargs.get( "direction", None ) is None ]
            # return result
            return wind
        else:
            try:
                # we need a float
                value = float( value )
            except:
                # must be text, localize and return (eg calm)
                return self.localize_text.get( value, value )
            # temp conversion
            if ( unit == "temp" or  unit == "tempdiff" ):
                # set our default temp unit
                tempunit = xbmc.getRegion( id="tempunit" )
                # calculate the localized temp if C is required
                if ( tempunit == "°C" ):
                    # C/F difference or temperature conversion
                    if ( unit == "tempdiff" ):
                        # 9 degrees of F equal 5 degrees of C
                        value = round( float( 5 * value ) / 9 )
                    else:
                        # convert to celcius
                        value = round( ( value - 32 ) * ( float( 5 ) / 9 ) )
                # return localized temp
                return "%s%d" % ( [ "", "+" ][ value >= 0 and unit == "tempdiff" ], value, )
            # speed conversion
            elif ( unit == "speed" ):
                # set our default speed unit
                speedunit = xbmc.getRegion( id="speedunit" )
                # calculate the localized speed
                if ( speedunit == "km/h" ):
                    value = round( value * 1.609344 )
                elif ( speedunit == "m/s" ):
                    value = round( value * 0.45 )
                elif ( speedunit == "ft/min" ):
                    value = round( value * 88 )
                elif ( speedunit == "ft/s" ):
                    value = round( value * 1.47 )
                elif ( speedunit == "yard/s" ):
                    value = round( value * 0.4883 )
                elif ( speedunit == "Beaufort" ):
                    # convert mph to knots
                    knots = float( value ) * 0.86898
                    value = ( knots > 1.0 ) + ( knots >= 3.5 ) + ( knots >= 6.5 ) + ( knots >= 10.5 ) + ( knots >= 16.5 ) + ( knots >= 21.5 ) + ( knots >= 27.5 ) + ( knots >= 33.5 ) + ( knots >= 40.5 ) + ( knots >= 47.5 ) + ( knots >= 55.5 ) + ( knots >= 63.5 )
                # return localized speed
                return "%d %s" % ( value, speedunit, )
            # depth conversion
            elif ( unit == "depth" ):
                # set our default depth unit
                depthunit = [ "cm.", "in." ][ xbmc.getRegion( id="tempunit" ) == "°F" ]
                # calculate the localized depth
                if ( depthunit == "cm." ):
                    value = float( value * 2.54 )
                # return localized depth
                return "%1.2f %s" % ( value, depthunit, )
            # pressure conversion
            elif ( unit == "pressure" ):
                # set our default pressure unit and format
                pressureunit = [ [ "mb.", "%.1f %s %s" ], [ "in.", "%.2f %s %s" ] ][ xbmc.getRegion( id="tempunit" ) == "°F" ]
                # calculate the localized pressure
                if ( pressureunit[ 0 ] == "mb." ):
                    value = float( value * 33.8637526 )
                # return localized pressure
                return pressureunit[ 1 ] % ( value, pressureunit[ 0 ], { "rising": "↑", "falling": "↓", "steady": "→" }.get( kwargs[ "status" ], kwargs[ "status" ] ), )
            # distance conversion
            elif ( unit == "distance" ):
                # set our default distance unit
                distanceunit = [ 33202, 33200 ][ xbmc.getRegion( id="speedunit" ) == "mph" ]
                # calculate the localized distance
                if ( distanceunit == 33202 ):
                    value = float( value * 1.609344 )
                # pluralize for values != 1
                if ( value != 1.0 ):
                    distanceunit += 1
                # return localized distance
                return "%.1f %s" % ( value, xbmc.getLocalizedString( distanceunit ), )
            # map coordinate
            elif ( unit == "latitude" or unit == "longitude" ):
                # map format
                if ( kwargs.get( "format", "0" ) == "0" ):
                    # separate degrees, minutes and seconds
                    degrees, minutes = divmod( abs( value ), 1 )
                    minutes, seconds = divmod( minutes * 60, 1 )
                    # return localized coordinate
                    return u"%s %d°%02d'%02d\"" % ( self.localize_text_special[ [ "N", "S", "E", "W" ][ ( value < 0 ) + ( ( unit == "longitude" ) * 2 ) ] ][ 0 ], degrees, minutes, seconds * 60, )
                # handle GPS format
                elif ( kwargs.get( "format", "0" ) == "1" ):
                    # separate degrees and minutes
                    degrees, minutes = divmod( abs( value ), 1 )
                    # return localized coordinate
                    return u"%s %d°%.03f'" % ( self.localize_text_special[ [ "N", "S", "E", "W" ][ ( value < 0 ) + ( ( unit == "longitude" ) * 2 ) ] ][ 0 ], degrees, minutes * 60, )
                # handle computer format
                else:
                    # return localized coordinate
                    return u"%s %.03f°" % ( self.localize_text_special[ [ "N", "S", "E", "W" ][ ( value < 0 ) + ( ( unit == "longitude" ) * 2 ) ] ][ 0 ], abs( value ), )

    def translate_text( self, phrases, language="en" ):
        """ Takes a list of phrases and returns a translated list. """
        # no need to translate english
        if ( language == "en" or not phrases ):
            return phrases
        # conversion for JSON
        null = None
        true = True
        false = False
        # google's translate url api
        base_url = "http://ajax.googleapis.com/ajax/services/language/translate?"
        # set language params
        params = ( {
            "langpair": "en|%s" % ( language, ),
            "v": "1.0",
            "q": " .-. ".join( phrases )
        } )
        # translate text
        response = eval( urllib.urlopen( base_url, data=urllib.urlencode( params ) ).read() )
        # successful, return translated text
        if ( response[ "responseStatus" ] == 200 ):
            return response[ "responseData" ][ "translatedText" ].split( ".-." )
        # unsuccessful, return original text FIXME: maybe return the error message
        else:
            return phrases
