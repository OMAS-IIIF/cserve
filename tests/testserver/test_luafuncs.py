import pytest

def test_fs_funcs(manager):
    response = manager.get_json('filefunctions')
    # {'fs': {'cwd': '/Users/rosenth/ProgDev/cserve/tests'}, 'status': 'OK'}
    assert response.get('status') == 'OK'
    assert response.get('fs')
    assert response['fs'].get('cwd')
    assert response['fs'].get('ftype1') == 'CHARDEV'
    assert response['fs'].get('ftype2') == 'DIRECTORY'
    assert response['fs'].get('modtime')
    assert response['fs'].get('dirlist')
    assert 'certificate' in response['fs']['dirlist']
    assert 'scripts' in response['fs']['dirlist']
    assert 'docroot' in response['fs']['dirlist']
    print(response)
