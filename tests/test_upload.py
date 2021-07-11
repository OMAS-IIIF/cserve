import pytest

def test_upload(manager):
    response = manager.upload('upload', './tux.jpg', 'image/jpeg')
    assert response.get('status') == 'OK'
    assert response.get('mimetype') == 'image/jpeg'
    assert response.get('consistency') is True
    assert response.get('origname') == 'tux.jpg'
    assert response.get('cnt') == 1
    assert response.get('filesize') == 2078

def test_upload2(manager):
    response = manager.upload('upload', './tux.jpg', 'text/plain')
    assert response.get('status') == 'OK'
    assert response.get('mimetype') == 'text/plain'
    assert response.get('consistency') is False
    assert response.get('origname') == 'tux.jpg'
    assert response.get('cnt') == 1
    assert response.get('filesize') == 2078
