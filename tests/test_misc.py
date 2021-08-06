import pytest
import jwt

def test_misc(manager):
    payload = {
        'key': 'Dies ist ein Testtoken'
    }
    newjwt = jwt.encode(payload, 'UP4014, the biggest steam engine', algorithm="HS256")
    response = manager.get_json('misc', params={'jwt': newjwt})

    assert response.get('status') == 'OK'
    assert response.get('uuid')
    assert response.get('uuid62')
    assert response.get('uuid_2')
    assert response.get('uuid62_2')
    assert response.get('jwt')
    token = jwt.decode(response['jwt'], 'UP4014, the biggest steam engine', audience='http://test.org', algorithms=["HS256"])
    assert token.get('aud') == 'http://test.org'
    assert token.get('iss') == 'http://cserver.org'
    assert token.get('jti') == '1234567890'
    assert token.get('key') == 'abcdefghijk'
    assert token.get('prn') == 'https://test.org/gaga'

