## Weather.com (standard)

import sys
import xbmcaddon

# Addon class
Addon = xbmcaddon.Addon( id="script.weather.standard" )


if ( __name__ == "__main__" ):
    # search for new town
    if sys.argv[ 1 ].startswith( "search" ):
        from xbmcweather import search
        search.TownSearch( addon=Addon, index=sys.argv[ 1 ][ 6 : ] ).get_town()
    # print properties
    elif ( sys.argv[ 1 ] == "properties" ):
        from xbmcweather import pprinter
        pprinter.PPrinter( addon=Addon ).print_properties()
    # fetch weather
    else:
        from xbmcweather import weather, localize
        weather.Weather( addon=Addon, index=sys.argv[ 1 ], refresh=sys.argv[ 2 ] == "1", localize=localize.Localize() ).fetch_weather()
