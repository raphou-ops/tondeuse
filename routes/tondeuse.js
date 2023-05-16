var express = require('express');
var router = express.Router();
var mysql = require('mysql');
var { io } = require('../socketIO');
var mqtt = require('mqtt');

var ackTondeuse = false;

var demandeLogout = false;

var etatEnvoiBluetooth = true;

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
    res.render('pages/tondeuse', { title: 'Dashboard', utilisateur: req.session.user.login, password: req.session.user.password, droit: req.session.user.droit });
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
    var msg = "<" + joystickGaucheX.toFixed(0).toString() + ";" + joystickGaucheY.toFixed(0).toString() + ";" + boutonX + ";" + boutonO + ";" + boutonTriangle + ">";
    //port.write(msg); decommenter cette ligne pour la communication avec la manette. (ne pas paniquer)
    io.sockets.emit('refreshCommande', msg);
  });
  socket.on('cancelBluetoothProg', function () {
    etatEnvoiBluetooth = false;
  });
  socket.on('progBluetooth', function (msg, longueur, state) {

    etatEnvoiBluetooth = state;
    var i = 1, howManyTimes = 10;

    function f() {
      if (etatEnvoiBluetooth) {
        if (ackTondeuse == true) {
          i++;
          ackTondeuse = false;
        }
        var str = "<" + msg[i] + ">";
        port.write(str);
        console.log(str);

        if (i < longueur - 1) {
          setTimeout(f, 100);
        }
        else {
          io.sockets.emit('finishedSendingData');
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
          return console.log('Error opening port: ', err.message)
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
          return console.log('Error closing port: ', err.message)
        }
        else {
          reponse = etat;
          io.sockets.emit('enableBouton', reponse);
        }
      })
    }
  });
});

var etat = 1;
var tabData = [];

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

port.on('error', function (err) {
  console.log('Error: ', err.message)
});

port.on('open', function () {
  console.log("COM 17 ouvert");
});

port.on('close', function () {
  console.log("COM 17 fermé");
});

port.pipe(parser);
parser.on('data', function (data) {

  var value = data;

  switch (etat) {
    case 1:
      if (value == "S") {
        index = 0;
        etat = 2;
        tabData = [];
      }
      else {
        etat = 1
      }
      break;

    case 2:
      // index++;
      // if (index < 2) {
      //   tabData.push(value);
      // }
      // else {
      //   console.log('Data:', tabData);
      //   etat = 1;
      // }
      if (value == "ack") {
        ackTondeuse = true;
        console.log(ackTondeuse);
      }
      break;

    default:
      // code block
      break;
  }
});
module.exports = router;