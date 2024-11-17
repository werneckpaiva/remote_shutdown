import logging

from flask import Flask, request, jsonify
from functools import wraps

import settings
from remote_shutdown import shutdown
from remote_shutdown.authentication import authenticate

app = Flask(__name__)

logging.basicConfig(level=settings.LOG_LEVEL)
logger = logging.getLogger(__name__)

class InvalidTokenException(Exception):
    pass

def validate_token():
    def decorator(func):
        @wraps(func)
        def wrapper(*args, **kwargs):
            token = request.args.get('token', type=str)
            if not authenticate(token):
                raise InvalidTokenException(f"Invalid token '{token}'")
            return func(*args, **kwargs)
        return wrapper
    return decorator


@app.route('/shutdown', methods=['GET'])
@validate_token()
def route_shutdown():
    shutdown_status = shutdown.shutdown(settings.DEFAULT_SHUTDOWN_DELAY_MIN)
    logger.info(f"Shutdown scheduled: {shutdown_status.scheduled_time}")
    return jsonify(shutdown_status.to_dict()), 200


@app.route('/suspend', methods=['GET'])
@validate_token()
def route_suspend():
    shutdown_status  = shutdown.suspend(settings.DEFAULT_SHUTDOWN_DELAY_MIN)
    logger.info(f"Suspend scheduled: {shutdown_status.scheduled_time}")
    return jsonify(shutdown_status.to_dict()), 200


@app.route('/cancel', methods=['GET'])
@validate_token()
def route_cancel():
    shutdown_status = shutdown.status()
    if shutdown_status.status == shutdown.StatusEnum.SCHEDULED:
        shutdown.cancel()
        logger.info(f"{shutdown_status.mode} canceled")
        shutdown_status.status = shutdown.StatusEnum.CANCELED
    return jsonify(shutdown_status.to_dict()), 200


@app.route('/status', methods=['GET'])
@validate_token()
def route_status():
    return jsonify(shutdown.status().to_dict()), 200


@app.errorhandler(Exception)
def handle_exception(e):
    logger.error(e)
    return jsonify({'error': str(e)}), 500


@app.errorhandler(InvalidTokenException)
def handle_exception(e):
    logger.error("Invalid token")
    return jsonify({'error': str(e)}), 401


if __name__ == '__main__':
    app.run(debug=settings.DEBUG, port=settings.DEFAULT_PORT, host='0.0.0.0')
