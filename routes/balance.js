var express = require('express');
var router = express.Router();
var mysql = require('mysql');
var { io } = require('../socketIO');
var mqtt = require('mqtt');

var erreur;
var unite;
var tare;
var poids;

var statusRecette;

var statusAspirateur;
var nivA = 0;
var nivB = 0;
var nivC = 0;

var demandeLogout = false;

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
        client.subscribe('RAM/balance/etats/poids');
        client.subscribe('RAM/balance/etats/tare');
        client.subscribe('RAM/balance/etats/unite');

        client.subscribe('RAM/melangeur/etats/recetteStatut');

        client.subscribe('RAM/shopvac/etats/sequence');
        client.subscribe('RAM/shopvac/etats/NivA');
        client.subscribe('RAM/shopvac/etats/NivB');
        client.subscribe('RAM/shopvac/etats/NivC');

        client.unsubscribe('RAM/powermeter/etats/Van');
        client.unsubscribe('RAM/powermeter/etats/Vbn');
        client.unsubscribe('RAM/powermeter/etats/Vab');
        client.unsubscribe('RAM/powermeter/etats/Ia');
        client.unsubscribe('RAM/powermeter/etats/Ib');
        client.unsubscribe('RAM/powermeter/etats/KW');
        client.unsubscribe('RAM/powermeter/etats/KWh');
        client.unsubscribe('RAM/powermeter/etats/FP');

        client.unsubscribe('RAM/panneau/etats/NivGB');
        client.unsubscribe('RAM/panneau/etats/NivPB');
        client.unsubscribe('RAM/panneau/etats/TmpPB');
        client.unsubscribe('RAM/panneau/etats/ValveGB');
        client.unsubscribe('RAM/panneau/etats/ValvePB');
        client.unsubscribe('RAM/panneau/etats/ValveEC');
        client.unsubscribe('RAM/panneau/etats/ValveEF');
        client.unsubscribe('RAM/panneau/etats/ValveEEC');
        client.unsubscribe('RAM/panneau/etats/ValveEEF');
        client.unsubscribe('RAM/panneau/etats/Pompe');

        res.render('pages/balance', { title: 'Balance', utilisateur: req.session.user.login, password: req.session.user.password, droit: req.session.user.droit, erreur, unite, tare, poids, statusRecette, statusAspirateur, nivA, nivB, nivC });
        console.log(req.session.user.login);
        console.log(req.session.user.password);
    }


});

client.on('message', function (topic, message) {

    if (topic == "RAM/balance/etats/poids") {
        poids = parseFloat(message).toFixed(2);
        io.sockets.emit('poids', poids);
    }
    if (topic == "RAM/balance/etats/tare") {
        tare = parseFloat(message).toFixed(2);
        io.sockets.emit('tare', tare);
    }
    if (topic == "RAM/balance/etats/unite") {
        unite = message.toString();
        io.sockets.emit('unite', unite);
    }
    if (topic == "RAM/alarmes/états/ALR_CNX_BAL") {
        erreur = message.toString();
        if (erreur == "ON") {
            console.log(erreur);
            let date = new Date();
            let tempsAlarme = date.getFullYear() + "-" + (date.getMonth() + 1) + "-" + date.getDay() + " " + date.getHours() + ":" + date.getMinutes() + ":" + date.getSeconds();
            var querystring = 'INSERT INTO journal (Type, UserLogin, ReqTime, Info) VALUES("LOG_ALARME", " ", "' + tempsAlarme + '", "Erreur communication balance ON")';

            var query = connection.query(querystring, function (err, rows, fields) {
                io.sockets.emit('erreurBalance');
            });
        }
    }

    if (topic == "RAM/alarmes/états/ALR_CNX_MEL") {
        erreur = message.toString();
        if (erreur == "ON") {
            console.log(erreur);
            let date = new Date();
            let tempsAlarme = date.getFullYear() + "-" + (date.getMonth() + 1) + "-" + date.getDay() + " " + date.getHours() + ":" + date.getMinutes() + ":" + date.getSeconds();
            var querystring = 'INSERT INTO journal (Type, UserLogin, ReqTime, Info) VALUES("LOG_ALARME", " ", "' + tempsAlarme + '", "Erreur communication melangeur ON")';

            var query = connection.query(querystring, function (err, rows, fields) {
                io.sockets.emit('erreurMelangeur');
            });
        }
    }

    if (topic == "RAM/alarmes/états/ALR_CNX_ASP") {
        erreur = message.toString();
        if (erreur == "ON") {
            console.log(erreur);
            let date = new Date();
            let tempsAlarme = date.getFullYear() + "-" + (date.getMonth() + 1) + "-" + date.getDay() + " " + date.getHours() + ":" + date.getMinutes() + ":" + date.getSeconds();
            var querystring = 'INSERT INTO journal (Type, UserLogin, ReqTime, Info) VALUES("LOG_ALARME", " ", "' + tempsAlarme + '", "Erreur communication aspirateur ON")';

            var query = connection.query(querystring, function (err, rows, fields) {
                io.sockets.emit('erreurAspirateur');
            });
        }
    }


    if (topic == "RAM/melangeur/etats/recetteStatut") {
        statusRecette = message.toString();
        io.sockets.emit('etatRecette', statusRecette);
    }
    if (topic == "RAM/shopvac/etats/sequence") {
        statusAspirateur = message.toString();
        io.sockets.emit('etatAspirateur', statusAspirateur);
    }
    if (topic == "RAM/shopvac/etats/NivA") {
        nivA = parseInt(message);
        io.sockets.emit('ingredientA', nivA);
    }
    if (topic == "RAM/shopvac/etats/NivB") {
        nivB = parseInt(message);
        io.sockets.emit('ingredientB', nivB);
    }
    if (topic == "RAM/shopvac/etats/NivC") {
        nivC = parseInt(message);
        io.sockets.emit('ingredientC', nivC);
    }
});

io.sockets.on('connection', function (socket) {
    socket.on('logoutUserBal', function (etat) {
        demandeLogout = etat;
        console.log(etat);
    });
});

module.exports = router;