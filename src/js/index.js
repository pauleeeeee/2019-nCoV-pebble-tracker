// const moment = require('moment');
// const Papa = require('papaparse');

//require Clay for settings
var Clay = require('pebble-clay');
var clayConfig = require('./config.json');
var clay = new Clay(clayConfig, null, { autoHandleEvents: false });

//require message queueing system
var MessageQueue = require('message-queue-pebble');

var yourLocation = {
    lat: 0,
    long: 0
}

var lastSync = "";
var units = "M";
var customLocation = false;
var minCases = 5;

// localStorage.clear();


Pebble.addEventListener('showConfiguration', function(e) {
    Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('webviewclosed', function(e) {
    if (e && !e.response) {
        return;
    }
    console.log('web view closed');
    var configuration = clay.getSettings(e.response, false);
    if (configuration.CustomLat.value && configuration.CustomLat.value != "" && configuration.CustomLong.value && configuration.CustomLong.value != "") {
        yourLocation.lat = configuration.CustomLat.value;
        yourLocation.long = configuration.CustomLong.value;
        customLocation = true;
        localStorage.setItem("customLocation", JSON.stringify(yourLocation));
        console.log('custom location detected and set');
    } else {
        customLocation = false;
        localStorage.removeItem("customLocation");
    }
    // console.log('custom units: ' + configuration.CustomUnits.value);
    units = configuration.CustomUnits.value;
    localStorage.setItem("customUnits", units);
    minCases = configuration.MinCases.value;
    localStorage.setItem("minCases", minCases);
    fetchAll();
});

Pebble.addEventListener('ready', function() {
    lastSync = localStorage.getItem("lastSync");
    var customLocation = JSON.parse(localStorage.getItem("customLocation"));
    if (customLocation){
        yourLocation = customLocation;
        customLocation = true;
    };
    var customUnits = localStorage.getItem("customUnits");
    if (customUnits) {
        units = customUnits;
    }
    cases = localStorage.getItem("minCases");
    if(cases){
        minCases = cases;
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
    var req = new XMLHttpRequest();
    req.open('GET', "https://corona.lmao.ninja/jhucsse", true);
    req.onload = function(e) {
        if (req.readyState == 4) {
          // 200 - HTTP OK
            if(req.status == 200) {
                var locations = JSON.parse(req.responseText);

                //example location entry returned by the API
                //{"country":"China",
                //"province":"Hubei",
                //"updatedAt":"2020-03-21T10:13:08",
                //"stats":{"confirmed":"67800","deaths":"3139","recovered":"58946"},
                //"coordinates":{"latitude":"30.9756","longitude":"112.2707"}}

                //world stats
                var worldStats = {
                    "Location": "World",
                    "Cases": 0,
                    "Deaths": 0,
                    "Recovered": 0,
                    "Scope": 2
                };


                for (let location of locations){
                    //calculate distance
                    location.distance = Math.round(distance(location.coordinates.latitude, location.coordinates.longitude,  yourLocation.lat, yourLocation.long, units));
                    
                    //add to world stats
                    worldStats.Cases += Number(location.stats.confirmed);
                    worldStats.Deaths += Number(location.stats.deaths);
                    worldStats.Recovered += Number(location.stats.recovered);
                 }

                 worldStats.Cases += "";
                 worldStats.Deaths += "";
                 worldStats.Recovered += "";

                //sort locations by distance from current location, ascending
                locations.sort((a, b) => (a.distance > b.distance) ? 1 : -1);
                // console.log(JSON.stringify(locations[0]));
                // console.log(worldStats);
                var regionalStats = {
                    "Location": "",
                    // "Cases":locations[0].stats.confirmed+"",
                    // "Deaths":locations[0].stats.deaths+"",
                    // "Recovered":locations[0].stats.recovered+"",
                    "Cases":0,
                    "Deaths":0,
                    "Recovered":0,
                    "Scope": 0
                };

                //check to see if a county is not listed with provinces. If it is not, report both regional and country as country
                var countryString;
                var countryStats;
                if (locations[0].province == null || locations[0].province == locations[0].country){
                    countryStats = {
                        "Location": locations[0].country,
                        "Cases":locations[0].stats.confirmed+"",
                        "Deaths":locations[0].stats.deaths+"",
                        "Recovered":locations[0].stats.recovered+"",
                        "Scope": 1
                    }
                    regionalStats.Location = locations[0].country;
                } else if (locations[0].province != locations[0].country && locations[0].province != null){
                    regionalStats.Location = locations[0].province;
                    countryString = locations[0].country;
                    for (let location of locations){
                        if (location.province == regionalStats.Location){
                            console.log(JSON.stringify(location));
                            regionalStats.Cases += Number(location.stats.confirmed);
                            regionalStats.Deaths += Number(location.stats.deaths);
                            regionalStats.Recovered += Number(location.stats.recovered);
                        }
                    }
                    regionalStats.Cases += "";
                    regionalStats.Deaths += "";
                    regionalStats.Recovered += "";
   
                }
                
                //if country stats hasn't been established, build the object by adding all provinces together
                if (!countryStats) {
                    var stats = {
                        "confirmed":0,
                        "deaths":0,
                        "recovered":0
                    }
                    for (let location of locations){
                        if (location.country == countryString){
                            stats.confirmed += Number(location.stats.confirmed);
                            stats.deaths += Number(location.stats.deaths);
                            stats.recovered += Number(location.stats.recovered);
                        }
                    }
                    countryStats = {
                        "Location": countryString,
                        "Cases":stats.confirmed+"",
                        "Deaths":stats.deaths+"",
                        "Recovered":stats.recovered+"",
                        "Scope": 1
                    }
                }

                console.log(JSON.stringify(worldStats));
                console.log(JSON.stringify(countryStats));
                console.log(JSON.stringify(regionalStats));
                MessageQueue.sendAppMessage(regionalStats);
                MessageQueue.sendAppMessage(countryStats);
                MessageQueue.sendAppMessage(worldStats);
                MessageQueue.sendAppMessage({"Ready":1});

            }
        }
    }
    req.send();

}

//old papaparse functionality
// function getCoronaData2(){
//     Papa.parse('https://raw.githubusercontent.com/CSSEGISandData/COVID-19/master/csse_covid_19_data/csse_covid_19_time_series/time_series_19-covid-Confirmed.csv',
//     {
//         download:true,
//         complete:(results)=>{
//             var modified = [];
//             for (let location of results.data){
//                 var count = location[location.length-1];
//                 if (count>minCases){
//                     var locString = "";
//                     if(location[0].length > 1){
//                         locString = location[0] + ", " + location[1];
//                     } else {
//                         locString = location[1];
//                     }
//                     var loc = {
//                         location:locString,
//                         lat: location[2],
//                         long: location[3],
//                         distance: Math.round(distance(location[2], location[3],  yourLocation.lat, yourLocation.long, units)),
//                         count:count
//                     }
//                     if(!locString.includes("Diamond Princess")){
//                         modified.push(loc);
//                     }
//                 }
//             }
//             modified.sort((a, b) => (a.distance > b.distance) ? 1 : -1);
//             console.log(JSON.stringify(modified[0]));
//             var closestLocations = "";
//             for (let i=0; i < 5; i++){
//                 closestLocations += modified[i].location + "\n" + modified[i].distance+displayUnits(units)+", " + modified[i].count + " cases" + "\n" ;
//             }
//             var virusData = {
//                 // "Distance": modified[0].distance+displayUnits(units),
//                 "Scope": 0,
//                 "Location": modified[0].location,
//                 "Cases": modified[0].count + "",
//                 "Deaths": 0+" ",
//                 "Recovered": 0+" ",
//                 "Ready": 1,
//                 // "ClosestCases": closestLocations
//             };
//             console.log(JSON.stringify(virusData));
//             // localStorage.setItem("lastSync", moment().format("MM-DD-YYYY"));
//             localStorage.setItem("lastSync", new Date().getTime());
//             localStorage.setItem("virusData", JSON.stringify(virusData));
//             Pebble.sendAppMessage(virusData);
//             // console.log(JSON.stringify(modified));
//         },
//         error:()=>{
//             Pebble.showSimpleNotificationOnPebble("Error", "Error in parsing results from webserver.");
//         }
//     });
// }


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