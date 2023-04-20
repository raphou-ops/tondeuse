var express = require('express');
var router = express.Router();
var mysql = require('mysql');
var { io } = require('../socketIO');
var mqtt = require('mqtt');

var demandeLogout = false;

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
    res.render('pages/tondeuse', { title: 'Paramètres', utilisateur: req.session.user.login, password: req.session.user.password, droit: req.session.user.droit });
    console.log(req.session.user.login);
    console.log(req.session.user.password);
  }
});

io.sockets.on('connection', function (socket) {
  socket.on('logoutUserBal', function (etat) {
    demandeLogout = etat;
    console.log(etat);
  });
  socket.on('Bluetooth', function (joystickGaucheX, joystickGaucheY, boutonX, boutonO, boutonTriangle) {
    var msg = "<" + joystickGaucheX.toFixed(0).toString() + ";" + joystickGaucheY.toFixed(0).toString() + ";" + boutonX + ";" + boutonO + ";" + boutonTriangle + ">";
    console.log(msg);
    port.write(msg);
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
  baudRate: 9600,
  dataBits: 8,
  parity: 'none',
  stopBits: 1,
  autoOpen: true,
});

port.on('error', function (err) {
  console.log('Error: ', err.message)
});

port.on('open', function () {
  console.log("COM 17 ouvert");
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
      index++;
      if (index < 7) {
        tabData.push(value);
      }
      else {
        console.log('Data:', tabData);
        etat = 1;
      }
      break;

    default:
      // code block
      break;
  }
});
module.exports = router;