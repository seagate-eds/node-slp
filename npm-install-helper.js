var os = require('os').platform();
var spawn = require('child_process').spawn;

cmd = process.env["npm_package_scripts_install_" + os];
console.log("os is " + os + ", bash cmd = " + cmd);
spawn('bash', ['-c', cmd], {stdio:'inherit'}).on("exit", code => process.exit(code));
