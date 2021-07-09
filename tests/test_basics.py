import pytest
import os
from pprint import pprint


def test_ping(manager):
    data = manager.get_text('ping')
    assert data == 'PONG'


def test_servervariables(manager):
    variables = manager.get_json('servervariables', {'param': 'all'})
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

def test_servervariables_secure(manager):
    variables = manager.get_json('servervariables', use_ssl=True)
    assert variables.get('status') == 'OK'
    assert variables['server'].get('secure') is True

def test_cookies(manager):
    variables = manager.get_json('servervariables', cookies={"keks": "abcdefg"})
    assert variables.get('status') == 'OK'
    assert variables.get('server')
    assert variables['server'].get('cookies')
    assert variables['server']['cookies'].get('keks') == 'abcdefg'
