// JavaScript source code
var nodetap = require('bindings')('./nodetap');

var handle = nodetap.openTap();
nodetap.configDhcp(handle, "10.0.0.1", "255.255.255.0", "10.0.0.2");
nodetap.dhcpSetOptions(handle, "10.0.0.0", "8.8.8.8", "8.8.4.4"); // DNS servers
nodetap.configTun(handle, "10.0.0.0", "10.0.0.0", "255.255.255.0");
nodetap.setMediaStatus(handle, true);

reader = function (err, length, buffer) {
    //if (length > 0) {
        console.log('in reader callback:' + length + "," + Object.prototype.toString.call(buffer));
    //}

    //nodetap.read(handle, buffer, buffer.length, reader);
}

var buffer = Buffer.alloc(65535);
console.log('real buffer:' + Object.prototype.toString.call(buffer));
nodetap.read(handle, buffer, buffer.length, reader);

const keypress = async () => {
    process.stdin.setRawMode(true)
    return new Promise(resolve => process.stdin.once('data', data => {
        const byteArray = [...data]
        if (byteArray.length > 0 && byteArray[0] === 3) {
            console.log('^C')
            process.exit(1)
        }
        process.stdin.setRawMode(false)
        resolve()
    }))
}

(async () => {

    console.log('Press any key to exit')
    await keypress()
})().then(process.exit);