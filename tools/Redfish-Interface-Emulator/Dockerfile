FROM python:3-slim

# For healthcheck
RUN apt-get update && apt-get install curl -y

# Install python requirements
COPY requirements.txt /tmp/
RUN pip install --upgrade pip && \
    pip install --no-cache-dir -r /tmp/requirements.txt

# Copy server files
COPY . /usr/src/app/.

# Env settings
EXPOSE 5000
HEALTHCHECK CMD curl --fail http://127.0.0.1:5000/redfish/v1/ || exit 1
WORKDIR /usr/src/app
ENTRYPOINT ["python", "emulator.py"]
