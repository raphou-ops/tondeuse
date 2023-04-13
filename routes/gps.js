var express = require('express');
var router = express.Router();
var mysql = require('mysql');
var { io } = require('../socketIO');
var mqtt = require('mqtt');

var demandeLogout = false;


// function calculateAverage(tabDonnes) {
//     var moyenne = 0;
//     for (i = 0; i < tabDonnes.length; i++) {
//         moyenne += parseFloat(tabDonnes[i]);
//     }
//     moyenne = moyenne / tabDonnes.length;
//     return moyenne;
// }

var client = mqtt.connect('mqtt://127.0.0.1:1883');

var connection = mysql.createConnection({
    host: 'localhost',
    user: 'root',
    password: '12345',
    database: 'users'
});

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
        res.render('pages/gps', { title: 'Mapping', utilisateur: req.session.user.login, password: req.session.user.password, droit: req.session.user.droit });
        console.log(req.session.user.login);
        console.log(req.session.user.password);
    }
});

io.sockets.on('connection', function (socket) {
    socket.on('logoutUserPanneau', function (etat) {
        demandeLogout = etat;
        console.log(etat);
    });
});
module.exports = router;