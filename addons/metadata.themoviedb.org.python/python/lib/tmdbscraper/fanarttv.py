from __future__ import absolute_import
from . import api_utils
try:
    from urllib import quote
except ImportError: # py2 / py3
    from urllib import quote

API_KEY = u'384afe262ee0962545a752ff340e3ce4'
API_URL = u'https://webservice.fanart.tv/v3/movies/{}'

ARTMAP = {
    u'movielogo': u'clearlogo',
    u'hdmovielogo': u'clearlogo',
    u'hdmovieclearart': u'clearart',
    u'movieart': u'clearart',
    u'moviedisc': u'discart',
    u'moviebanner': u'banner',
    u'moviethumb': u'landscape',
    u'moviebackground': u'fanart',
    u'movieposter': u'poster'
}

def get_details(uniqueids, clientkey, language, set_tmdbid):
    media_id = _get_mediaid(uniqueids)
    if not media_id:
        return {}

    movie_data = _get_data(media_id, clientkey)
    movieset_data = _get_data(set_tmdbid, clientkey)
    if not movie_data and not movieset_data:
        return {}

    movie_art = {}
    movieset_art = {}
    if movie_data:
        movie_art = _parse_data(movie_data, language)
    if movieset_data:
        movieset_art = _parse_data(movieset_data, language)
        movieset_art = dict((u'set.' + key, value) for key, value in movieset_art.items())

    available_art = movie_art
    available_art.update(movieset_art)

    return {u'available_art': available_art}

def _get_mediaid(uniqueids):
    for source in (u'tmdb', u'imdb', u'unknown'):
        if source in uniqueids:
            return uniqueids[source]

def _get_data(media_id, clientkey):
    headers = {u'api-key': API_KEY}
    if clientkey:
        headers[u'client-key'] = clientkey
    api_utils.set_headers(headers)
    fanarttv_url = API_URL.format(media_id)
    return api_utils.load_info(fanarttv_url, default={})

def _parse_data(data, language):
    result = {}
    for arttype, artlist in data.items():
        if arttype not in ARTMAP:
            continue
        for image in artlist:
            image_lang = _get_imagelanguage(arttype, image)
            if image_lang and image_lang != language:
                continue

            generaltype = ARTMAP[arttype]
            if generaltype == u'poster' and not image_lang:
                generaltype = u'keyart'
            if artlist and generaltype not in result:
                result[generaltype] = []

            url = quote(image[u'url'], safe=u"%/:=&?~#+!$,;'@()*[]")
            resultimage = {u'url': url, u'preview': url.replace(u'.fanart.tv/fanart/', u'.fanart.tv/preview/')}
            result[generaltype].append(resultimage)

    return result

def _get_imagelanguage(arttype, image):
    if u'lang' not in image or arttype == u'moviebackground':
        return None
    if arttype in (u'movielogo', u'hdmovielogo', u'hdmovieclearart', u'movieart', u'moviebanner',
            u'moviethumb', u'moviedisc'):
        return image[u'lang'] if image[u'lang'] not in (u'', u'00') else u'en'
    # movieposter may or may not have a title and thus need a language
    return image[u'lang'] if image[u'lang'] not in (u'', u'00') else None
