// const moment = require('moment');
// const Papa = require('papaparse');
const point2place = require('point2place');

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
var useCustomLocation = false;

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
    console.log(JSON.stringify(configuration))

    if (configuration.CustomLat.value != "" && configuration.CustomLong.value != "") {
        yourLocation.lat = configuration.CustomLat.value;
        yourLocation.long = configuration.CustomLong.value;
        useCustomLocation = true;
        localStorage.setItem("customLocation", JSON.stringify(yourLocation));
        // console.log('custom location detected and set');
    } else {
        useCustomLocation = false;
        localStorage.removeItem("customLocation");
    }
    fetchAll();
});

Pebble.addEventListener('ready', function() {
    // lastSync = localStorage.getItem("lastSync");
    var customLocation = JSON.parse(localStorage.getItem("customLocation"));
    console.log(JSON.stringify(customLocation));
    if (customLocation){
        // console.log('custom location detected in ready listener');
        yourLocation = customLocation;
        useCustomLocation = true;
    };
    if (lastSync == "" || (new Date().getTime() - lastSync) > 3.6e+6 ){
        fetchAll();
        // console.log('fetching fresh');
    } else {
        var virusData = JSON.parse(localStorage.getItem("virusData"));
        Pebble.sendAppMessage(virusData);
        // console.log('sent cached');
    };

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
    if (useCustomLocation) {
        // console.log('custom location caught in fetchAll');
        getCoronaData();
    } else {
        navigator.geolocation.getCurrentPosition(success, error, options);
    }
}

var place;

function getCoronaData(){
    place = point2place([yourLocation.long, yourLocation.lat]);
    // console.log(JSON.stringify(place));
    // var example = {
    //     "country":{"id":"USA","continent":"North America","name":"United States of America","name2":"United States","population":326625791,"income":"1. High income: OECD"},
    //     "nearestCity":{"name":"Denver","state_province":"Colorado","population":2313000,"sovereign":"United States of America","distanceKm":17.629558350620453}
    // };
   getWorld();
}

function getWorld(){
    var worldStats = {
        "Location": "World",
        "Cases": 0,
        "Deaths": 0,
        "Recovered": 0,
        "Scope": 2
    };
    var req = new XMLHttpRequest();
    req.open('GET', "https://corona.lmao.ninja/all", true);
    req.onload = function(e) {
        if (req.readyState == 4) {
          // 200 - HTTP OK
            if(req.status == 200) {
                var worldData = JSON.parse(req.responseText);
                //{"cases":468012,"deaths":21180,"recovered":113809,"updated":1585179172282}
                worldStats.Cases = worldData.cases + "";
                worldStats.Deaths = worldData.deaths + "";
                worldStats.Recovered = worldData.recovered + "";
                MessageQueue.sendAppMessage(worldStats);
                getCountry();
            }
        }
    }
    req.send();
};

function getCountry(){
    var req = new XMLHttpRequest();
    req.open('GET', "https://corona.lmao.ninja/countries/" + place.country.name, true);
    req.onload = function(e) {
        if (req.readyState == 4) {
          // 200 - HTTP OK
            if(req.status == 200) {
                var data = JSON.parse(req.responseText);
                console.log(JSON.stringify(data));
                //{
                //  "country":"USA",
                //  "countryInfo":{"iso2":"NO DATA","iso3":"NO DATA","_id":"NO DATA","lat":0,"long":0,"flag":"https://raw.githubusercontent.com/NovelCOVID/API/master/assets/flags/unknow.png"},
                //   "cases":65652,"todayCases":10796,"deaths":931,"todayDeaths":151,"recovered":394,"active":64327,"critical":1411,"casesPerOneMillion":198,"deathsPerOneMillion":3
                // }
                var countryData = {
                    "Location": place.country.name,
                    "Cases": data.cases + "",
                    "Deaths": data.deaths + "",
                    "Recovered": data.recovered + "",
                    "Scope": 1
                }
                MessageQueue.sendAppMessage(countryData);
                if (place.country.id == "USA") {
                    getState();
                } else {
                    MessageQueue.sendAppMessage({"Ready":2})
                }
            }
        }
    }
    req.send();


};

function getState(){
    var req = new XMLHttpRequest();
    req.open('GET', "https://corona.lmao.ninja/states/", true);
    req.onload = function(e) {
        if (req.readyState == 4) {
          // 200 - HTTP OK
            if(req.status == 200) {
                var states = JSON.parse(req.responseText);
                var stateInfo;
                //{"state":"Colorado","cases":1086,"todayCases":174,"deaths":19,"todayDeaths":8,"active":1067}
                for (let state of states){
                    if (state.state == place.nearestCity.state_province){
                        stateInfo = state;
                        break;
                    }
                }
                var stateData = {
                    "Location": stateInfo.state,
                    "Cases": stateInfo.cases + "",
                    "Deaths": stateInfo.deaths + "",
                    "Recovered": "?" + "",
                    "Scope": 0
                }
                MessageQueue.sendAppMessage(stateData);
                MessageQueue.sendAppMessage({"Ready":1})
            }
        }
    }
    req.send();

};

// function getCoronaData3(){
//     var req = new XMLHttpRequest();
//     req.open('GET', "https://corona.lmao.ninja/jhucsse", true);
//     req.onload = function(e) {
//         if (req.readyState == 4) {
//           // 200 - HTTP OK
//             if(req.status == 200) {
//                 var locations = JSON.parse(req.responseText);

//                 //example location entry returned by the API
//                 //{"country":"China",
//                 //"province":"Hubei",
//                 //"updatedAt":"2020-03-21T10:13:08",
//                 //"stats":{"confirmed":"67800","deaths":"3139","recovered":"58946"},
//                 //"coordinates":{"latitude":"30.9756","longitude":"112.2707"}}

//                 //world stats
//                 var worldStats = {
//                     "Location": "World",
//                     "Cases": 0,
//                     "Deaths": 0,
//                     "Recovered": 0,
//                     "Scope": 2
//                 };


//                 for (let location of locations){
//                     //calculate distance
//                     location.distance = Math.round(distance(location.coordinates.latitude, location.coordinates.longitude,  yourLocation.lat, yourLocation.long, units));
                    
//                     //add to world stats
//                     worldStats.Cases += Number(location.stats.confirmed);
//                     worldStats.Deaths += Number(location.stats.deaths);
//                     worldStats.Recovered += Number(location.stats.recovered);
//                  }

//                  worldStats.Cases += "";
//                  worldStats.Deaths += "";
//                  worldStats.Recovered += "";

//                 //sort locations by distance from current location, ascending
//                 locations.sort((a, b) => (a.distance > b.distance) ? 1 : -1);
//                 // console.log(JSON.stringify(locations[0]));
//                 // console.log(worldStats);
//                 var regionalStats = {
//                     "Location": "",
//                     // "Cases":locations[0].stats.confirmed+"",
//                     // "Deaths":locations[0].stats.deaths+"",
//                     // "Recovered":locations[0].stats.recovered+"",
//                     "Cases":0,
//                     "Deaths":0,
//                     "Recovered":0,
//                     "Scope": 0
//                 };

//                 //check to see if a county is not listed with provinces. If it is not, report both regional and country as country
//                 var countryString;
//                 var countryStats;
//                 if (locations[0].province == null || locations[0].province == locations[0].country){
//                     countryStats = {
//                         "Location": locations[0].country,
//                         "Cases":locations[0].stats.confirmed+"",
//                         "Deaths":locations[0].stats.deaths+"",
//                         "Recovered":locations[0].stats.recovered+"",
//                         "Scope": 1
//                     }
//                     regionalStats.Location = locations[0].country;
//                 } else if (locations[0].province != locations[0].country && locations[0].province != null){
//                     regionalStats.Location = locations[0].province;
//                     countryString = locations[0].country;
//                     for (let location of locations){
//                         if (location.province == regionalStats.Location){
//                             console.log(JSON.stringify(location));
//                             regionalStats.Cases += Number(location.stats.confirmed);
//                             regionalStats.Deaths += Number(location.stats.deaths);
//                             regionalStats.Recovered += Number(location.stats.recovered);
//                         }
//                     }
//                     regionalStats.Cases += "";
//                     regionalStats.Deaths += "";
//                     regionalStats.Recovered += "";
   
//                 }
                
//                 //if country stats hasn't been established, build the object by adding all provinces together
//                 if (!countryStats) {
//                     var stats = {
//                         "confirmed":0,
//                         "deaths":0,
//                         "recovered":0
//                     }
//                     for (let location of locations){
//                         if (location.country == countryString){
//                             stats.confirmed += Number(location.stats.confirmed);
//                             stats.deaths += Number(location.stats.deaths);
//                             stats.recovered += Number(location.stats.recovered);
//                         }
//                     }
//                     countryStats = {
//                         "Location": countryString,
//                         "Cases":stats.confirmed+"",
//                         "Deaths":stats.deaths+"",
//                         "Recovered":stats.recovered+"",
//                         "Scope": 1
//                     }
//                 }

//                 console.log(JSON.stringify(worldStats));
//                 console.log(JSON.stringify(countryStats));
//                 console.log(JSON.stringify(regionalStats));
//                 MessageQueue.sendAppMessage(regionalStats);
//                 MessageQueue.sendAppMessage(countryStats);
//                 MessageQueue.sendAppMessage(worldStats);
//                 MessageQueue.sendAppMessage({"Ready":1});

//             }
//         }
//     }
//     req.send();

// }

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


// function distance(lat1, lon1, lat2, lon2, unit) {
// 	if ((lat1 == lat2) && (lon1 == lon2)) {
// 		return 0;
// 	}
// 	else {
// 		var radlat1 = Math.PI * lat1/180;
// 		var radlat2 = Math.PI * lat2/180;
// 		var theta = lon1-lon2;
// 		var radtheta = Math.PI * theta/180;
// 		var dist = Math.sin(radlat1) * Math.sin(radlat2) + Math.cos(radlat1) * Math.cos(radlat2) * Math.cos(radtheta);
// 		if (dist > 1) {
// 			dist = 1;
// 		}
// 		dist = Math.acos(dist);
// 		dist = dist * 180/Math.PI;
// 		dist = dist * 60 * 1.1515;
// 		if (unit=="K") { dist = dist * 1.609344 }
// 		if (unit=="N") { dist = dist * 0.8684 }
// 		return dist;
// 	}
// }

// function displayUnits(unit){
//     switch(unit){
//         case "M": return "mi";
//         case "K": return "km";
//         case "N": return "na";
//     }
// }