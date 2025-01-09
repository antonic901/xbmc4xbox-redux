def configure_scraped_details(details, settings):
    details = _configure_rating_prefix(details, settings)
    details = _configure_keeporiginaltitle(details, settings)
    details = _configure_trailer(details, settings)
    details = _configure_multiple_studios(details, settings)
    details = _configure_default_rating(details, settings)
    details = _configure_tags(details, settings)
    return details

def configure_tmdb_artwork(details, settings):
    if u'available_art' not in details:
        return details

    art = details[u'available_art']
    fanart_enabled = bool(settings.getSetting(u'fanart'))
    if not fanart_enabled:
        if u'fanart' in art:
            del art[u'fanart']
        if u'set.fanart' in art:
            del art[u'set.fanart']
    if not bool(settings.getSetting(u'landscape')):
        if u'landscape' in art:
            if fanart_enabled:
                art[u'fanart'] = art.get(u'fanart', []) + art[u'landscape']
            del art[u'landscape']
        if u'set.landscape' in art:
            if fanart_enabled:
                art[u'set.fanart'] = art.get(u'set.fanart', []) + art[u'set.landscape']
            del art[u'set.landscape']

    return details

def is_fanarttv_configured(settings):
    return bool(settings.getSetting(u'enable_fanarttv_artwork'))

def _configure_rating_prefix(details, settings):
    if details[u'info'].get(u'mpaa'):
        details[u'info'][u'mpaa'] = settings.getSetting(u'certprefix') + details[u'info'][u'mpaa']
    return details

def _configure_keeporiginaltitle(details, settings):
    if bool(settings.getSetting(u'keeporiginaltitle')):
        details[u'info'][u'title'] = details[u'info'][u'originaltitle']
    return details

def _configure_trailer(details, settings):
    if details[u'info'].get(u'trailer') and not bool(settings.getSetting(u'trailer')):
        del details[u'info'][u'trailer']
    return details

def _configure_multiple_studios(details, settings):
    if not bool(settings.getSetting(u'multiple_studios')):
        details[u'info'][u'studio'] = details[u'info'][u'studio'][:1]
    return details

def _configure_default_rating(details, settings):
    imdb_default = bool(details[u'ratings'].get(u'imdb')) and settings.getSetting(u'RatingS') == u'IMDb'
    trakt_default = bool(details[u'ratings'].get(u'trakt')) and settings.getSetting(u'RatingS') == u'Trakt'
    default_rating = u'themoviedb'
    if imdb_default:
        default_rating = u'imdb'
    elif trakt_default:
        default_rating = u'trakt'
    if default_rating not in details[u'ratings']:
        default_rating = list(details[u'ratings'].keys())[0] if details[u'ratings'] else None
    for rating_type in details[u'ratings'].keys():
        details[u'ratings'][rating_type][u'default'] = rating_type == default_rating
    return details

def _configure_tags(details, settings):
    if not bool(settings.getSetting(u'add_tags')):
        del details[u'info'][u'tag']
    return details

# pylint: disable=invalid-name
try:
    basestring
except NameError: # py2 / py3
    basestring = unicode

#pylint: disable=redefined-builtin
class PathSpecificSettings(object):
    # read-only shim for typed `xbmcaddon.Addon().getSetting*` methods
    def __init__(self, settings_dict, log_fn):
        self.data = settings_dict
        self.log = log_fn

    def getSettingBool(self, id):
        return self._inner_get_setting(id, bool, False)

    def getSettingInt(self, id):
        return self._inner_get_setting(id, int, 0)

    def getSettingNumber(self, id):
        return self._inner_get_setting(id, float, 0.0)

    def getSettingString(self, id):
        return self._inner_get_setting(id, basestring, u'')

    def _inner_get_setting(self, setting_id, setting_type, default):
        value = self.data.get(setting_id)
        if isinstance(value, setting_type):
            return value
        self._log_bad_value(value, setting_id)
        return default

    def _log_bad_value(self, value, setting_id):
        if value is None:
            self.log(u"requested setting ({0}) was not found.".format(setting_id))
        else:
            self.log(u'failed to load value "{0}" for setting {1}'.format(value, setting_id))
