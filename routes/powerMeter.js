var express = require('express');
var router = express.Router();
var mysql = require('mysql');
var { io } = require('../socketIO');
var mqtt = require('mqtt');

var tabVan = [];
var tabVbn = [];
var tabVab = [];
var tabIa = [];
var tabIb = [];
var tabKw = [];
var tabKWh = [];
var tabFp = [];

var tabValVan = [];
var tabValVbn = [];
var tabValVab = [];
var tabValIa = [];
var tabValIb = [];
var tabValKw = [];
var tabValKWh = [];
var tabValFp = [];

var Van;
var Vbn;
var Vab;
var Ia;
var Ib;
var Kw;
var KWh;
var Fp;

var nivGB = 0;
var nivPB = 0;
var tmpPB = 0;
var valveGB = 0;
var valvePB = 0;
var valveEC = 0;
var valveEF = 0;
var valveEEC = "";
var valveEEF = "";
var pompe = "";

var demandeLogout = false;


function calculateAverage(tabDonnes) {
    var moyenne = 0;
    for (i = 0; i < tabDonnes.length; i++) {
        moyenne += parseFloat(tabDonnes[i]);
    }
    moyenne = moyenne / tabDonnes.length;
    return moyenne;
}

var client = mqtt.connect('mqtt://127.0.0.1:1883');

var connection = mysql.createConnection({
    host: 'localhost',
    user: 'root',
    password: '12345',
    database: 'users'
});

client.subscribe('RAM/alarmes/états/ALR_CNX_BAL');
client.subscribe('RAM/alarmes/états/ALR_CNX_POW');
client.subscribe('RAM/alarmes/états/ALR_CNX_MEL');
client.subscribe('RAM/alarmes/états/ALR_CNX_ASP');

client.subscribe('RAM/alarmes/états/ALR_GB_OVF');
client.subscribe('RAM/alarmes/états/ALR_PB_OVF');
client.subscribe('RAM/alarmes/états/ALR_GB_NIV_MAX');
client.subscribe('RAM/alarmes/états/ALR_PB_NIV_MAX');

router.get('/', function (req, res, next) {
    if (demandeLogout) {
        req.session.user = {
            id: 0,
            login: '',
            droit: 0,
            password: ''
        }
        demandeLogout = false;
        res.redirect('/');
    }
    else {
        client.subscribe('RAM/powermeter/etats/Van');
        client.subscribe('RAM/powermeter/etats/Vbn');
        client.subscribe('RAM/powermeter/etats/Vab');
        client.subscribe('RAM/powermeter/etats/Ia');
        client.subscribe('RAM/powermeter/etats/Ib');
        client.subscribe('RAM/powermeter/etats/KW');
        client.subscribe('RAM/powermeter/etats/KWh');
        client.subscribe('RAM/powermeter/etats/FP');

        client.subscribe('RAM/panneau/etats/NivGB');
        client.subscribe('RAM/panneau/etats/NivPB');
        client.subscribe('RAM/panneau/etats/TmpPB');
        client.subscribe('RAM/panneau/etats/ValveGB');
        client.subscribe('RAM/panneau/etats/ValvePB');
        client.subscribe('RAM/panneau/etats/ValveEC');
        client.subscribe('RAM/panneau/etats/ValveEF');
        client.subscribe('RAM/panneau/etats/ValveEEC');
        client.subscribe('RAM/panneau/etats/ValveEEF');
        client.subscribe('RAM/panneau/etats/Pompe');

        client.unsubscribe('RAM/melangeur/etats/recetteStatut');

        client.unsubscribe('RAM/shopvac/etats/sequence');
        client.unsubscribe('RAM/shopvac/etats/NivA');
        client.unsubscribe('RAM/shopvac/etats/NivB');
        client.unsubscribe('RAM/shopvac/etats/NivC');

        client.unsubscribe('RAM/balance/etats/poids');
        client.unsubscribe('RAM/balance/etats/tare');
        client.unsubscribe('RAM/balance/etats/unite');

        res.render('pages/powerMeter', { title: 'Power meter', utilisateur: req.session.user.login, password: req.session.user.password, droit: req.session.user.droit, tabValVan, tabValVbn, tabValVab, tabValIa, tabValIb, tabValKw, tabValKWh, tabValFp, nivGB, nivPB, tmpPB, valveGB, valvePB, valveEC, valveEF, valveEEC, valveEEF, pompe });
        console.log(req.session.user.login);
        console.log(req.session.user.password);
    }
});

client.on('message', function (topic, message) {

    if (topic == "RAM/powermeter/etats/Van") {
        Van = parseFloat(message).toFixed(2);

        tabVan.push(Van);

        tabValVan[0] = Van;
        tabValVan[1] = Math.min.apply(Math, tabVan).toFixed(2);
        tabValVan[2] = Math.max.apply(Math, tabVan).toFixed(2);
        tabValVan[3] = calculateAverage(tabVan).toFixed(2);

        io.sockets.emit('Van', tabValVan);
    }
    if (topic == "RAM/powermeter/etats/Vbn") {
        Vbn = parseFloat(message).toFixed(2);

        tabVbn.push(Vbn);

        tabValVbn[0] = Vbn;
        tabValVbn[1] = Math.min.apply(Math, tabVbn).toFixed(2);
        tabValVbn[2] = Math.max.apply(Math, tabVbn).toFixed(2);
        tabValVbn[3] = calculateAverage(tabVbn).toFixed(2);

        io.sockets.emit('Vbn', tabValVbn);
    }
    if (topic == "RAM/powermeter/etats/Vab") {
        Vab = parseFloat(message).toFixed(2);

        tabVab.push(Vab);

        tabValVab[0] = Vab;
        tabValVab[1] = Math.min.apply(Math, tabVab).toFixed(2);
        tabValVab[2] = Math.max.apply(Math, tabVab).toFixed(2);
        tabValVab[3] = calculateAverage(tabVab).toFixed(2);

        io.sockets.emit('Vab', tabValVab);
    }
    if (topic == "RAM/powermeter/etats/Ia") {
        Ia = parseFloat(message).toFixed(2);

        tabIa.push(Ia);

        tabValIa[0] = Ia;
        tabValIa[1] = Math.min.apply(Math, tabIa).toFixed(2);
        tabValIa[2] = Math.max.apply(Math, tabIa).toFixed(2);
        tabValIa[3] = calculateAverage(tabIa).toFixed(2);

        io.sockets.emit('Ia', tabValIa);
    }
    if (topic == "RAM/powermeter/etats/Ib") {
        Ib = parseFloat(message).toFixed(2);

        tabIb.push(Ib);

        tabValIb[0] = Ib;
        tabValIb[1] = Math.min.apply(Math, tabIb).toFixed(2);
        tabValIb[2] = Math.max.apply(Math, tabIb).toFixed(2);
        tabValIb[3] = calculateAverage(tabIb).toFixed(2);

        io.sockets.emit('Ib', tabValIb);
    }
    if (topic == "RAM/powermeter/etats/KW") {
        Kw = parseFloat(message).toFixed(2);

        tabKw.push(Kw);

        tabValKw[0] = Kw;
        tabValKw[1] = Math.min.apply(Math, tabKw).toFixed(2);
        tabValKw[2] = Math.max.apply(Math, tabKw).toFixed(2);
        tabValKw[3] = calculateAverage(tabKw).toFixed(2);

        io.sockets.emit('KW', tabValKw);
    }
    if (topic == "RAM/powermeter/etats/KWh") {
        KWh = parseFloat(message).toFixed(2);

        tabKWh.push(KWh);

        tabValKWh[0] = KWh;
        tabValKWh[1] = Math.min.apply(Math, tabKWh).toFixed(2);
        tabValKWh[2] = Math.max.apply(Math, tabKWh).toFixed(2);
        tabValKWh[3] = calculateAverage(tabKWh).toFixed(2);

        io.sockets.emit('KWh', tabValKWh);
    }
    if (topic == "RAM/powermeter/etats/FP") {
        Fp = parseFloat(message).toFixed(2);

        tabFp.push(Fp);

        tabValFp[0] = Fp;
        tabValFp[1] = Math.min.apply(Math, tabFp).toFixed(2);
        tabValFp[2] = Math.max.apply(Math, tabFp).toFixed(2);
        tabValFp[3] = calculateAverage(tabFp).toFixed(2);

        io.sockets.emit('FP', tabValFp);
    }
    if (topic == "RAM/alarmes/états/ALR_CNX_POW") {
        erreur = message.toString();
        if (erreur == "ON") {
            console.log(erreur);
            let date = new Date();
            let tempsAlarme = date.getFullYear() + "-" + (date.getMonth() + 1) + "-" + date.getDay() + " " + date.getHours() + ":" + date.getMinutes() + ":" + date.getSeconds();
            var querystring = 'INSERT INTO journal (Type, UserLogin, ReqTime, Info) VALUES("LOG_ALARME", " ", "' + tempsAlarme + '", "Erreur communication powerMeter ON")';

            var query = connection.query(querystring, function (err, rows, fields) {
                io.sockets.emit('erreurPowerMeter');
            });
        }
    }
    if (topic == "RAM/alarmes/états/ALR_GB_OVF") {
        erreur = message.toString();
        if (erreur == "ON") {
            console.log(erreur);
            let date = new Date();
            let tempsAlarme = date.getFullYear() + "-" + (date.getMonth() + 1) + "-" + date.getDay() + " " + date.getHours() + ":" + date.getMinutes() + ":" + date.getSeconds();
            var querystring = 'INSERT INTO journal (Type, UserLogin, ReqTime, Info) VALUES("LOG_ALARME", " ", "' + tempsAlarme + '", "Alarme de debordement GB ON ' + nivGB.toString() + '")';

            var query = connection.query(querystring, function (err, rows, fields) {
                io.sockets.emit('erreurALR_GB_OVF');
            });
        }
    }
    if (topic == "RAM/alarmes/états/ALR_PB_OVF") {
        erreur = message.toString();
        if (erreur == "ON") {
            console.log(erreur);
            let date = new Date();
            let tempsAlarme = date.getFullYear() + "-" + (date.getMonth() + 1) + "-" + date.getDay() + " " + date.getHours() + ":" + date.getMinutes() + ":" + date.getSeconds();
            var querystring = 'INSERT INTO journal (Type, UserLogin, ReqTime, Info) VALUES("LOG_ALARME", " ", "' + tempsAlarme + '", "Alarme de debordement PB ON ' + nivPB.toString() + '")';

            var query = connection.query(querystring, function (err, rows, fields) {
                io.sockets.emit('erreurALR_PB_OVF');
            });
        }
    }
    if (topic == "RAM/alarmes/états/ALR_GB_NIV_MAX") {
        erreur = message.toString();
        if (erreur == "ON") {
            console.log(erreur);
            let date = new Date();
            let tempsAlarme = date.getFullYear() + "-" + (date.getMonth() + 1) + "-" + date.getDay() + " " + date.getHours() + ":" + date.getMinutes() + ":" + date.getSeconds();
            var querystring = 'INSERT INTO journal (Type, UserLogin, ReqTime, Info) VALUES("LOG_ALARME", " ", "' + tempsAlarme + '", "Alarme niveau max GB ON ' + nivGB.toString() + '")';

            var query = connection.query(querystring, function (err, rows, fields) {
                io.sockets.emit('erreurALR_GB_NIV_MAX');
            });
        }
    }
    if (topic == "RAM/alarmes/états/ALR_PB_NIV_MAX") {
        erreur = message.toString();
        if (erreur == "ON") {
            console.log(erreur);
            let date = new Date();
            let tempsAlarme = date.getFullYear() + "-" + (date.getMonth() + 1) + "-" + date.getDay() + " " + date.getHours() + ":" + date.getMinutes() + ":" + date.getSeconds();
            var querystring = 'INSERT INTO journal (Type, UserLogin, ReqTime, Info) VALUES("LOG_ALARME", " ", "' + tempsAlarme + '", "Alarme niveau max PB ON ' + nivPB.toString() + '")';

            var query = connection.query(querystring, function (err, rows, fields) {
                io.sockets.emit('erreurALR_PB_NIV_MAX');
            });
        }
    }


    if (topic == "RAM/panneau/etats/NivGB") {
        nivGB = parseInt(message);
        io.sockets.emit('NivGB', nivGB);
    }
    if (topic == "RAM/panneau/etats/NivPB") {
        nivPB = parseInt(message);
        io.sockets.emit('NivPB', nivPB);
    }
    if (topic == "RAM/panneau/etats/TmpPB") {
        tmpPB = parseInt(message);
        io.sockets.emit('TmpPB', tmpPB);
    }
    if (topic == "RAM/panneau/etats/ValveGB") {
        valveGB = parseInt(message);
        io.sockets.emit('ValveGB', valveGB);
    }
    if (topic == "RAM/panneau/etats/ValvePB") {
        valvePB = parseInt(message);
        io.sockets.emit('ValvePB', valvePB);
    }
    if (topic == "RAM/panneau/etats/ValveEC") {
        valveEC = parseInt(message);
        io.sockets.emit('ValveEC', valveEC);
    }
    if (topic == "RAM/panneau/etats/ValveEF") {
        valveEF = parseInt(message);
        io.sockets.emit('ValveEF', valveEF);
    }
    if (topic == "RAM/panneau/etats/ValveEEC") {
        valveEEC = message.toString();
        io.sockets.emit('ValveEEC', valveEEC);
    }
    if (topic == "RAM/panneau/etats/ValveEEF") {
        valveEEF = message.toString();
        io.sockets.emit('ValveEEF', valveEEF);
    }
    if (topic == "RAM/panneau/etats/Pompe") {
        pompe = message.toString();
        io.sockets.emit('Pompe', pompe);
    }
});


io.sockets.on('connection', function (socket) {
    socket.on('logoutUserPanneau', function (etat) {
        demandeLogout = etat;
        console.log(etat);
    });
});


module.exports = router;