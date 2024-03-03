// JavaScript source code
var addon = require('bindings')('./nodetap');

var handle = addon.openTap();

addon.configDhcp(handle, "10.0.0.1", "255.255.255.0", "10.0.0.2");
addon.dhcpSetOptions(handle,
    "10.0.0.0",          // Default gateway
    "8.8.8.8", "8.8.4.4" // DNS servers
); 
addon.configTun(handle, "10.0.0.1", "10.0.0.0", "255.255.255.0");
addon.setMediaStatus(handle, true);

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