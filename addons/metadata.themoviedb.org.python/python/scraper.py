from __future__ import absolute_import
import json
import sys
import xbmc
import xbmcaddon
import xbmcgui
import xbmcplugin

from lib.tmdbscraper.tmdb import TMDBMovieScraper
from lib.tmdbscraper.fanarttv import get_details as get_fanarttv_artwork
from lib.tmdbscraper.imdbratings import get_details as get_imdb_details
from lib.tmdbscraper.traktratings import get_trakt_ratinginfo
from scraper_datahelper import combine_scraped_details_info_and_ratings, \
    combine_scraped_details_available_artwork, find_uniqueids_in_text, get_params
from scraper_config import configure_scraped_details, PathSpecificSettings, \
    configure_tmdb_artwork, is_fanarttv_configured

ADDON_SETTINGS = xbmcaddon.Addon()
ID = ADDON_SETTINGS.getAddonInfo(u'id')

def log(msg, level=xbmc.LOGDEBUG):
    xbmc.log(msg=u'[{addon}]: {msg}'.format(addon=ID, msg=msg), level=level)

def get_tmdb_scraper(settings):
    language = settings.getSetting(u'language')
    certcountry = settings.getSetting(u'tmdbcertcountry')
    return TMDBMovieScraper(ADDON_SETTINGS, language, certcountry)

def search_for_movie(title, year, handle, settings):
    log(u"Find movie with title '{title}' from year '{year}'".format(title=title, year=year), xbmc.LOGINFO)
    title = _strip_trailing_article(title)
    search_results = get_tmdb_scraper(settings).search(title, year)
    if not search_results:
        return
    if u'error' in search_results:
        header = u"The Movie Database Python error searching with web service TMDB"
        xbmcgui.Dialog().notification(header, search_results[u'error'], xbmcgui.NOTIFICATION_WARNING)
        log(header + u': ' + search_results[u'error'], xbmc.LOGWARNING)
        return

    for movie in search_results:
        listitem = _searchresult_to_listitem(movie)
        uniqueids = {u'tmdb': unicode(movie[u'id'])}
        xbmcplugin.addDirectoryItem(handle=handle, url=build_lookup_string(uniqueids),
            listitem=listitem, isFolder=True)

_articles = [prefix + article for prefix in (u', ', u' ') for article in (u"the", u"a", u"an")]
def _strip_trailing_article(title):
    title = title.lower()
    for article in _articles:
        if title.endswith(article):
            return title[:-len(article)]
    return title

def _searchresult_to_listitem(movie):
    movie_info = {u'title': movie[u'title']}
    movie_label = movie[u'title']

    movie_year = movie[u'release_date'].split(u'-')[0] if movie.get(u'release_date') else None
    if movie_year:
        movie_label += u' ({})'.format(movie_year)
        movie_info[u'year'] = movie_year

    listitem = xbmcgui.ListItem(movie_label, offscreen=True)

    listitem.setInfo(u'video', movie_info)
    if movie[u'poster_path']:
        listitem.setArt({u'thumb': movie[u'poster_path']})

    return listitem

# Low limit because a big list of artwork can cause trouble in some cases
# (a column can be too large for the MySQL integration),
# and how useful is a big list anyway? Not exactly rhetorical, this is an experiment.
IMAGE_LIMIT = 10

def add_artworks(listitem, artworks):
    for arttype, artlist in artworks.items():
        if arttype == u'fanart':
            continue
        for image in artlist[:IMAGE_LIMIT]:
            listitem.addAvailableArtwork(image[u'url'], arttype)

    fanart_to_set = [{u'image': image[u'url'], u'preview': image[u'preview']}
        for image in artworks[u'fanart'][:IMAGE_LIMIT]]
    listitem.setAvailableFanart(fanart_to_set)

def get_details(input_uniqueids, handle, settings):
    if not input_uniqueids:
        return False
    details = get_tmdb_scraper(settings).get_details(input_uniqueids)
    if not details:
        return False
    if u'error' in details:
        header = u"The Movie Database Python error with web service TMDB"
        xbmcgui.Dialog().notification(header, details[u'error'], xbmcgui.NOTIFICATION_WARNING)
        log(header + u': ' + details[u'error'], xbmc.LOGWARNING)
        return False

    details = configure_tmdb_artwork(details, settings)

    if settings.getSetting(u'RatingS') == u'IMDb' or bool(settings.getSetting(u'imdbanyway')):
        imdbinfo = get_imdb_details(details[u'uniqueids'])
        if u'error' in imdbinfo:
            header = u"The Movie Database Python error with website IMDB"
            log(header + u': ' + imdbinfo[u'error'], xbmc.LOGWARNING)
        else:
            details = combine_scraped_details_info_and_ratings(details, imdbinfo)

    if settings.getSetting(u'RatingS') == u'Trakt' or bool(settings.getSetting(u'traktanyway')):
        traktinfo = get_trakt_ratinginfo(details[u'uniqueids'])
        details = combine_scraped_details_info_and_ratings(details, traktinfo)

    if is_fanarttv_configured(settings):
        fanarttv_info = get_fanarttv_artwork(details[u'uniqueids'],
            settings.getSetting(u'fanarttv_clientkey'),
            settings.getSetting(u'fanarttv_language'),
            details[u'_info'][u'set_tmdbid'])
        details = combine_scraped_details_available_artwork(details, fanarttv_info)

    details = configure_scraped_details(details, settings)

    listitem = xbmcgui.ListItem(details[u'info'][u'title'], offscreen=True)
    listitem.setInfo(u'video', details[u'info'])
    listitem.setCast(details[u'cast'])
    listitem.setUniqueIDs(details[u'uniqueids'], u'tmdb')
    add_artworks(listitem, details[u'available_art'])

    for rating_type, value in details[u'ratings'].items():
        if u'votes' in value:
            listitem.setRating(rating_type, value[u'rating'], value[u'votes'], value[u'default'])
        else:
            listitem.setRating(rating_type, value[u'rating'], defaultt=value[u'default'])

    xbmcplugin.setResolvedUrl(handle=handle, succeeded=True, listitem=listitem)
    return True

def find_uniqueids_in_nfo(nfo, handle):
    uniqueids = find_uniqueids_in_text(nfo)
    if uniqueids:
        listitem = xbmcgui.ListItem(offscreen=True)
        xbmcplugin.addDirectoryItem(
            handle=handle, url=build_lookup_string(uniqueids), listitem=listitem, isFolder=True)

def build_lookup_string(uniqueids):
    return json.dumps(uniqueids)

def parse_lookup_string(uniqueids):
    try:
        return json.loads(uniqueids)
    except ValueError:
        log(u"Can't parse this lookup string, is it from another add-on?\n" + uniqueids, xbmc.LOGWARNING)
        return None

def run():
    params = get_params(sys.argv[1:])
    enddir = True
    if u'action' in params:
        settings = ADDON_SETTINGS if not params.get(u'pathSettings') else \
            PathSpecificSettings(json.loads(params[u'pathSettings']), lambda msg: log(msg, xbmc.LOGWARNING))
        action = params[u"action"]
        if action == u'find' and u'title' in params:
            search_for_movie(params[u"title"], params.get(u"year"), params[u'handle'], settings)
        elif action == u'getdetails' and u'url' in params:
            enddir = not get_details(parse_lookup_string(params[u"url"]), params[u'handle'], settings)
        elif action == u'NfoUrl' and u'nfo' in params:
            find_uniqueids_in_nfo(params[u"nfo"], params[u'handle'])
        else:
            log(u"unhandled action: " + action, xbmc.LOGWARNING)
    else:
        log(u"No action in 'params' to act on", xbmc.LOGWARNING)
    if enddir:
        xbmcplugin.endOfDirectory(params[u'handle'])

if __name__ == u'__main__':
    run()
