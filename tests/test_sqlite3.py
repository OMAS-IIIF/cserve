import pytest

def test_sqlite3(manager):
    print('==================================================')
    response = manager.get_json('sqlite3')
    print("**************************************************")
    print(response)
    assert response.get('status') == 'OK'
    assert response.get('result')
    assert response['result'].get('1c1dcc9d-8e80-43a6-8421-56c5b5f42de7') == "Though this be madness, yet there is method in 't. Will you walk out of the air, my lord?"
