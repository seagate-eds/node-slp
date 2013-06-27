var tap = require('tap');
var test = tap.test;
var plan = tap.plan;
var slp = require('..');

var service = {
  type: 'service:myservice',
  host: '127.0.0.1',
  port: 3306,
  addr: function() { return this.type + '://' + this.host + ':' + this.port; }
};

test('load module', function (t) {
  t.ok(slp, 'node OpenSLP loaded');
  t.ok(slp.version, 'version');
  t.ok(slp.refreshInterval >= 0, 'refreshInterval');
  t.equal(slp.escape('foo=bar'), 'foo\\3Dbar', 'escape');
  t.equal(slp.unescape('foo\\3Dbar'), 'foo=bar', 'unescape');
  t.equal(slp.property('foobar'), null, 'foobar property exists?!');
  var parsed = slp.parseSrvUrl(service.addr());
  t.equal(parsed.type, service.type, 'service type');
  t.equal(parsed.host, service.host, 'service host');
  t.equal(parsed.port, service.port, 'service port');
  t.end();
});

test('find scopes', function (t) {
  var scopes = slp.findScopes().split(',');
  t.ok(scopes.length > 0, 'no scope found');
  t.ok(scopes.indexOf('DEFAULT') >= 0, 'default scope');
  t.end();
});

test('register service', function (t) {
  slp.reg(service.addr(), slp.MAX_LIFETIME, '(attr1=val1),(attr2=val2)', function (err) {
    t.equal(err, null, 'error');
    t.end();
  });
});

test('find service types', function (t) {
  slp.findSrvTypes('*', 'DEFAULT', function (err, srvTypes) {
    t.equal(err, null, 'error');
    t.end();
  });
});

test('find services', function (t) {
  slp.findSrvs(service.type, '', '', function (err, srvs) {
    t.equal(err, null, 'error');
    t.end();
  });
});

test('find attributes', function (t) {
  slp.findAttrs(service.type, '', '', function (err, attrs) {
    t.equal(err, null, 'error');
    t.end();
  });
});

test('deregister service', function (t) {
  slp.dereg(service.addr(), function (err) {
    t.equal(err, null, 'error');
    t.end();
  });
});
