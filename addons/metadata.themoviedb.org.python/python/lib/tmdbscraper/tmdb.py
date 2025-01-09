from __future__ import absolute_import
from datetime import datetime, timedelta
from . import tmdbapi


class TMDBMovieScraper(object):
    def __init__(self, url_settings, language, certification_country):
        self.url_settings = url_settings
        self.language = language
        self.certification_country = certification_country
        self._urls = None

    @property
    def urls(self):
        if not self._urls:
            self._urls = _load_base_urls(self.url_settings)
        return self._urls

    def search(self, title, year=None):
        search_media_id = _parse_media_id(title)
        if search_media_id:
            if search_media_id[u'type'] == u'tmdb':
                result = _get_movie(search_media_id[u'id'], self.language, True)
                result = [result]
            else:
                response = tmdbapi.find_movie_by_external_id(search_media_id[u'id'], language=self.language)
                theerror = response.get(u'error')
                if theerror:
                    return u'error: {}'.format(theerror)
                result = response.get(u'movie_results')
            if u'error' in result:
                return result
        else:
            response = tmdbapi.search_movie(query=title, year=year, language=self.language)
            theerror = response.get(u'error')
            if theerror:
                return u'error: {}'.format(theerror)
            result = response[u'results']
        urls = self.urls

        def is_best(item):
            return item[u'title'].lower() == title and (
                not year or item.get(u'release_date', u'').startswith(year))
        if result and not is_best(result[0]):
            best_first = (item for item in result if is_best(item)), None.next()
            if best_first:
                result = [best_first] + [item for item in result if item is not best_first]

        for item in result:
            if item.get(u'poster_path'):
                item[u'poster_path'] = urls[u'preview'] + item[u'poster_path']
            if item.get(u'backdrop_path'):
                item[u'backdrop_path'] = urls[u'preview'] + item[u'backdrop_path']
        return result

    def get_details(self, uniqueids):
        media_id = uniqueids.get(u'tmdb') or uniqueids.get(u'imdb')
        details = self._gather_details(media_id)
        if not details:
            return None
        if details.get(u'error'):
            return details
        return self._assemble_details(**details)

    def _gather_details(self, media_id):
        movie = _get_movie(media_id, self.language)
        if not movie or movie.get(u'error'):
            return movie

        # don't specify language to get English text for fallback
        movie_fallback = _get_movie(media_id)

        collection = _get_moviecollection(movie[u'belongs_to_collection'].get(u'id'), self.language) if \
            movie[u'belongs_to_collection'] else None
        collection_fallback = _get_moviecollection(movie[u'belongs_to_collection'].get(u'id')) if \
            movie[u'belongs_to_collection'] else None

        return {u'movie': movie, u'movie_fallback': movie_fallback, u'collection': collection,
            u'collection_fallback': collection_fallback}

    def _assemble_details(self, movie, movie_fallback, collection, collection_fallback):
        info = {
            u'title': movie[u'title'],
            u'originaltitle': movie[u'original_title'],
            u'plot': movie.get(u'overview') or movie_fallback.get(u'overview'),
            u'tagline': movie.get(u'tagline') or movie_fallback.get(u'tagline'),
            u'studio': _get_names(movie[u'production_companies']),
            u'genre': _get_names(movie[u'genres']),
            u'country': _get_names(movie[u'production_countries']),
            u'credits': _get_cast_members(movie[u'casts'], u'crew', u'Writing', [u'Screenplay', u'Writer', u'Author']),
            u'director': _get_cast_members(movie[u'casts'], u'crew', u'Directing', [u'Director']),
            u'premiered': movie[u'release_date'],
            u'tag': _get_names(movie[u'keywords'][u'keywords'])
        }

        if u'countries' in movie[u'releases']:
            certcountry = self.certification_country.upper()
            for country in movie[u'releases'][u'countries']:
                if country[u'iso_3166_1'] == certcountry and country[u'certification']:
                    info[u'mpaa'] = country[u'certification']
                    break

        trailer = _parse_trailer(movie.get(u'trailers', {}), movie_fallback.get(u'trailers', {}))
        if trailer:
            info[u'trailer'] = trailer
        if collection:
            info[u'set'] = collection.get(u'name') or collection_fallback.get(u'name')
            info[u'setoverview'] = collection.get(u'overview') or collection_fallback.get(u'overview')
        if movie.get(u'runtime'):
            info[u'duration'] = movie[u'runtime'] * 60

        ratings = {u'themoviedb': {u'rating': float(movie[u'vote_average']), u'votes': int(movie[u'vote_count'])}}
        uniqueids = {u'tmdb': movie[u'id'], u'imdb': movie[u'imdb_id']}
        cast = [{
                u'name': actor[u'name'],
                u'role': actor[u'character'],
                u'thumbnail': self.urls[u'original'] + actor[u'profile_path']
                    if actor[u'profile_path'] else u"",
                u'order': actor[u'order']
            }
            for actor in movie[u'casts'].get(u'cast', [])
        ]
        available_art = _parse_artwork(movie, collection, self.urls, self.language)

        _info = {u'set_tmdbid': movie[u'belongs_to_collection'].get(u'id')
            if movie[u'belongs_to_collection'] else None}

        return {u'info': info, u'ratings': ratings, u'uniqueids': uniqueids, u'cast': cast,
            u'available_art': available_art, u'_info': _info}

def _parse_media_id(title):
    if title.startswith(u'tt') and title[2:].isdigit():
        return {u'type': u'imdb', u'id':title} # IMDB ID works alone because it is clear
    title = title.lower()
    if title.startswith(u'tmdb/') and title[5:].isdigit(): # TMDB ID
        return {u'type': u'tmdb', u'id':title[5:]}
    elif title.startswith(u'imdb/tt') and title[7:].isdigit(): # IMDB ID with prefix to match
        return {u'type': u'imdb', u'id':title[5:]}
    return None

def _get_movie(mid, language=None, search=False):
    details = None if search else \
        u'trailers,images,releases,casts,keywords' if language is not None else \
        u'trailers'
    response = tmdbapi.get_movie(mid, language=language, append_to_response=details)
    theerror = response.get(u'error')
    if theerror:
        return u'error: {}'.format(theerror)
    else:
        return response

def _get_moviecollection(collection_id, language=None):
    if not collection_id:
        return None
    details = u'images'
    response = tmdbapi.get_collection(collection_id, language=language, append_to_response=details)
    theerror = response.get(u'error')
    if theerror:
        return u'error: {}'.format(theerror)
    else:
        return response

def _parse_artwork(movie, collection, urlbases, language):
    if language:
        # Image languages don't have regional variants
        language = language.split(u'-')[0]
    posters = []
    landscape = []
    fanart = []
    if u'images' in movie:
        posters = _get_images_with_fallback(movie[u'images'][u'posters'], urlbases, language)
        landscape = _get_images(movie[u'images'][u'backdrops'], urlbases, language)
        fanart = _get_images(movie[u'images'][u'backdrops'], urlbases, None)

    setposters = []
    setlandscape = []
    setfanart = []
    if collection and u'images' in collection:
        setposters = _get_images_with_fallback(collection[u'images'][u'posters'], urlbases, language)
        setlandscape = _get_images(collection[u'images'][u'backdrops'], urlbases, language)
        setfanart = _get_images(collection[u'images'][u'backdrops'], urlbases, None)

    return {u'poster': posters, u'landscape': landscape, u'fanart': fanart,
        u'set.poster': setposters, u'set.landscape': setlandscape, u'set.fanart': setfanart}

def _get_images_with_fallback(imagelist, urlbases, language, language_fallback=u'en'):
    images = _get_images(imagelist, urlbases, language)

    # Add backup images
    if language != language_fallback:
        images.extend(_get_images(imagelist, urlbases, language_fallback))

    # Add any images if nothing set so far
    if not images:
        images = _get_images(imagelist, urlbases)

    return images

def _get_images(imagelist, urlbases, language=u'_any'):
    result = []
    for img in imagelist:
        if language != u'_any' and img[u'iso_639_1'] != language:
            continue
        result.append({
            u'url': urlbases[u'original'] + img[u'file_path'],
            u'preview': urlbases[u'preview'] + img[u'file_path'],
        })
    return result

def _get_date_numeric(datetime_):
    return (datetime_ - datetime(1970, 1, 1)).total_seconds()

def _load_base_urls(url_settings):
    urls = {}
    urls[u'original'] = url_settings.getSetting(u'originalUrl')
    urls[u'preview'] = url_settings.getSetting(u'previewUrl')
    last_updated = url_settings.getSetting(u'lastUpdated')
    if not urls[u'original'] or not urls[u'preview'] or not last_updated or \
            float(last_updated) < _get_date_numeric(datetime.now() - timedelta(days=30)):
        conf = tmdbapi.get_configuration()
        if conf:
            urls[u'original'] = conf[u'images'][u'secure_base_url'] + u'original'
            urls[u'preview'] = conf[u'images'][u'secure_base_url'] + u'w780'
            url_settings.setSetting(u'originalUrl', urls[u'original'])
            url_settings.setSetting(u'previewUrl', urls[u'preview'])
            url_settings.setSetting(u'lastUpdated', unicode(_get_date_numeric(datetime.now())))
    return urls

def _parse_trailer(trailers, fallback):
    if trailers.get(u'youtube'):
        return u'plugin://plugin.video.youtube/?action=play_video&videoid='+trailers[u'youtube'][0][u'source']
    if fallback.get(u'youtube'):
        return u'plugin://plugin.video.youtube/?action=play_video&videoid='+fallback[u'youtube'][0][u'source']
    return None

def _get_names(items):
    return [item[u'name'] for item in items] if items else []

def _get_cast_members(casts, casttype, department, jobs):
    result = []
    if casttype in casts:
        for cast in casts[casttype]:
            if cast[u'department'] == department and cast[u'job'] in jobs and cast[u'name'] not in result:
                result.append(cast[u'name'])
    return result
