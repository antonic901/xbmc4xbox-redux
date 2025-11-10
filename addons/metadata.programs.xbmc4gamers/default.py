import sys, os, datetime
import xml.etree.ElementTree as ET
import xbmc, xbmcaddon, xbmcplugin, xbmcgui

from urlparse import parse_qsl

ID = xbmcaddon.Addon().getAddonInfo(u'id')

def log(msg, level=xbmc.LOGDEBUG):
    xbmc.log(msg=u'[{addon}]: {msg}'.format(addon=ID, msg=msg), level=level)

# get addon params from the plugin path querystring
def get_params(argv):
    result = {u'handle': int(argv[0])}
    if len(argv) < 2 or not argv[1]:
        return result

    result.update(parse_qsl(argv[1].lstrip(u'?')))
    return result

def _get_values(items):
    return [item.strip() for item in items] if items else []

def load_metadata(root_dir):
    tree = ET.parse(os.path.join(root_dir, "_resources", "default.xml"))
    root = tree.getroot()

    info = {}

    mediatype = root.find(u'type')
    if mediatype is not None and mediatype.text:
        info[u'mediatype'] = mediatype.text

    system = root.find(u'system')
    if system is not None and system.text:
        info[u'system'] = system.text

    title = root.find(u'title')
    if title is not None and title.text:
        info[u'title'] = title.text

    developer = root.find(u'developer')
    if developer is not None and developer.text:
        info[u'developer'] = developer.text

    publisher = root.find(u'publisher')
    if publisher is not None and publisher.text:
        info[u'publisher'] = publisher.text

    features_general = root.find(u'features_general')
    if features_general is not None and features_general.text:
        general_features = features_general.text.split(", ")
        info[u'generalfeature'] = _get_values(general_features)

    features_online = root.find(u'features_online')
    if features_online is not None and features_online.text:
        online_features = features_online.text.split(", ")
        info[u'onlinefeature'] = _get_values(online_features)

    esrb = root.find(u'esrb')
    if esrb is not None and esrb.text:
        info[u'esrb'] = esrb.text

    genre = root.find(u'genre')
    if genre is not None and genre.text:
        genres = genre.text.split(", ")
        info[u'genre'] = _get_values(genres)

    release_date = root.find("release_date")
    if release_date is not None and release_date.text:
        try:
            date_obj = datetime.datetime.strptime(release_date.text, "%d %b %Y")
            release_date_formatted = date_obj.strftime("%Y-%m-%d")
        except:
            release_date_formatted = "1970-01-01"
        info[u'releasedate'] = release_date_formatted

    year = root.find(u'year')
    if year is not None and year.text:
        info[u'year'] = year.text

    rating = root.find(u'rating')
    if rating is not None and rating.text:
        info[u'rating'] = rating.text

    platform = root.find(u'platform')
    if platform is not None and platform.text:
        platforms = platform.text.split(", ")
        info[u'platform'] = _get_values(platforms)

    exclusive = root.find(u'exclusive')
    if exclusive is not None and exclusive.text:
        info[u'exclusive'] = exclusive.text

    overview = root.find(u'overview')
    if overview is not None and overview.text:
        info[u'overview'] = overview.text

    trailer = os.path.join(root_dir, "_resources", "media", "preview.mp4")
    if os.path.exists(trailer):
        info[u'trailer'] = trailer

    return info

def get_details(programPath, handle):
    root_dir = os.path.dirname(programPath)
    info = load_metadata(root_dir)

    listitem = xbmcgui.ListItem(info[u'title'], offscreen=True)
    listitem.setInfo('program', info)

    xbmcplugin.setResolvedUrl(handle=handle, succeeded=True, listitem=listitem)

def run():
    params = get_params(sys.argv[1:])
    enddir = True
    action = params.get(u"action")
    if action == u'getdetails' and u'url' in params:
        enddir = not get_details(params[u"url"], params[u'handle'])
    else:
        log(u"unhandled action: " + action, xbmc.LOGWARNING)
    if enddir:
        xbmcplugin.endOfDirectory(params[u'handle'])

if __name__ == u'__main__':
    run()
