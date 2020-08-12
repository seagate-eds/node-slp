var os = require('os').platform();
var spawn = require('child_process').spawn;

cmd = process.env["npm_package_scripts_install_" + os];
console.log("os is " + os + ", bash cmd = " + cmd);
if (os == 'win32') {
    spawn('cmd.exe', ['/c', cmd], {stdio:'inherit'}).on("exit", code => process.exit(code));
} else {
    spawn('/bin/bash', ['-c', cmd], {stdio:'inherit'}).on("exit", code => process.exit(code));
}    
