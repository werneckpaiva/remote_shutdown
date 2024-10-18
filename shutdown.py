import subprocess
from datetime import datetime
from typing import Optional


def shutdown(minutes:int) -> str:
    result = subprocess.run(['shutdown', '-h', str(minutes), 'Remote shutdown'], capture_output=True, text=True)
    result.check_returncode()
    return result.stdout

def cancel():
    result = subprocess.run(['shutdown', '-c'], capture_output=True, text=True)
    result.check_returncode()
    return result.stdout

def status() -> dict[str, str]:
    try:
        with open('/run/systemd/shutdown/scheduled', 'r') as file:
            config={}
            for line in file:
                if '=' in line:
                    key, value = line.split('=', 1)
                    config[key.strip()] = value.strip()
            print(config)
            return {
                'status': 'SCHEDULED',
                'message': config.get('WALL_MESSAGE'),
                'mode': config.get('MODE'),
                'time':  datetime.fromtimestamp(int(config.get('USEC', 0)) / 1_000_000).strftime('%Y-%m-%d %H:%M:%S')
            }
    except FileNotFoundError:
        return {
            'status': 'NOT_SCUEDULED'
        }

def uptime() -> Optional[str]:
    # Check the system uptime
    uptime_result = subprocess.run(['uptime', '-p'], capture_output=True, text=True)
    uptime_result.check_returncode()
    if not uptime_result.stdout:
        return None
    return uptime_result.stdout.strip()

