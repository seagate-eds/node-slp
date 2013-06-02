var events = require('events');
var util = require('util');

var OpenSLP = require('./build/Release/slp').OpenSLP;
util.inherits(OpenSLP, events.EventEmitter);

module.exports = OpenSLP;
