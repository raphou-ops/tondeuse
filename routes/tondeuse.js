/**
* @file tondeuse.js
* @author Adrian Catuneanu et Raphaël Tazbaz
* @brief Ce fichier javascript gere la connection bluetooth du serveur nodeJs avec le robot tondeuse. De plus, elle gere l'envoi de coordonées vers la tondeuse et l'envoi de commandes à partir de la mannette ps4 à l'aide de sockets.
* @version 1.4
* @date 2023-05-15
*
* @copyright Copyright (c) 2023
*/

var express = require('express');
var router = express.Router();
var mysql = require('mysql');
var { io } = require('../socketIO');
var mqtt = require('mqtt');

var ackTondeuse = false;
var demandeLogout = false;
var etatEnvoiBluetooth = true;
var value = 0;
var etatReception = 1;
var tabData = [];
var modeTondeuse = 0;
var pourcentageBatterie = 98.72;
var flagPluie = 0;
var etatCoupe = 0;
var flagLiDAR = 0;
var latitudeTondeuse = 45.64443;
var longitudeTondeuse = -73.84283;
var etatConnectionBluetooth = 0;

var client = mqtt.connect('mqtt://127.0.0.1:1883');

var connection = mysql.createConnection({
  host: 'localhost',
  user: 'root',
  password: '12345',
  database: 'users'
});

const { SerialPort, ReadlineParser } = require('serialport');
//const parser = new ByteLengthParser({ length: 1 });
const parser = new ReadlineParser({ delimiter: ';' });
// Create a port
const port = new SerialPort({
  path: 'COM17',
  baudRate: 115200,
  dataBits: 8,
  parity: 'none',
  stopBits: 1,
  autoOpen: false,
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
    res.render('pages/tondeuse', { title: 'Dashboard', utilisateur: req.session.user.login, password: req.session.user.password, droit: req.session.user.droit, pourcentageBatterie, flagPluie, etatCoupe, flagLiDAR, latitudeTondeuse, longitudeTondeuse, modeTondeuse, etatConnectionBluetooth });
    // console.log(req.session.user.login);
    // console.log(req.session.user.password);
  }
});

io.sockets.on('connection', function (socket) {
  socket.on('logoutUserBal', function (etat) {
    demandeLogout = etat;
    console.log(etat);
    console.log("Un utilisateur c'est déconnecté !");
  });
  socket.on('Bluetooth', function (joystickGaucheX, joystickGaucheY, boutonX, boutonO, boutonTriangle) {
    var msg = "<" + joystickGaucheX.toFixed(0).toString() + ";" + joystickGaucheY.toFixed(0).toString() + ";" + boutonX + ";" + boutonO + ";" + boutonTriangle + ">";//changer le protocol
    if (modeTondeuse == 0)
      port.write(msg); //decommenter cette ligne pour la communication avec la manette. (ne pas paniquer)
    io.sockets.emit('refreshCommande', msg);
  });
  socket.on('cancelBluetoothProg', function () {
    etatEnvoiBluetooth = false;
  });
  socket.on('progBluetooth', function (msg, longueur, state) {

    etatEnvoiBluetooth = state;
    var i = 1;

    function f() {
      if (etatEnvoiBluetooth) {
        if (ackTondeuse == true) {
          i++;
          ackTondeuse = false;
          io.sockets.emit('refreshData', i, longueur);
        }
        var str = "<" + msg[i] + ">";
        if (modeTondeuse == 1) {
          port.write(str);
          console.log(str);
        }
        else {
          io.sockets.emit('erreurEnvoi');
          etatEnvoiBluetooth = false;
        }

        if (i < longueur - 1) {
          setTimeout(f, 200);
        }
        else {
          io.sockets.emit('finishedSendingData');
          etatReception = 1;
        }
      }
    }
    f();
  });

  socket.on('connectBluetooth', function (etat) {
    var reponse;
    if (etat) {
      port.open(function (err) {
        if (err) {
          reponse = false;
          io.sockets.emit('enableBouton', reponse);
          return console.log('Error opening port: ', err.message);
        }
        else {
          reponse = etat;
          io.sockets.emit('enableBouton', reponse);
        }
      })
    }
    else {
      port.close(function (err) {
        if (err) {
          reponse = false;
          io.sockets.emit('enableBouton', reponse);
          return console.log('Error closing port: ', err.message);
        }
        else {
          reponse = etat;
          io.sockets.emit('enableBouton', reponse);
        }
      })
    }
  });
});

port.on('error', function (err) {
  etatConnectionBluetooth = 0;
  modeTondeuse = 0;
  console.log('Error: ', err.message);
  reponse = false;
  io.sockets.emit('enableBouton', reponse);
});

port.on('open', function () {
  etatConnectionBluetooth = 1;
  console.log("COM 17 ouvert");
});

port.on('close', function () {
  etatConnectionBluetooth = 0;
  modeTondeuse = 0;
  console.log("COM 17 fermé");
});

port.pipe(parser);
parser.on('data', function (data) {

  value = data;

  switch (etatReception) {
    case 1:
      if (value == "S") {
        index = 0;
        etatReception = 2;
        tabData = [];
      }
      else if (value == "C") {
        index = 0;
        etatReception = 3;
        tabData = [];
      }
      else {
        etatReception = 1;
      }
      break;

    case 2:
      index++;
      if ((index <= 7) && (value != "S")) {
        tabData.push(value);
      }
      else {
        console.log('Data:', tabData);
        io.sockets.emit('receptionBluetooth', tabData);
        pourcentageBatterie = tabData[0];
        flagPluie = tabData[1];
        etatCoupe = tabData[2];
        flagLiDAR = tabData[3];
        latitudeTondeuse = tabData[4];
        longitudeTondeuse = tabData[5];
        modeTondeuse = tabData[6];
        etatReception = 1;
      }

      break;
    case 3:
      if (value == "ack") {
        ackTondeuse = true;
        console.log(ackTondeuse);
      }
      break;

    default:
      // rien pour le moment
      break;
  }
});
module.exports = router;