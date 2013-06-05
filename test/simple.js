var tap = require('tap');
var test = tap.test;
var plan = tap.plan;
var nodeSLP;


test('load module', function (t) {
  nodeSLP = require('..');
  t.ok(nodeSLP.OpenSLP, 'OpenSLP class');
  t.ok(nodeSLP.version, 'version');
  t.ok(nodeSLP.refreshInterval >= 0, 'refreshInterval');
  t.equal(nodeSLP.escape('foo=bar'), 'foo\\3Dbar', 'escape');
  t.equal(nodeSLP.unescape('foo\\3Dbar'), 'foo=bar', 'unescape');
  t.equal(nodeSLP.property('foobar'), null, 'foobar property exists?!');

  /*var parsed = nodeSLP.parseSrvUrl('service:bb:session/foo:1222');
  console.error(parsed);
  t.ok(parsed);*/

  t.end();
});

test('create obj', function (t) {
  var slp = new nodeSLP.OpenSLP();
  t.ok(slp, 'new openslp');
  t.equal(slp.lastError, 0, 'openslp error');
  var scopes = slp.findScopes();
  t.ok(scopes.indexOf('default') >= 0, 'default scope');
  t.end();
  // slp.findSrvs('service:bb', '', '', function (err, services) {
  //   console.error('err', err);
  //   console.error('services', services);
  //   t.ok(services);
  //   t.end();
  // });
});
