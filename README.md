# 2019-nCoV-pebble-tracker
Pebble smartwatch app that shows you the nearest outbreak location and number of cases. This app shows you the nearest outbreak of the COVID 19 (2019-nCoV) virus aka 'Coronavirus' to your location. The data is supplied by Johns Hopkins University Center for Systems Science and Engineering (JHU CCSE) (https://github.com/CSSEGISandData/COVID-19). In the latest version (v2.0.0 onward), the same data is pulled, but instead from this endpoint: https://github.com/novelcovid/api

The datasource is updated once daily, around 23:59 (UTC). This app filters locations by two criteria:
1) Locations with 5 (or a user defined number) or fewer reported cases of the virus are not considered.
2) All cases listed in the dataset associated with the Diamond Princess cruise ship are not considered.

If your country has province / state level data... your nearest province or state will be shown on the first screen. Then you can press down to scroll to your country's data and then again to the world's data.

Tested and working on Android. Does not appear to work on iOS. Open to pull requests to make it available for both platforms!
