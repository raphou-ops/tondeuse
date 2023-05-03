var express = require('express');
var router = express.Router();
var mysql = require('mysql');
var { io } = require('../socketIO');
var session = require('cookie-session');
var mqtt = require('mqtt');
var login = "";
var droit = 0;

var client = mqtt.connect('mqtt://127.0.0.1:1883');

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
        // req.session.user.id = 4;
        // req.session.user.login = "invité";
        // req.session.user.droit = 0;
        // req.session.user.password = "burger";
        // droit = req.session.user.droit;
        // res.render('pages/utilisateur', { texte: "Bonjour invité", utilisateur: req.session.user.login, droit, password: req.session.user.password });
        res.render('pages/erreurLogin', { utilisateur: login, password: req.query.password });
      }
      else {
        req.session.user.id = rows[0].ID;
        req.session.user.login = rows[0].login;
        req.session.user.droit = rows[0].niveauDroit;
        req.session.user.password = rows[0].password;
        droit = req.session.user.droit;
        res.render('pages/utilisateur', { texte: rows[0].texteAccueil, utilisateur: req.session.user.login, droit, password: rows[0].password });
        console.log('Un client est connecté!');
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


