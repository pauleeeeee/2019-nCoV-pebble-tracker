const moment = require('moment');
const Papa = require('papaparse');
var Clay = require('pebble-clay');
var clayConfig = require('./config.json');
var clay = new Clay(clayConfig, null, { autoHandleEvents: false });

var yourLocation = {
    lat: 0,
    long: 0
}

var lastSync = "";
var units = "M";
var customLocation = false;

// localStorage.clear();


Pebble.addEventListener('showConfiguration', function(e) {
    Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('webviewclosed', function(e) {
    if (e && !e.response) {
        return;
    }
    var configuration = clay.getSettings(e.response, false);
    if (configuration.CustomLat.value && configuration.CustomLat.value != "" & configuration.CustomLong.value && configuration.CustomLong.value != "") {
        yourLocation.lat = configuration.CustomLat.value;
        yourLocation.long = configuration.CustomLong.value;
        customLocation = true;
        localStorage.setItem("customLocation", JSON.stringify(yourLocation));
    } else {
        customLocation = false;
        localStorage.removeItem("customLocation");
    }
    // console.log('custom units: ' + configuration.CustomUnits.value);
    units = configuration.CustomUnits.value;
    localStorage.setItem("customUnits", units);
    fetchAll();
});

Pebble.addEventListener('ready', function() {
    lastSync = localStorage.getItem("lastSync");
    var customLocation = localStorage.getItem("customLocation");
    if (customLocation){
        yourLocation = customLocation;
        customLocation = true;
    };
    var customUnits = localStorage.getItem("customUnits");
    if (customUnits) {
        units = customUnits;
    }
    // if (lastSync == "" || lastSync != moment().format("MM-DD-YYYY")){
    if (lastSync == "" || (new Date().getTime() - lastSync) > 3.6e+6 ){
        fetchAll();
        console.log('fetching fresh');
    } else {
        var virusData = JSON.parse(localStorage.getItem("virusData"));
        Pebble.sendAppMessage(virusData);
        console.log('sent cached');
    };
    // navigator.geolocation.getCurrentPosition(success, error, options);

});

Pebble.addEventListener('appmessage', function(e) {
    var doUpdate = e.payload["RequestUpdate"];
    if(doUpdate){
        fetchAll();
    }
});

function success(pos) {
    yourLocation.lat = pos.coords.latitude;
    yourLocation.long = pos.coords.longitude;
    getCoronaData();
}

function error(err) {
    console.log('location error (' + err.code + '): ' + err.message);
    Pebble.showSimpleNotificationOnPebble("Error", "Geolocation fetch failed.");
}

// Choose options about the data returned
var options = {
    enableHighAccuracy: true,
    timeout: 10000
};

function fetchAll(){
    if (customLocation) {
        getCoronaData();
    } else {
        navigator.geolocation.getCurrentPosition(success, error, options);
    }
}

function getCoronaData(){
    Papa.parse('https://raw.githubusercontent.com/CSSEGISandData/COVID-19/master/csse_covid_19_data/csse_covid_19_time_series/time_series_19-covid-Confirmed.csv',
    {
        download:true,
        complete:(results)=>{
            var modified = [];
            for (let location of results.data){
                var count = location[location.length-1];
                if (count>4){
                    var locString = "";
                    if(location[0].length > 1){
                        locString = location[0] + ", " + location[1];
                    } else {
                        locString = location[1];
                    }
                    var loc = {
                        location:locString,
                        lat: location[2],
                        long: location[3],
                        distance: Math.round(distance(location[2], location[3],  yourLocation.lat, yourLocation.long, units)),
                        count:count
                    }
                    if(!locString.includes("Diamond Princess")){
                        modified.push(loc);
                    }
                }
            }
            modified.sort((a, b) => (a.distance > b.distance) ? 1 : -1);
            console.log(JSON.stringify(modified[0]));
            var closestLocations = "";
            for (let i=0; i < 5; i++){
                closestLocations += modified[i].location + "\n" + modified[i].distance+displayUnits(units)+", " + modified[i].count + " cases" + "\n" ;
            }
            var virusData = {
                "Distance": modified[0].distance+displayUnits(units),
                "Location": modified[0].location,
                "LocationCases": modified[0].count + " cases",
                "ClosestCases": closestLocations
            };
            // localStorage.setItem("lastSync", moment().format("MM-DD-YYYY"));
            localStorage.setItem("lastSync", new Date().getTime());
            localStorage.setItem("virusData", JSON.stringify(virusData));
            Pebble.sendAppMessage(virusData);
            // console.log(JSON.stringify(modified));
        },
        error:()=>{
            Pebble.showSimpleNotificationOnPebble("Error", "Error in parsing results from webserver.");
        }
    });
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//:::                                                                         :::
//:::  This routine calculates the distance between two points (given the     :::
//:::  latitude/longitude of those points). It is being used to calculate     :::
//:::  the distance between two locations using GeoDataSource (TM) prodducts  :::
//:::                                                                         :::
//:::  Definitions:                                                           :::
//:::    South latitudes are negative, east longitudes are positive           :::
//:::                                                                         :::
//:::  Passed to function:                                                    :::
//:::    lat1, lon1 = Latitude and Longitude of point 1 (in decimal degrees)  :::
//:::    lat2, lon2 = Latitude and Longitude of point 2 (in decimal degrees)  :::
//:::    unit = the unit you desire for results                               :::
//:::           where: 'M' is statute miles (default)                         :::
//:::                  'K' is kilometers                                      :::
//:::                  'N' is nautical miles                                  :::
//:::                                                                         :::
//:::  Worldwide cities and other features databases with latitude longitude  :::
//:::  are available at https://www.geodatasource.com                         :::
//:::                                                                         :::
//:::  For enquiries, please contact sales@geodatasource.com                  :::
//:::                                                                         :::
//:::  Official Web site: https://www.geodatasource.com                       :::
//:::                                                                         :::
//:::               GeoDataSource.com (C) All Rights Reserved 2018            :::
//:::                                                                         :::
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

function distance(lat1, lon1, lat2, lon2, unit) {
	if ((lat1 == lat2) && (lon1 == lon2)) {
		return 0;
	}
	else {
		var radlat1 = Math.PI * lat1/180;
		var radlat2 = Math.PI * lat2/180;
		var theta = lon1-lon2;
		var radtheta = Math.PI * theta/180;
		var dist = Math.sin(radlat1) * Math.sin(radlat2) + Math.cos(radlat1) * Math.cos(radlat2) * Math.cos(radtheta);
		if (dist > 1) {
			dist = 1;
		}
		dist = Math.acos(dist);
		dist = dist * 180/Math.PI;
		dist = dist * 60 * 1.1515;
		if (unit=="K") { dist = dist * 1.609344 }
		if (unit=="N") { dist = dist * 0.8684 }
		return dist;
	}
}

function displayUnits(unit){
    switch(unit){
        case "M": return "mi";
        case "K": return "km";
        case "N": return "na";
    }
}