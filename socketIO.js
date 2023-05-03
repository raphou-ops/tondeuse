var socket_io = require('socket.io');
var io = socket_io();
var socketIO = {};

socketIO.io = io;

io.on('connection', function (socket) {
})

module.exports = socketIO;