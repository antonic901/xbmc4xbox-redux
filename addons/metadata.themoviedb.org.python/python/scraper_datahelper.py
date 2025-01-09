from __future__ import absolute_import
import re
try:
    from urlparse import parse_qsl
except ImportError: # py2 / py3
    from urlparse import parse_qsl

# get addon params from the plugin path querystring
def get_params(argv):
    result = {u'handle': int(argv[0])}
    if len(argv) < 2 or not argv[1]:
        return result

    result.update(parse_qsl(argv[1].lstrip(u'?')))
    return result

def combine_scraped_details_info_and_ratings(original_details, additional_details):
    def update_or_set(details, key, value):
        if key in details:
            details[key].update(value)
        else:
            details[key] = value

    if additional_details:
        if additional_details.get(u'info'):
            update_or_set(original_details, u'info', additional_details[u'info'])
        if additional_details.get(u'ratings'):
            update_or_set(original_details, u'ratings', additional_details[u'ratings'])
    return original_details

def combine_scraped_details_available_artwork(original_details, additional_details):
    if additional_details and additional_details.get(u'available_art'):
        available_art = additional_details[u'available_art']
        if not original_details.get(u'available_art'):
            original_details[u'available_art'] = available_art
        else:
            for arttype, artlist in available_art.items():
                original_details[u'available_art'][arttype] = \
                    artlist + original_details[u'available_art'].get(arttype, [])

    return original_details

def find_uniqueids_in_text(input_text):
    result = {}
    res = re.search(ur'(themoviedb.org/movie/)([0-9]+)', input_text)
    if (res):
        result[u'tmdb'] = res.group(2)
    res = re.search(ur'imdb....?/title/tt([0-9]+)', input_text)
    if (res):
        result[u'imdb'] = u'tt' + res.group(1)
    else:
        res = re.search(ur'imdb....?/Title\?t{0,2}([0-9]+)', input_text)
        if (res):
            result[u'imdb'] = u'tt' + res.group(1)
    return result
