var socket_io = require('socket.io');
var io = socket_io();
var socketIO = {};

socketIO.io = io;

io.on('connection', function (socket) {
    console.log('Un client est connecté!');
})

module.exports = socketIO;