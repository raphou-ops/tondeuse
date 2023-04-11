var express = require('express');
var router = express.Router();
var mysql = require('mysql');
var { io } = require('../socketIO');
var session = require('cookie-session');
var mqtt = require('mqtt');
var login = "";
var droit = 0;

var client = mqtt.connect('mqtt://127.0.0.1:1883');


// const requestAnimationFrame = (fn: Function) => fn();
// globalThis.requestAnimationFrame = requestAnimationFrame;

client.subscribe('RAM/alarmes/états/ALR_CNX_BAL');
client.subscribe('RAM/alarmes/états/ALR_CNX_POW');
client.subscribe('RAM/alarmes/états/ALR_CNX_MEL');
client.subscribe('RAM/alarmes/états/ALR_CNX_ASP');

client.subscribe('RAM/alarmes/états/ALR_GB_OVF');
client.subscribe('RAM/alarmes/états/ALR_PB_OVF');
client.subscribe('RAM/alarmes/états/ALR_GB_NIV_MAX');
client.subscribe('RAM/alarmes/états/ALR_PB_NIV_MAX');


client.on('connect', function () {
  console.log("MQTT connecté !");
});

var connection = mysql.createConnection({
  host: 'localhost',
  user: 'root',
  password: '12345',
  database: 'users'
});

connection.connect(function (err) {
  if (err) throw err;
  console.log('Vous êtes connecté à votre BDD...');
});

router.use(session({ secret: 'todotopsecret' }));

router.use(function (req, res, next) {
  if (typeof (req.session.user) == 'undefined') {
    req.session.user = {
      id: 0,
      login: '',
      droit: 0,
      password: ''
    }
  }
  next();
})

/* GET home page. */
router.get('/', function (req, res, next) {


  client.unsubscribe('RAM/balance/etats/poids');
  client.unsubscribe('RAM/balance/etats/tare');
  client.unsubscribe('RAM/balance/etats/unite');

  client.unsubscribe('RAM/melangeur/etats/recetteStatut');

  client.unsubscribe('RAM/shopvac/etats/sequence');
  client.unsubscribe('RAM/shopvac/etats/NivA');
  client.unsubscribe('RAM/shopvac/etats/NivB');
  client.unsubscribe('RAM/shopvac/etats/NivC');

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

  login = req.query.login;

  if (login && req.query.password) {

    var querystring = 'SELECT * FROM caracuser WHERE login = "' + login + '" AND password = "' + req.query.password + '"';

    var query = connection.query(querystring, function (err, rows, fields) {
      /*
      if (!err) {
        console.log("Ma requête est passée !");
        console.log(rows);
        //console.log(fields); lignes de debuggage
      };
      */
      if (rows.length == 0) {
        //res.render('pages/erreurLogin',{utilisateur : "invité",password : "burger"});
        req.session.user.id = 4;
        req.session.user.login = "invité";
        req.session.user.droit = 0;
        req.session.user.password = "burger";
        droit = req.session.user.droit;
        res.render('pages/utilisateur', { texte: "Bonjour invité", utilisateur: req.session.user.login, droit, password: req.session.user.password });
      }
      else {
        req.session.user.id = rows[0].ID;
        req.session.user.login = rows[0].login;
        req.session.user.droit = rows[0].niveauDroit;
        req.session.user.password = rows[0].password;
        droit = req.session.user.droit;
        res.render('pages/utilisateur', { texte: rows[0].texteAccueil, utilisateur: req.session.user.login, droit, password: rows[0].password });
      }
    });
  }
  else if (req.query.pokeLogout) {
    req.session.user = {
      id: 0,
      login: '',
      droit: 0,
      password: ''
    }
    res.render('pages/accueil', { title: 'Login' });
  }
  else {
    res.render('pages/accueil', { title: 'Login' });
  }
});


io.sockets.on('connection', function (socket) {

  function createList() {
    if (droit == 3) {
      querystring = 'SELECT login,password,texteAccueil FROM caracuser';

      query = connection.query(querystring, function (err, rows, fields) {
        var listPassword = [];
        var listUser = [];
        var listTexte = [];
        for (i = 0; i < rows.length; i++) {
          listPassword[i] = rows[i].password;
          listUser[i] = rows[i].login;
          listTexte[i] = rows[i].texteAccueil;
        }
        io.sockets.emit('tableauUser', listPassword, listTexte, listUser);
      });
    }
  }

  socket.on('ackBalance', function (utilisateur) {
    let date = new Date();
    var tempsAlarme = date.getFullYear() + "-" + (date.getMonth() + 1) + "-" + date.getDay() + " " + date.getHours() + ":" + date.getMinutes() + ":" + date.getSeconds();
    querystring = 'INSERT INTO journal (Type, UserLogin, ReqTime, Info) VALUES("LOG_ALARME", "' + utilisateur + '", "' + tempsAlarme + '", "Erreur communication balance ACK")';

    query = connection.query(querystring, function (err, rows, fields) {
      client.publish('RAM/alarmes/états/ALR_CNX_BAL', '\06', { retain: true, qos: 0 });
      io.sockets.emit('hideErreurBal');
    });

  });

  socket.on('ackPowerMeter', function (utilisateur) {
    let date = new Date();
    var tempsAlarme = date.getFullYear() + "-" + (date.getMonth() + 1) + "-" + date.getDay() + " " + date.getHours() + ":" + date.getMinutes() + ":" + date.getSeconds();
    querystring = 'INSERT INTO journal (Type, UserLogin, ReqTime, Info) VALUES("LOG_ALARME", "' + utilisateur + '", "' + tempsAlarme + '", "Erreur communication powerMeter ACK")';

    query = connection.query(querystring, function (err, rows, fields) {
      client.publish('RAM/alarmes/états/ALR_CNX_POW', '\06', { retain: true, qos: 0 });
      io.sockets.emit('hideErreurMeter');
    });
  });

  socket.on('ackMelangeur', function (utilisateur) {
    let date = new Date();
    var tempsAlarme = date.getFullYear() + "-" + (date.getMonth() + 1) + "-" + date.getDay() + " " + date.getHours() + ":" + date.getMinutes() + ":" + date.getSeconds();
    querystring = 'INSERT INTO journal (Type, UserLogin, ReqTime, Info) VALUES("LOG_ALARME", "' + utilisateur + '", "' + tempsAlarme + '", "Erreur communication melangeur ACK")';

    query = connection.query(querystring, function (err, rows, fields) {
      client.publish('RAM/alarmes/états/ALR_CNX_MEL', '\06', { retain: true, qos: 0 });
      io.sockets.emit('hideErreurMel');
    });

  });

  socket.on('ackAspirateur', function (utilisateur) {
    let date = new Date();
    var tempsAlarme = date.getFullYear() + "-" + (date.getMonth() + 1) + "-" + date.getDay() + " " + date.getHours() + ":" + date.getMinutes() + ":" + date.getSeconds();
    querystring = 'INSERT INTO journal (Type, UserLogin, ReqTime, Info) VALUES("LOG_ALARME", "' + utilisateur + '", "' + tempsAlarme + '", "Erreur communication aspirateur ACK")';

    query = connection.query(querystring, function (err, rows, fields) {
      client.publish('RAM/alarmes/états/ALR_CNX_ASP', '\06', { retain: true, qos: 0 });
      io.sockets.emit('hideErreurAsp');
    });
  });



  socket.on('ackALR_GB_OVF', function (utilisateur) {
    let date = new Date();
    var tempsAlarme = date.getFullYear() + "-" + (date.getMonth() + 1) + "-" + date.getDay() + " " + date.getHours() + ":" + date.getMinutes() + ":" + date.getSeconds();
    querystring = 'INSERT INTO journal (Type, UserLogin, ReqTime, Info) VALUES("LOG_ALARME", "' + utilisateur + '", "' + tempsAlarme + '", "Alarme de debordement GB ACK")';

    query = connection.query(querystring, function (err, rows, fields) {
      client.publish('RAM/alarmes/états/ALR_GB_OVF', '\06', { retain: true, qos: 0 });
      io.sockets.emit('hideALR_GB_OVF');
    });
  });

  socket.on('ackALR_PB_OVF', function (utilisateur) {
    let date = new Date();
    var tempsAlarme = date.getFullYear() + "-" + (date.getMonth() + 1) + "-" + date.getDay() + " " + date.getHours() + ":" + date.getMinutes() + ":" + date.getSeconds();
    querystring = 'INSERT INTO journal (Type, UserLogin, ReqTime, Info) VALUES("LOG_ALARME", "' + utilisateur + '", "' + tempsAlarme + '", "Alarme de debordement PB ACK")';

    query = connection.query(querystring, function (err, rows, fields) {
      client.publish('RAM/alarmes/états/ALR_PB_OVF', '\06', { retain: true, qos: 0 });
      io.sockets.emit('hideALR_PB_OVF');
    });
  });

  socket.on('ackALR_GB_NIV_MAX', function (utilisateur) {
    let date = new Date();
    var tempsAlarme = date.getFullYear() + "-" + (date.getMonth() + 1) + "-" + date.getDay() + " " + date.getHours() + ":" + date.getMinutes() + ":" + date.getSeconds();
    querystring = 'INSERT INTO journal (Type, UserLogin, ReqTime, Info) VALUES("LOG_ALARME", "' + utilisateur + '", "' + tempsAlarme + '", "Alarme niveau max GB ACK")';

    query = connection.query(querystring, function (err, rows, fields) {
      client.publish('RAM/alarmes/états/ALR_GB_NIV_MAX', '\06', { retain: true, qos: 0 });
      io.sockets.emit('hideALR_GB_NIV_MAX');
    });
  });

  socket.on('ackALR_PB_NIV_MAX', function (utilisateur) {
    let date = new Date();
    var tempsAlarme = date.getFullYear() + "-" + (date.getMonth() + 1) + "-" + date.getDay() + " " + date.getHours() + ":" + date.getMinutes() + ":" + date.getSeconds();
    querystring = 'INSERT INTO journal (Type, UserLogin, ReqTime, Info) VALUES("LOG_ALARME", "' + utilisateur + '", "' + tempsAlarme + '", "Alarme niveau max PB ACK")';

    query = connection.query(querystring, function (err, rows, fields) {
      client.publish('RAM/alarmes/états/ALR_PB_NIV_MAX', '\06', { retain: true, qos: 0 });
      io.sockets.emit('hideALR_PB_NIV_MAX');
    });
  });



  socket.on('moteurA', function (etat) {
    client.publish('RAM/melangeur/cmd/motA', etat);
  });
  socket.on('moteurB', function (etat) {
    client.publish('RAM/melangeur/cmd/motB', etat);
  });
  socket.on('moteurC', function (etat) {
    client.publish('RAM/melangeur/cmd/motC', etat);
  });
  socket.on('addRecette', function (recette) {
    client.publish('RAM/melangeur/cmd/recette', recette);
  });
  socket.on('stopRecette', function (cmd) {
    client.publish('RAM/melangeur/cmd/recetteGo', cmd);
  });
  socket.on('flushRecette', function (cmd) {
    client.publish('RAM/melangeur/cmd/recetteGo', cmd);
  });
  socket.on('startRecette', function (cmd) {
    client.publish('RAM/melangeur/cmd/recetteGo', cmd);
  });
  socket.on('modeMelangeur', function (mode) {
    client.publish('RAM/melangeur/cmd/mode', mode);
  });
  socket.on('goAspirateur', function (cmd) {
    client.publish('RAM/shopvac/cmd/force', cmd);
  });
  socket.on('stopAspirateur', function (cmd) {
    client.publish('RAM/shopvac/cmd/force', cmd);
  });


  socket.on('modeRam', function (cmd) {
    client.publish('RAM/panneau/cmd/Mode', cmd);
  });
  socket.on('setEEC', function (cmd) {
    client.publish('RAM/panneau/cmd/ValveEEC', cmd);
  });
  socket.on('setEEF', function (cmd) {
    client.publish('RAM/panneau/cmd/ValveEEF', cmd);
  });
  socket.on('setPompe', function (cmd) {
    client.publish('RAM/panneau/cmd/Pompe', cmd);
  });
  socket.on('setValveGB', function (cmd) {
    client.publish('RAM/panneau/cmd/ValveGB', cmd);
  });
  socket.on('setValvePB', function (cmd) {
    client.publish('RAM/panneau/cmd/ValvePB', cmd);
  });
  socket.on('setValveEC', function (cmd) {
    client.publish('RAM/panneau/cmd/ValveEC', cmd);
  });
  socket.on('setValveEF', function (cmd) {
    client.publish('RAM/panneau/cmd/ValveEF', cmd);
  });
  socket.on('setConsigneNivGb', function (cmd) {
    client.publish('RAM/panneau/cmd/ConsNivGB', cmd);
  });
  socket.on('setConsigneNivPb', function (cmd) {
    client.publish('RAM/panneau/cmd/ConsNivPB', cmd);
  });
  socket.on('setConsigneTmpPb', function (cmd) {
    client.publish('RAM/panneau/cmd/ConsTmpPB', cmd);
  });

  socket.on('setNivLhGB', function (cmd) {
    client.publish('RAM/alarmes/cmd/NivLhGB', cmd);
  });
  socket.on('setTgNivGB', function (cmd) {
    client.publish('RAM/alarmes/cmd/TgNivGB', cmd);
  });
  socket.on('setTrNivGB', function (cmd) {
    client.publish('RAM/alarmes/cmd/TrNivGB', cmd);
  });

  socket.on('setNivLhPB', function (cmd) {
    client.publish('RAM/alarmes/cmd/NivLhPB', cmd);
  });
  socket.on('setTgNivPB', function (cmd) {
    client.publish('RAM/alarmes/cmd/TgNivPB', cmd);
  });
  socket.on('setTrNivPB', function (cmd) {
    client.publish('RAM/alarmes/cmd/TrNivPB', cmd);
  });




  socket.on('messageLogin', function (messageLogin, userActuels) {

    querystring = 'UPDATE caracuser SET texteAccueil = "' + messageLogin + '" WHERE login = "' + userActuels + '"';

    query = connection.query(querystring, function (err, rows, fields) {
      console.log("L'utilisateur " + userActuels.toString() + " a changé son message d'accueil !");
      createList();
    });
  });

  socket.on('messagePassword', function (messagePassword, userActuels) {

    querystring = 'UPDATE caracuser SET password = "' + messagePassword + '" WHERE login = "' + userActuels + '"';

    query = connection.query(querystring, function (err, rows, fields) {
      console.log("L'utilisateur " + userActuels.toString() + " a changé son mot de passe !");
      createList();
    });
  });

  socket.on('messageAdminLogin', function (message, userMessage) {

    querystring = 'UPDATE caracuser SET texteAccueil = "' + message + '" WHERE login = "' + userMessage + '"';

    query = connection.query(querystring, function (err, rows, fields) {
      console.log("L'admin a changé le message d'accueil de " + userMessage.toString());
      createList();
    });
  });

  socket.on('messageAdminPassword', function (password, userPassword) {

    querystring = 'UPDATE caracuser SET password = "' + password + '" WHERE login = "' + userPassword + '"';

    query = connection.query(querystring, function (err, rows, fields) {
      console.log("L'admin a changé le mot de passe de " + userPassword.toString());
      createList();
    });
  });

  socket.on('ajoutUtilisateur', function (login, password, ajoutDroit, ajoutMessage) {
    querystring = 'INSERT INTO caracuser (login, password, niveauDroit, texteAccueil) VALUES("' + login + '", "' + password + '", "' + ajoutDroit + '", "' + ajoutMessage + '")';

    query = connection.query(querystring, function (err, rows, fields) {
      console.log("L'admin a creer l'utilisateur " + login.toString());
      createList();
    });
  });

  socket.on('deleteUser', function (login) {
    querystring = 'DELETE FROM caracuser WHERE login="' + login + '"';

    query = connection.query(querystring, function (err, rows, fields) {
      console.log("L'admin a enlever l'utilisateur " + login.toString());
      createList();
    });
  });

  createList();

});
module.exports = router;


