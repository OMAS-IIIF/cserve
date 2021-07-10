import pytest
import os


def test_ping(manager):
    data = manager.get_text('ping')
    assert data == 'PONG'

def test_get_html(manager):
    with open('./testserver/docroot/test.html') as inf:
        str = inf.read()

    resource = manager.get_text('/test.html')
    assert resource == str

def test_get_not_found(manager):
    with pytest.raises(Exception) as e_info:
        resource = manager.get_text('/gaga.html')

def test_sget_html(manager):
    with open('./testserver/docroot/test.html') as inf:
        str = inf.read()

    resource = manager.sget_text('/test.html')
    assert resource == str

def test_get_csv(manager):
    with open('./testserver/docroot/test.csv') as inf:
        str = inf.read()

    resource = manager.get_text('/test.csv')
    assert resource == str

def test_sget_csv(manager):
    with open('./testserver/docroot/test.csv') as inf:
        str = inf.read()

    resource = manager.sget_text('/test.csv')
    assert resource == str

def test_get_range(manager):
    range = manager.get_text('/range.dat', headers={"Range": "bytes=5-14"})
    assert len(range) == 10
    assert range == '456789B012'

def test_elua(manager):
    str = r"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>LuaTest</title>
</head>
<body>
Hello from Lua!
</body>
</html>"""
    resource = manager.get_text('/luatest.elua')
    assert resource == str

def test_servervariables(manager):
    variables = manager.get_json('servervariables', params={'param': 'all'})
    assert variables.get('status') == 'OK'
    assert variables.get('config') is not None
    assert variables['config'].get('port') == 8080
    assert variables['config'].get('scriptdir') == './testserver/scripts'
    assert variables['config'].get('docroot') == './testserver/docroot'
    assert variables['config'].get('keep_alive') == 5
    assert variables['config'].get('nthreads') == 4
    assert variables['config'].get('max_post_size') == '1.0MB'

    assert variables.get('server') is not None
    assert variables['server'].get('method') == 'GET'
    assert variables['server'].get('host') == 'localhost:8080'
    assert variables['server'].get('client_ip') == '127.0.0.1'
    assert variables['server'].get('client_port')
    assert variables['server'].get('uri') == '/servervariables'
    assert variables['server'].get('secure') == False
    assert variables['server'].get('get') is not None
    assert variables['server']['get'].get('param') == 'all'
    assert variables['server'].get('has_openssl') is not None
    if variables['server']['has_openssl']:
        assert variables['config'].get('ssl_port') == 8443
    assert variables['server'].get('header') is not None
    assert variables['server']['header'].get('accept') == '*/*'
    assert variables['server']['header'].get('connection') == 'keep-alive'
    assert variables['server']['header'].get('host') == 'localhost:8080'


def test_servervariables_secure(manager):
    variables = manager.sget_json('servervariables')
    assert variables.get('status') == 'OK'
    assert variables['server'].get('secure') is True

def test_cookies(manager):
    variables = manager.get_json('servervariables', cookies={"keks": "abcdefg"})
    assert variables.get('status') == 'OK'
    assert variables.get('server')
    assert variables['server'].get('cookies')
    assert variables['server']['cookies'].get('keks') == 'abcdefg'
