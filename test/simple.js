var tap = require('tap');
var test = tap.test;
var plan = tap.plan;
var slp = require('..');


test('load module', function (t) {
  t.ok(slp, 'node OpenSLP loaded');
  t.ok(slp.version, 'version');
  t.ok(slp.refreshInterval >= 0, 'refreshInterval');
  t.equal(slp.escape('foo=bar'), 'foo\\3Dbar', 'escape');
  t.equal(slp.unescape('foo\\3Dbar'), 'foo=bar', 'unescape');
  t.equal(slp.property('foobar'), null, 'foobar property exists?!');
  t.end();
});

test('find scopes', function (t) {
  var scopes = slp.findScopes();
  t.ok(scopes.indexOf('default') >= 0, 'default scope');
  t.end();
});

var service_addr = 'service:myservice.myorg://127.0.0.1:3306';
var service_name = 'myservice.myorg';

test('register service', function (t) {
  slp.reg(service_addr, slp.MAX_LIFETIME, '', function (err) {
    t.equal(err, null, 'error');
    t.end();
  });
});

test('find servers', function (t) {
  slp.findSrvs(service_name, null, null, function (err, servers) {
    console.error(servers);
    t.equal(err, null, 'error');
    t.end();
  });
});

test('deregister service', function (t) {
  slp.dereg(service_addr, function (err) {
    t.equal(err, null, 'error');
    t.end();
  });
});