import pytest

def test_fs_funcs(manager):
    response = manager.get_json('filefunctions')
    assert response.get('status') == 'OK'
    assert response.get('fs')
    assert response['fs'].get('cwd')
    assert response['fs'].get('exists1') is True
    assert response['fs'].get('exists2') is False
    assert response['fs'].get('ftype1') == 'CHARDEV'
    assert response['fs'].get('ftype2') == 'DIRECTORY'
    assert response['fs'].get('modtime')
    assert response['fs'].get('dirlist')
    assert 'certificate' in response['fs']['dirlist']
    assert 'scripts' in response['fs']['dirlist']
    assert 'docroot' in response['fs']['dirlist']
    assert 'tmp' in response['fs']['dirlist']
    assert response['fs'].get('readable1') is True
    assert response['fs'].get('readable2') is False
    assert response['fs'].get('writeable1') is True
    assert response['fs'].get('writeable2') is False
    assert response['fs'].get('executable1') is True
    assert response['fs'].get('executable2') is False
    assert response['fs'].get('mkdir_success') == 'DIRECTORY'
    assert response['fs'].get('dir_exists') is False
    assert response['fs'].get('copyfile_exists') is True
    assert response['fs'].get('unlink_exists') is False
    assert response['fs'].get('movefile_exists') is True
    assert 'testserver/tmp' in response['fs'].get('after_chdir')




