import pytest
import os

def test_ping(manager):
    """
    Test the ping route (which is programmed in C++)
    :param manager: defined in conftest.py
    :return: None
    """
    response = manager.get('ping')
    assert response.text == 'PONG'
    assert response.headers['Content-Length'] ==  '4'


def test_get_html(manager):
    """
    Test getting an ordinary HTML file

    :param manager: defined in conftest.py
    :return: None
    """
    with open('./testserver/docroot/test.html') as inf:
        str = inf.read()

    response = manager.get('/test.html')
    assert response.text == str
    assert response.headers['Content-Type'] ==  'text/html; charset=utf-8'

def test_get_not_found(manager):
    """
    Testing for requesting non-existing file
    :param manager: defined in conftest.py
    :return: None
    """
    with pytest.raises(Exception) as e_info:
        resource = manager.get('/gaga.html')

def test_sget_html(manager):
    """
    testing https access
    :param manager: defined in conftest.py
    :return: None
    """
    with open('./testserver/docroot/test.html') as inf:
        str = inf.read()

    response = manager.sget('/test.html')
    assert response.text == str
    assert response.headers['Content-Type'] ==  'text/html; charset=utf-8'

def test_get_csv(manager):
    """
    Testing access to CSV file
    :param manager: defined in conftest.py
    :return: None
    """
    with open('./testserver/docroot/test.csv') as inf:
        str = inf.read()

    response = manager.get('/test.csv')
    assert response.text == str
    assert response.headers['Content-Type'] == 'text/csv'

def test_sget_csv(manager):
    """
    Getting CSV file with https
    :param manager: defined in conftest.py
    :return: None
    """
    with open('./testserver/docroot/test.csv') as inf:
        str = inf.read()

    response = manager.sget('/test.csv')
    assert response.text == str
    assert response.headers['Content-Type'] == 'text/csv'

def test_get_range(manager):
    """
    Testing the range header to get part of a file
    :param manager: defined in conftest.py
    :return: defined in conftest.py
    """
    response = manager.get('/range.dat', headers={"Range": "bytes=5-14"})
    assert len(response.text) == 10
    assert response.text == '456789B012'
    assert response.headers['Content-Type'] == 'text/plain'

def test_elua(manager):
    """
    Testing proper elua processing (embedded Lua)
    :param manager: defined in conftest.py
    :return: None
    """
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
    response = manager.get('/luatest.elua')
    assert response.text == str
    assert response.headers['Content-Type'] ==  'text/html; charset=utf-8'


def test_servervariables(manager):
    """
    Testing all config and server variables provided from the server to Lua
    :param manager: defined in conftest.py
    :return: None
    """
    variables = manager.get_json('servervariables', params={'param': 'all'})
    assert variables.get('status') == 'OK'
    assert variables.get('config') is not None
    assert variables['config'].get('port') == 8080
    assert variables['config'].get('scriptdir') == './testserver/scripts'
    assert variables['config'].get('docroot') == './testserver/docroot'
    assert variables['config'].get('keep_alive') == 5
    assert variables['config'].get('nthreads') == 4
    assert variables['config'].get('max_post_size') == '1MB'

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
