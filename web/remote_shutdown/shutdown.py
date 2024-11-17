import os
import subprocess
from dataclasses import dataclass
from datetime import datetime
from enum import Enum
from typing import Optional, Literal, TypeVar

import math


class StatusEnum(Enum):
    NONE = "NONE"
    SCHEDULED = "SCHEDULED"
    CANCELED = "CANCELED"


T = TypeVar('T', bound='ShutdownStatus')


@dataclass
class ShutdownStatus:
    status: StatusEnum
    mode: Optional[Literal["SHUTDOWN", "SUSPEND"]] = None
    scheduled_time: Optional[datetime] = None
    message: Optional[str] = None

    def to_dict(self) -> dict:
        status_dict = {
            'status': self.status.value
        }
        if self.mode:
            status_dict['mode'] = self.mode
        if self.scheduled_time:
            status_dict['time'] = self.scheduled_time.strftime('%Y-%m-%d %H:%M:%S')
        if self.message:
            status_dict['message'] = self.message
        return status_dict

    @classmethod
    def no_shutdown_scheduled(cls: T) -> T:
        return ShutdownStatus(status=StatusEnum.NONE)


def get_shutdown_status() -> ShutdownStatus:
    try:
        with open('/run/systemd/shutdown/scheduled', 'r') as file:
            config = {}
            for line in file:
                if '=' in line:
                    key, value = line.split('=', 1)
                    config[key.strip()] = value.strip()
            return ShutdownStatus(
                status=StatusEnum.SCHEDULED,
                message=config.get('WALL_MESSAGE'),
                mode="SHUTDOWN",
                scheduled_time=datetime.fromtimestamp(int(config.get('USEC', 0)) / 1_000_000)
            )
    except FileNotFoundError:
        return ShutdownStatus.no_shutdown_scheduled()


def get_scheduled_suspend_id() -> Optional[str]:
    result = subprocess.run(['atq'], capture_output=True, text=True)
    result.check_returncode()
    for line in result.stdout.split("\n"):
        line_parts = line.strip().split('\t')
        if line and len(line_parts) > 0:
            at_id = line_parts[0]
            result = subprocess.run(['at', '-c', at_id], capture_output=True, text=True)
            result.check_returncode()
            if 'suspend' in result.stdout:
                return at_id
    return None


def get_suspend_status() -> ShutdownStatus:
    suspend_id = get_scheduled_suspend_id()
    if not suspend_id:
        return ShutdownStatus.no_shutdown_scheduled()
    result = subprocess.run(['atq', suspend_id], capture_output=True, text=True)
    result.check_returncode()
    parts = result.stdout.split("\t")
    parts = parts[1].split(" a ")
    scheduled_time_str = parts[0]
    scheduled_time = datetime.strptime(scheduled_time_str, '%a %b %d %H:%M:%S %Y')

    return ShutdownStatus(
        status=StatusEnum.SCHEDULED,
        mode="SUSPEND",
        scheduled_time=scheduled_time
    )


def status() -> ShutdownStatus:
    shutdown_status = get_shutdown_status()
    suspend_status = get_suspend_status()
    return min(shutdown_status, suspend_status,
               key=lambda x: math.inf if x.scheduled_time is None else x.scheduled_time.timestamp())


def cancel_shutdown() -> str:
    result = subprocess.run(['shutdown', '-c'], capture_output=True, text=True)
    result.check_returncode()
    return result.stdout


def cancel_suspend() -> None:
    while suspend_id := get_scheduled_suspend_id():
        result = subprocess.run(['atrm', suspend_id], capture_output=True, text=True)
        result.check_returncode()


def cancel():
    cancel_shutdown()
    cancel_suspend()


def shutdown(minutes: int) -> ShutdownStatus:
    cancel()
    result = subprocess.run(['shutdown', '-h', str(minutes), 'Remote shutdown'], capture_output=True, text=True)
    result.check_returncode()
    return get_shutdown_status()


def suspend(minutes: int) -> ShutdownStatus:
    cancel()
    current_dir = os.path.dirname(os.path.abspath(__file__))
    suspend_cmd = os.path.join(current_dir, "bin", "suspend.sh")
    result = subprocess.run(['at', '-f', suspend_cmd, f'now + {minutes} minutes'], capture_output=True, text=True)
    result.check_returncode()
    return get_suspend_status()


def uptime() -> Optional[str]:
    # Check the system uptime
    uptime_result = subprocess.run(['uptime', '-p'], capture_output=True, text=True)
    uptime_result.check_returncode()
    if not uptime_result.stdout:
        return None
    return uptime_result.stdout.strip()
