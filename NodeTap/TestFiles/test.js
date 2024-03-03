// JavaScript source code
var addon = require('bindings')('./nodetap');
console.log(addon.hello()); // 'world'

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