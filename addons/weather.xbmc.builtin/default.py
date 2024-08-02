import sys, urllib, urllib2, json, xbmc, xbmcgui, xbmcaddon
from urlparse import urlparse, parse_qs

heading = "Weather API"


class Location:
    def __init__(self, id, name, region, country, lat, lon):
        self.id = id
        self.name = name
        self.region = region
        self.country = country
        self.lat = lat
        self.lon = lon


class WeatherAPI:
    api = "http://api.weatherapi.com/v1/search.json"

    def make_request(self, url):
        try:
            request = urllib2.Request(url)
            request.add_header("User-Agent", "XBMC4Xbox/4.0")
            response = urllib2.urlopen(request)
            response_data = response.read()
            body = json.loads(response_data)
            return body
        except Exception as e:
            xbmc.log(
                "Weather API: error when calling: {0}\n Error {1}".format(url, str(e))
            )
            xbmcgui.Dialog().notification(
                heading, xbmc.getLocalizedString(15301), xbmcgui.NOTIFICATION_ERROR
            )
        return None

    def search_location(self, query):
        url = "{0}?{1}".format(
            self.api,
            urllib.urlencode({"key": "e74f39b17d374e86ad4233506220912", "q": query}),
        )
        return self.make_request(url)


def run(id, addon):
    kbd = xbmc.Keyboard("", xbmc.getLocalizedString(14024), False)
    kbd.doModal()
    if not kbd.isConfirmed():
        return
    query = kbd.getText()

    service = WeatherAPI()
    search_result = service.search_location(query)
    if search_result is None:
        return

    locations = []
    labels = []
    for item in search_result:
        location = Location(
            item["id"],
            item["name"],
            item["region"],
            item["country"],
            item["lat"],
            item["lon"],
        )
        locations.append(location)
        labels.append("{0}, {1}".format(item["name"], item["country"]))

    if locations:
        selected = xbmcgui.Dialog().select(xbmc.getLocalizedString(396), labels)
        if selected != -1:
            selected_location = locations[selected]
            addon.setSetting(
                "Location{}".format(id),
                "{0}, {1}".format(selected_location.name, selected_location.country),
            )
            addon.setSetting("Location{}ID".format(id), str(selected_location.id))
            addon.setSetting(
                "Location{}LAT".format(id), str(selected_location.lat)
            )
            addon.setSetting(
                "Location{}LON".format(id), str(selected_location.lon)
            )
    else:
        xbmcgui.Dialog().ok(
            "Weather API (Internal Weather)", xbmc.getLocalizedString(284), "", ""
        )
        run(id, addon)


if __name__ == "__main__":
    addon = xbmcaddon.Addon()

    parsed_query = urlparse("?" + sys.argv[1])
    params = parse_qs(parsed_query.query)
    id_param = params.get("id", [None])[0]

    run(id_param, addon)
