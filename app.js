var createError = require('http-errors');
var express = require('express');
var path = require('path');
var cookieParser = require('cookie-parser');
var logger = require('morgan');
var etat = 1;
var tabData = [];

// var indexRouter = require('./routes/index');
// var usersRouter = require('./routes/users');
var indexRouter = require('./routes/accueil');
var usersRouter = require('./routes/users');
var balanceRouter = require('./routes/balance');
var powerMeter = require('./routes/powerMeter');

var app = express();

// view engine setup
app.set('views', path.join(__dirname, 'views'));
app.set('view engine', 'ejs');

app.use(logger('dev'));
app.use(express.json());
app.use(express.urlencoded({ extended: false }));
app.use(cookieParser());
app.use(express.static(path.join(__dirname, 'public')));

app.use('/', indexRouter);
app.use('/users', usersRouter);
app.use('/balance', balanceRouter);
app.use('/powerMeter', powerMeter);

// catch 404 and forward to error handler
app.use(function (req, res, next) {
  next(createError(404));
});

// error handler
app.use(function (err, req, res, next) {
  // set locals, only providing error in development
  res.locals.message = err.message;
  res.locals.error = req.app.get('env') === 'development' ? err : {};

  // render the error page
  res.status(err.status || 500);
  res.render('error');
});

module.exports = app;

const { SerialPort, ReadlineParser } = require('serialport');
const { ByteLengthParser } = require('@serialport/parser-byte-length');

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

