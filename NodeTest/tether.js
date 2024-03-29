// Copyright 2011 ClockworkMod, LLC.

var fs = require('fs');
var put = require('put');
var buffers = require('buffers');
var os = require('os');
var rl = require('readline');
var cluster = require('cluster');

var adb;

var tun = require('./tuntap');

var myWorker;
var t;

function createTun(withWorker) {
    console.log('Opening tun device.');
    if (!withWorker || cluster.isMaster) {
        try {
            t = new tun.tun(null, '10.0.0.1', { noCatcher: withWorker });
        }
        catch (e) {
            console.log('unable to open tun/tap device.');
            console.log(e);
            process.exit();
        }

        if (withWorker) {
            function forker() {
                t.setCatcherWorker(myWorker = cluster.fork());
            }
            if (os.platform() == 'win32') {
                console.log('Waiting for interface to get ready... (forker, waiting 5 seconds)');
                setTimeout(forker, 5000);
            }
            else {
                forker();
            }
        }

        t.on('ready', function () {
            console.log('STATUS: Tether interface is ready.');
        });
    }
    else {
        t = tun.startMasterTunWorker('10.0.0.1');
    }

    if (!withWorker || !cluster.isMaster) {
        adb = require('./adb');
        var interfaces;
        if (process.env.USE_INTERFACES)
            interfaces = require('./interfaces');
        function getRelay() {
            var allRelays = adb.getRelays();
            if (interfaces)
                allRelays = allRelays.concat(interfaces.getRelays());
            if (allRelays.length == 0)
                return null;
            var rand = Math.round(Math.random() * (allRelays.length - 1));
            return allRelays[rand];
        }
    }

    var pendingConnections = {};

    var relayCount = 0;

    t.on('tcp-outgoing', function (connection) {
        var existing = pendingConnections[connection.sourcePort];
        // console.log('pending: ' + Object.keys(pendingConnections).length)
        if (existing)
            return;

        var relayConnection = getRelay();
        if (!relayConnection)
            return;

        connection.ready = function () {
            delete pendingConnections[connection.sourcePort];
            connection.accept();
        }
        connection.close = function () {
            // console.log('conn closed');
            delete pendingConnections[connection.sourcePort];
            if (connection.connected) {
                // console.log('tuntap.js relay count: ' + relayCount);
                connection.connected = false;
                relayCount--;
            }
            try {
                if (connection.socket) {
                    connection.socket.destroy();
                }
            }
            catch (e) {
            }
        }
        connection.write = function (data) {
            if (!connection.socket) {
                if (!connection.pending)
                    connection.pending = new buffers();
                connection.pending.push(data);
                return;
            }
            connection.socket.write(data);
        }
        connection.destroy = function () {
            connection.close();
            try {
                if (connection.socket) {
                    connection.socket.destroy();
                }
            }
            catch (e) {
            }
        }

        relayConnection.makeConnection(connection);
        connection.relayConnection = relayConnection;

        pendingConnections[connection.sourcePort] = true;
    });

    t.on('tcp-connect', function (connection) {
        relayCount++;
        connection.connected = true;
        // console.log('tuntap.js relay count: ' + relayCount);

        var relayConnection = connection.relayConnection;
        relayConnection.connectRelay(connection);
        if (connection.pending) {
            var packet = put()
                .put(connection.pending)
                .write(connection.socket);

            connection.pending = null;
        }
        connection.socket.on('data', connection.send);
    });

    t.on('udp-connect', function (connection) {
        var relayConnection = connection.relayConnection;
        if (!relayConnection)
            return;
        relayConnection.connectRelay(connection);

        var timeout;
        var duration = 10000;

        function scheduleAutocleanup() {
            if (timeout)
                clearTimeout(timeout);
            timeout = setTimeout(function () {
                try {
                    connection.close();
                }
                catch (e) {
                }
            }, duration);
        }

        scheduleAutocleanup();
        connection.socket.on('message', function (message, rinfo) {
            connection.send(message);

            // if (connection.destinationPort === 53)
            // duration = 2000;
            scheduleAutocleanup();
        });
    });

    t.on('udp-outgoing', function (connection) {
        var relayConnection = getRelay();
        if (!relayConnection)
            return;

        connection.write = function (data) {
            var socket = connection.socket;
            socket.send(data, 0, data.length, connection.sourcePort, connection.destinationString, function (err, bytes) {
                if (err)
                    console.log(err);
            });
        }
        connection.close = function () {
            if (connection.socket.handle != null) {
                connection.socket.close();
            }
        }
        connection.ready = function () {
        }
        connection.destroy = function () {
            connection.socket.close();
        }
        relayConnection.makeConnection(connection);
        connection.relayConnection = relayConnection;

        connection.accept();
    });
}

function exitTether() {
    process.exit();
    console.log('Closing tether adapter.');
    if (myWorker) {
        myWorker.destroy();
        myWorker = null;
    }
    if (adb)
        adb.closeAll();
    t.close(function () {
        console.log('Tether is exiting.');
        process.exit();
    });
}

var useWorker = true;
if (process.env.NO_TUNWORKER)
    useWorker = false;
createTun(useWorker);
if (cluster.isMaster) {
    var inputInterface = rl.createInterface(process.stdin, process.stdout);

    inputInterface.on('line', function (line) {
        line = line.trim();
        if (line == 'quit') {
            console.log('Quit command received.');
            exitTether();
        }
        else if (line == 'close') {
            if (t)
                t.close();
            t = null;
        }
        else if (line == 'open') {
            if (t)
                t.close();
            t = null;
            createTun(useWorker);
        }
    });

    inputInterface.on('close', function (line) {
        exitTether();
    });
}
else {
    console.log('tun worker initialized.');
}