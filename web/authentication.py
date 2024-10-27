from datetime import datetime, timezone

import settings


def lcg_random(seed:int, salt:int, size:int) -> int:
    a=1664525
    c=1013904223
    current = seed
    max_number = 10 ** size
    for i in range(salt):
        current = (a * current + c) % max_number
    return current

def get_current_minute_timestamp()->int:
    return int(datetime.now(timezone.utc).timestamp() // 60)

def generate_current_token(seed=None)->str:
    if not seed:
        seed = get_current_minute_timestamp()
    expected = lcg_random(seed=seed,
                          salt=settings.AUTHENTICATION_SALT,
                          size=settings.TOKEN_SIZE)
    return str(expected).zfill(settings.TOKEN_SIZE)

def authenticate(token:str)->bool:
    current_minute_ts = get_current_minute_timestamp()

    # Generate the interval to be tested always starting with 0,
    # to increase the chances of a hit in the first step
    minutes = [0]
    for i in range(1, (settings.AUTHENTICATION_WINDOW_MINUTES // 2) + 1):
        minutes += [-i, i]

    for delta in minutes:
        expected_token = generate_current_token(current_minute_ts + delta)
        print("Expected: ", expected_token, " Provided: ", token)
        if token == expected_token:
            return True

    return False
