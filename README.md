# 2019-nCoV-pebble-tracker
Pebble smartwatch app that shows you the nearest outbreak location and number of cases. This app shows you the nearest outbreak of the COVID 19 (2019-nCoV) virus aka 'Coronavirus' to your location. The data is supplied by Johns Hopkins University Center for Systems Science and Engineering (JHU CCSE) (https://github.com/CSSEGISandData/COVID-19).

The datasource is updated once daily, around 23:59 (UTC). This app filters locations by two criteria:
1) Locations with 5 or fewer reported cases of the virus are not considered.
2) All cases listed in the dataset associated with the Diamond Princess cruise ship are not considered.

You can press the up button to see the four nearest locations. More features to come. This app is tested and working on Android. Some iOS users have reported that the app launches but sits at the 'loading' screen. If this happens to you try manually setting your location coordinates in the boxes below and press 'SAVE'.

Tested and working on Android. Does not appear to work on iOS (the custom papaparse package is the likely culprit). Open to pull requests to make it available for both platforms!
