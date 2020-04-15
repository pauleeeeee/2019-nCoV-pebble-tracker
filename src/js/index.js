//require Clay for settings
var Clay = require('pebble-clay');
var clayConfig = require('./config.json');
var clay = new Clay(clayConfig, null, { autoHandleEvents: false });

//require message queueing system
var MessageQueue = require('message-queue-pebble');

var country = '';
var stateProvince = '';

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
    // console.log(JSON.stringify(configuration));
    localStorage.setItem("configuration", JSON.stringify(configuration));
    country = configuration.Country.value;
    stateProvince = configuration.StateProvince.value;
    fetchAll();
});

Pebble.addEventListener('ready', function() {
    // lastSync = localStorage.getItem("lastSync");
    var configuration = JSON.parse(localStorage.getItem("configuration"));
    // console.log(JSON.stringify(configuration));
    if (configuration){
        country = configuration.Country.value;
        stateProvince = configuration.StateProvince.value;
    } else {
        country = "USA";
        stateProvince = "California";
        Pebble.showSimpleNotificationOnPebble("Configuration needed", "Visit the app's settings page inside the Pebble phone app to configure your location.");
    }
    fetchAll();
});

Pebble.addEventListener('appmessage', function(e) {
    var doUpdate = e.payload["RequestUpdate"];
    if(doUpdate){
        fetchAll();
    }
});


function fetchAll(){
    getWorld();
}

function getWorld(){
    var worldStats = {
        'Location': 'World',
        'Cases': 0,
        'Deaths': 0,
        'Recovered': 0,
        'Scope': 2
    };
    var req = new XMLHttpRequest();
    req.open('GET', "https://corona.lmao.ninja/all", true);
    req.onload = function(e) {
        if (req.readyState == 4) {
          // 200 - HTTP OK
            if(req.status == 200) {
                var worldData = JSON.parse(req.responseText);
                //{"cases":468012,"deaths":21180,"recovered":113809,"updated":1585179172282}
                worldStats.Cases = filterNumber(worldData.cases);
                worldStats.Deaths = worldData.deaths + "";
                worldStats.Recovered = worldData.recovered + "";
                console.log('sending world');
                console.log(JSON.stringify(worldStats));
                MessageQueue.sendAppMessage(worldStats, onSuccess, onFailure);
                getCountry();
            }
        }
    }
    req.send();
};

function onSuccess(data){
    console.log('success');
    console.log(JSON.stringify(data));
}

function onFailure(data, error){
    console.log('error');
    console.log(JSON.stringify(data));
    console.log(JSON.stringify(error));

}

function getCountry(){
    var req = new XMLHttpRequest();
    req.open('GET', "https://corona.lmao.ninja/countries/" + country, true);
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
                    'Location': country,
                    'Cases': filterNumber(data.cases),
                    'Deaths': data.deaths + "",
                    'Recovered': data.recovered + "",
                    'Scope': 1
                };
                console.log('sending country');
                console.log(JSON.stringify(countryData));
                MessageQueue.sendAppMessage(countryData);
                if (country == "USA") {
                    getState();
                } else {
                    MessageQueue.sendAppMessage({'Ready':2})
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
                for ( i = 0; i < states.length; i++){
                    if (states[i].state == stateProvince){
                        stateInfo = states[i];
                        break;
                    }
                }
                var stateData = {
                    'Location': stateInfo.state,
                    'Cases': stateInfo.cases + "",
                    'Deaths': stateInfo.deaths + "",
                    'Recovered': "?" + "",
                    'Scope': 0
                };
                console.log('sending state');
                console.log(JSON.stringify(stateData));

                MessageQueue.sendAppMessage(stateData);
                MessageQueue.sendAppMessage({'Ready':1})
            }
        }
    }
    req.send();

};

function filterNumber(number){
    if (number < 2000000){
        return number + "";
    } else if (number >= 2000000 ){
        number = (number/1000000).toFixed(1);
        number += "M";
        console.log(number);
        return number;
    }
}