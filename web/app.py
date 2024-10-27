import logging
from typing import Optional, Any

from flask import Flask, request, jsonify
from functools import wraps
import shutdown
from authentication import authenticate

logging.basicConfig(level=logging.ERROR)

app = Flask(__name__)

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
    result = shutdown.shutdown(1)
    app.logger.info(f"Shutdown scheduled: {result}")
    return jsonify(shutdown.status()), 200

@app.route('/cancel', methods=['GET'])
@validate_token()
def route_cancel():
    status = shutdown.status()
    result = shutdown.cancel()
    app.logger.info(f"Shutdown canceled: {result}")
    if status.get("status") == 'SCHEDULED':
        status['status'] = 'CANCELED'
    return jsonify(status), 200

@app.route('/status', methods=['GET'])
@validate_token()
def route_status():
    return jsonify(shutdown.status()), 200

@app.errorhandler(Exception)
def handle_exception(e):
    app.logger.error(e)
    return jsonify({'error': str(e)}), 500

@app.errorhandler(Exception)
def handle_exception(e):
    app.logger.error("Invalid token")
    return jsonify({'error': str(e)}), 401


if __name__ == '__main__':
    app.run(debug=True, port=8090, host='0.0.0.0')
